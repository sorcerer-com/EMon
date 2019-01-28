#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <EEPROM.h>

#include "Settings.h"
#include "src/NTPClient.h"
#include "src/ADS1015.h"
#include "src/EnergyMonitor.h"

extern "C" uint32_t _SPIFFS_end;

class DataManagerClass
{
  private:
    uint32_t startTime;
    uint32_t timer;

    ADS1115 ads;
    EnergyMonitor monitor1;
    EnergyMonitor monitor2;
    EnergyMonitor monitor3;
    EnergyMonitor monitor4;

    uint32_t dataBuffer[MONITORS_COUNT][60]; // data for the last hour (per minute)

  public:
    Settings settings;

    DataManagerClass() : monitor1(ads, 0), monitor2(ads, 1), monitor3(ads, 2), monitor4(ads, 3)
    {
        startTime = 0;
        timer = 0;
    }

    void setup()
    {
        EEPROM.begin(4096);
        readEEPROM(settings);

        // read data buffer from the end of the EEPROM
        uint32_t EEPROM_end = ((uint32_t)&_SPIFFS_end - 0x40200000) + SPI_FLASH_SEC_SIZE;
        noInterrupts();
        spi_flash_read(EEPROM_end - sizeof(dataBuffer) - 1, (uint32_t *)dataBuffer, sizeof(dataBuffer));
        interrupts();

        // retry 5 times to get the time, else try every minute on update
        for (int i = 0; i < 5; i++)
        {
            startTime = getTime();
            if (startTime != 0)
                break;
        }

        date_time dt = getCurrentTime();
        DEBUGLOG("DataManager", "Start time: %02d:%02d:%02d %02d/%02d/%04d",
                 dt.Hour, dt.Minute, dt.Second, dt.Day, dt.Month, dt.Year);
        timer = millis() - dt.Second * MILLIS_IN_A_SECOND;

        ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        ads.begin();
    }

    void update()
    {
        for (int i = 0; i < MONITORS_COUNT; i++)
            getMonitor(i).update();

        if (millis() - timer > MILLIS_IN_A_MINUTE)
        {
            timer = millis();

            if (startTime == 0) // if time isn't getted in setup, try again
                startTime = getTime();

            date_time dt = getCurrentTime();
            DEBUGLOG("DataManager", "Current time: %02d:%02d:%02d %02d/%02d/%04d",
                     dt.Hour, dt.Minute, dt.Second, dt.Day, dt.Month, dt.Year);

            if (dt.Minute == 0 && startTime != 0) // in the end of the minute and if there is start time
            {
                distributeData(dt);

                int prevHour = dt.Hour - 1;
                if (prevHour < 0)
                    prevHour += 24;
                for (int m = 0; m < MONITORS_COUNT; m++)
                {
                    uint32_t sum = 0;
                    for (int i = 0; i < 60; i++)
                    {
                        if (dataBuffer[m][i] != 0xFFFFFFFF)
                            sum += dataBuffer[m][i];
                    }
                    settings.hours[prevHour][m] = sum; // TODO: add some justification value (*0.88)

                    DEBUGLOG("DataManager", "Consumption for %d hour monitor %d: %d",
                             prevHour, m, settings.hours[prevHour][m]);
                }
                writeEEPROM(settings);
                memset(dataBuffer, 0xFF, sizeof(dataBuffer));
            }

            for (int m = 0; m < MONITORS_COUNT; m++)
                writeToDataBuffer(m, dt.Minute, getMonitor(m).getEnergy());
        }
    }

    inline double getCurrent(const int &monitorIdx)
    {
        return getMonitor(monitorIdx).current;
    }

    inline double getEnergy(const int &monitorIdx)
    {
        EnergyMonitor &monitor = getMonitor(monitorIdx);
        return monitor.current * monitor.voltage;
    }

    inline uint32_t getCurrentHourEnergy(const int &monitorIdx)
    {
        uint32_t sum = 0;
        for (int i = 0; i < 60; i++)
        {
            if (dataBuffer[monitorIdx][i] != 0xFFFFFFFF)
                sum += dataBuffer[monitorIdx][i];
        }
        return sum + getMonitor(monitorIdx).getEnergy(false);
    }

    inline void getCurrentDayEnergy(const int &monitorIdx, uint32_t values[TARIFFS_COUNT])
    {
        date_time dt = getCurrentTime();
        for (int t = 0; t < TARIFFS_COUNT; t++)
            values[t] = 0;

        uint32_t value = 0;
        for (int h = 0; h <= dt.Hour; h++)
        {
            if (h != dt.Hour)
                value = settings.hours[h][monitorIdx];
            else
                value = getCurrentHourEnergy(monitorIdx);

            if (h >= settings.tariffStartHours[0] && h < settings.tariffStartHours[1])
                values[0] += value;
            else if (h >= settings.tariffStartHours[1] && h < settings.tariffStartHours[2])
                values[1] += value;
            else if (h >= settings.tariffStartHours[2] || h < settings.tariffStartHours[0])
                values[2] += value;
        }
    }

    inline void getCurrentMonthEnergy(const int &monitorIdx, uint32_t values[TARIFFS_COUNT])
    {
        date_time dt = getCurrentTime();

        int year = dt.Year;
        int prevMonth = dt.Month - 1;
        if (prevMonth == 0)
        {
            prevMonth += 12;
            year--;
        }
        const uint8_t daysCount = getMonthLength(prevMonth, year);

        uint32_t currentDay[TARIFFS_COUNT];
        getCurrentDayEnergy(monitorIdx, currentDay);
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            values[t] = 0;
            for (int d = 0; d < daysCount; d++)
            {
                if ((dt.Day >= settings.billDay && d + 1 >= settings.billDay && d + 1 <= dt.Day) ||
                    (dt.Day < settings.billDay && (d + 1 >= settings.billDay || d + 1 <= dt.Day)))
                {
                    if (d != dt.Day - 1)
                        values[t] += settings.days[d][t][monitorIdx];
                    else
                        values[t] += currentDay[t];
                }
            }
        }
    }

    inline date_time getCurrentTime()
    {
        return breakTime(startTime + settings.timeZone * SECONDS_IN_AN_HOUR + (millis() / MILLIS_IN_A_SECOND));
    }

  private:
    void distributeData(const date_time &dt)
    {
        if (dt.Day == settings.lastDistributeDay || dt.Hour == 0) // if not the first update for the new day
            return;
        settings.lastDistributeDay = dt.Day;

        int year = dt.Year;
        int prevMonth = dt.Month - 1;
        if (prevMonth == 0)
        {
            prevMonth += 12;
            year--;
        }
        const uint8_t daysCount = getMonthLength(prevMonth, year);
        int prevDay = dt.Day - 1;
        if (prevDay == 0)
            prevDay += daysCount;
        if (dt.Day == settings.billDay) // the data for the month is full
        {
            for (int i = 0; i < MONITORS_COUNT; i++)
            {
                for (int t = 0; t < TARIFFS_COUNT; t++)
                {
                    uint32_t sum = 0;
                    for (int d = 0; d < daysCount; d++)
                    {
                        sum += settings.days[d][t][i];
                    }
                    settings.months[prevMonth - 1][t][i] = sum;

                    DEBUGLOG("DataManager", "Consumption for %d month (%d tariff) monitor %d: %d",
                             prevMonth, t, i, settings.months[prevMonth - 1][t][i]);
                }
            }
        }

        for (int i = 0; i < MONITORS_COUNT; i++)
        {
            uint32_t sum[3] = {0, 0, 0};
            for (int h = 0; h < 24; h++)
            {
                if (h >= settings.tariffStartHours[0] && h < settings.tariffStartHours[1])
                    sum[0] += settings.hours[h][i];
                else if (h >= settings.tariffStartHours[1] && h < settings.tariffStartHours[2])
                    sum[1] += settings.hours[h][i];
                else if (h >= settings.tariffStartHours[2] || h < settings.tariffStartHours[0])
                    sum[2] += settings.hours[h][i];
            }
            settings.days[prevDay - 1][0][i] = sum[0];
            settings.days[prevDay - 1][1][i] = sum[1];
            settings.days[prevDay - 1][2][i] = sum[2];

            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, settings.days[prevDay - 1][0][i]);
            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, settings.days[prevDay - 1][1][i]);
            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, settings.days[prevDay - 1][2][i]);
        }
    }

    inline EnergyMonitor &getMonitor(const int &idx)
    {
        switch (idx)
        {
        case 0:
            return monitor1;
        case 1:
            return monitor2;
        case 2:
            return monitor3;
        case 3:
            return monitor4;
        }
    }

    inline bool writeToDataBuffer(const int &monitorIdx, const int &valueIdx, uint32_t value)
    {
        if (monitorIdx < 0 || monitorIdx >= MONITORS_COUNT ||
            valueIdx < 0 || valueIdx >= 60)
            return false;

        dataBuffer[monitorIdx][valueIdx] = value;

        // write only this value to EEPROM
        uint32_t EEPROM_end = ((uint32_t)&_SPIFFS_end - 0x40200000) + SPI_FLASH_SEC_SIZE;
        int idx = monitorIdx * 60 + valueIdx;
        return spi_flash_write(EEPROM_end - sizeof(dataBuffer) - 1 + idx * sizeof(value), &value, sizeof(value)) == SPI_FLASH_RESULT_OK;
    }
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
DataManagerClass DataManager;
#endif

#endif