#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <EEPROM.h>

#include "Data.h"
#include "src/NTPClient.h"
#include "src/ADS1015.h"
#include "src/EnergyMonitor.h"

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

public:
    Data data;

    DataManagerClass() : monitor1(ads, 0), monitor2(ads, 1), monitor3(ads, 2), monitor4(ads, 3)
    {
        startTime = 0;
        timer = 0;

        EEPROM.begin(4096);
        data.readEEPROM();
        if (data.startTime == -1) // reset data if EEPROM is empty
            data.reset();
    }

    void setup()
    {
        // retry 5 times to get the time, else try every minute on update
        for (int i = 0; i < 5; i++)
        {
            if (setStartTime(getTime()))
                break;
        }

        date_time dt = getCurrentTime();
        DEBUGLOG("DataManager", "Start time: %s (%d)", dateTimeToString(dt).c_str(),
                 startTime + data.settings.timeZone * SECONDS_IN_AN_HOUR + (millis() / MILLIS_IN_A_SECOND));
        timer = millis() - dt.Second * MILLIS_IN_A_SECOND;

        ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        ads.begin();
    }

    void update()
    {
        for (int m = 0; m < MONITORS_COUNT; m++)
            getMonitor(m).update();

        if (millis() - timer > MILLIS_IN_A_MINUTE)
        {
            // if millis rollover then fix the start time by re-setting
            if (timer > millis())
            {
                startTime = 0;
                DEBUGLOG("DataManager", "Millis rollover")
            }
            timer = millis();

            // if time isn't received in the setup, try again every 15th second
            if (startTime == 0 && (millis() / MILLIS_IN_A_SECOND) % 15 == 0)
                setStartTime(getTime());

            date_time dt = getCurrentTime();
            DEBUGLOG("DataManager", "Current time: %s", dateTimeToString(dt).c_str());

            // if new hour begins
            if (dt.Hour != data.lastSaveHour)
            {
                data.lastSaveHour = dt.Hour;

                int prevHour = dt.Hour - 1;
                if (prevHour < 0)
                    prevHour += 24;
                DEBUGLOG("DataManager", "Save data for %d hour", prevHour);
                for (int m = 0; m < MONITORS_COUNT; m++)
                {
                    uint32_t sum = 0;
                    for (int i = 0; i < 60; i++)
                    {
                        if (data.minutesBuffer[m][i] != 0xFFFFFFFF)
                            sum += data.minutesBuffer[m][i];
                    }
                    data.hours[prevHour][m] = sum;

                    DEBUGLOG("DataManager", "Consumption for %d hour monitor %d: %d",
                             prevHour, m, data.hours[prevHour][m]);
                }

                distributeData(dt);

                data.writeEEPROM();
            }

            for (int m = 0; m < MONITORS_COUNT; m++)
                data.writeToMinutesBuffer(m, dt.Minute, getMonitor(m).getEnergy());
        }
    }

    inline double getCurrent(const int &monitorIdx)
    {
        return getMonitor(monitorIdx).current * data.settings.coefficient;
    }

    inline double getEnergy(const int &monitorIdx)
    {
        EnergyMonitor &monitor = getMonitor(monitorIdx);
        return monitor.current * monitor.voltage * data.settings.coefficient;
    }

    inline uint32_t getCurrentHourEnergy(const int &monitorIdx)
    {
        uint32_t sum = 0;
        for (int i = 0; i < 60; i++)
        {
            if (data.minutesBuffer[monitorIdx][i] != 0xFFFFFFFF)
                sum += data.minutesBuffer[monitorIdx][i];
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
                value = data.hours[h][monitorIdx];
            else
                value = getCurrentHourEnergy(monitorIdx);

            if (h >= data.settings.tariffStartHours[0] && h < data.settings.tariffStartHours[1])
                values[0] += value;
            else if (h >= data.settings.tariffStartHours[1] && h < data.settings.tariffStartHours[2])
                values[1] += value;
            else if (h >= data.settings.tariffStartHours[2] || h < data.settings.tariffStartHours[0])
                values[2] += value;
        }
    }

    inline void getCurrentMonthEnergy(const int &monitorIdx, uint32_t values[TARIFFS_COUNT])
    {
        date_time dt = getCurrentTime();

        int year = dt.Year;
        int prevMonth = dt.Month;
        if (dt.Day < data.settings.billDay)
        {
            prevMonth = dt.Month - 1;
            if (prevMonth == 0)
            {
                prevMonth += 12;
                year--;
            }
        }
        const uint8_t daysCount = getMonthLength(prevMonth, year);

        uint32_t currentDay[TARIFFS_COUNT];
        getCurrentDayEnergy(monitorIdx, currentDay);
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            values[t] = 0;
            for (int d = 0; d < daysCount; d++)
            {
                if ((dt.Day >= data.settings.billDay && d + 1 >= data.settings.billDay && d + 1 <= dt.Day) ||
                    (dt.Day < data.settings.billDay && (d + 1 >= data.settings.billDay || d + 1 <= dt.Day)))
                {
                    if (d != dt.Day - 1)
                        values[t] += data.days[d][t][monitorIdx];
                    else
                        values[t] += currentDay[t];
                }
            }
        }
    }

    inline date_time getCurrentTime() const
    {
        uint32_t sTime = startTime != 0 ? startTime : data.startTime;
        return breakTime(sTime + data.settings.timeZone * SECONDS_IN_AN_HOUR + (millis() / MILLIS_IN_A_SECOND));
    }

    inline bool setStartTime(const uint32_t &value)
    {
        if (value == 0)
            return false;

        startTime = value;
        startTime -= millis() / MILLIS_IN_A_SECOND;
        data.startTime = startTime;
        return true;
    }

private:
    void distributeData(const date_time &dt)
    {
        if (dt.Day == data.lastSaveDay) // if not the first update for the new day
            return;
        data.lastSaveDay = dt.Day;
        // re-sync current time once per day
        startTime = 0;

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

        DEBUGLOG("DataManager", "Save data for %d day", prevDay);
        for (int i = 0; i < MONITORS_COUNT; i++)
        {
            uint32_t sum[3] = {0, 0, 0};
            for (int h = 0; h < 24; h++)
            {
                if (h >= data.settings.tariffStartHours[0] && h < data.settings.tariffStartHours[1])
                    sum[0] += data.hours[h][i];
                else if (h >= data.settings.tariffStartHours[1] && h < data.settings.tariffStartHours[2])
                    sum[1] += data.hours[h][i];
                else if (h >= data.settings.tariffStartHours[2] || h < data.settings.tariffStartHours[0])
                    sum[2] += data.hours[h][i];
            }
            data.days[prevDay - 1][0][i] = sum[0];
            data.days[prevDay - 1][1][i] = sum[1];
            data.days[prevDay - 1][2][i] = sum[2];

            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, data.days[prevDay - 1][0][i]);
            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, data.days[prevDay - 1][1][i]);
            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, data.days[prevDay - 1][2][i]);
            yield();
        }

        // the data for the month is full or if we miss the bill day
        if ((dt.Month != data.lastSaveMonth && dt.Day >= data.settings.billDay) ||
            dt.Month > data.lastSaveMonth + 1)
        {
            if (dt.Month > data.lastSaveMonth)
                data.lastSaveMonth++;

            DEBUGLOG("DataManager", "Save data for %d month", prevMonth);
            for (int i = 0; i < MONITORS_COUNT; i++)
            {
                for (int t = 0; t < TARIFFS_COUNT; t++)
                {
                    uint32_t sum = 0;
                    for (int d = 0; d < daysCount; d++)
                    {
                        sum += data.days[d][t][i];
                    }
                    data.months[prevMonth - 1][t][i] = sum;

                    DEBUGLOG("DataManager", "Consumption for %d month (%d tariff) monitor %d: %d",
                             prevMonth, t, i, data.months[prevMonth - 1][t][i]);
                }
                yield();
            }
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
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
DataManagerClass DataManager;
#endif

#endif