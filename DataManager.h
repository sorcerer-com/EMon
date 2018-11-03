#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <EEPROM.h>

#include "Settings.h"
#include "src/NTPClient.h"
#include "src/ADS1015.h"
#include "src/EnergyMonitor.h"

class DataManagerClass
{
  public: // TODO: private
    uint32_t startTime;
    uint32_t timer;

    ADS1115 ads;
    EnergyMonitor monitor1;
    EnergyMonitor monitor2;
    EnergyMonitor monitor3;
    EnergyMonitor monitor4;

  public:
    Settings settings;

    DataManagerClass() : monitor1(ads, 0), monitor2(ads, 1), monitor3(ads, 2), monitor4(ads, 3)
    {
    }

    void setup()
    {
        EEPROM.begin(4096);
        readEEPROM(settings);

        startTime = getTime();
        date_time dt = breakTime(startTime);
        DEBUGLOG("DataManager", "Start time: %02d:%02d:%02d %02d/%02d/%04d",
                 dt.Hour, dt.Minute, dt.Second, dt.Day, dt.Month, dt.Year);
        timer = millis() - (dt.Minute * 60 + dt.Second) * 1000;

        ads.setGain(GAIN_ONE); // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
        ads.begin();
    }

    void update()
    {
        for (int i = 0; i < MONITORS_COUNT; i++)
            getMonitor(i).update();

        // TODO: start bill day isn't taken in to count
        if (millis() - timer > 60 * 60 * 1000) // TODO: to const / do it more offten?
        {
            timer = millis();

            uint32_t currentTime = startTime + settings.timeZone * 3600 + (millis() / 1000); // TODO: maybe to consts (hour & millis)
            date_time dt = breakTime(currentTime);
            DEBUGLOG("DataManager", "Current time: %02d:%02d:%02d %02d/%02d/%04d",
                     dt.Hour, dt.Minute, dt.Second, dt.Day, dt.Month, dt.Year);

            distributeData(dt);

            int prevHour = dt.Hour - 1;
            if (prevHour < 0)
                prevHour += 24;
            for (int i = 0; i < MONITORS_COUNT; i++)
            {
                settings.hours[prevHour][i] = getMonitor(i).getEnergy(); // TODO: add some justification value (*0.88)

                DEBUGLOG("DataManager", "Consumption for %d hour monitor %d: %d",
                         prevHour, i, settings.hours[prevHour][i]);
            }
            writeEEPROM(settings);
        }
    }

    inline double getCurrent(const int &monitorIdx)
    {
        return getMonitor(monitorIdx).current;
    }

  private:
    void distributeData(const date_time &dt)
    {
        if (dt.Hour != 1) // if not the first update for the new day
            return;

        // TODO: test it:
        int prevMonth = dt.Month - 1;
        if (prevMonth == 0)
            prevMonth += 12;
        const uint8_t daysCount = monthDays[prevMonth - 1];
        int prevDay = dt.Day - 1;
        if (prevDay == 0)
            prevDay += daysCount;
        if (dt.Day == 2) // the data for the first day of the new month is full
        {
            for (int i = 0; i < MONITORS_COUNT; i++)
            {
                for (int t = 0; t < TARIFFS_COUNT; t++)
                {
                    uint32_t sum = 0;
                    for (int d = 0; d < daysCount; d++)
                    {
                        sum += settings.days[d][t][i];
                        settings.days[d][t][i] = 0;
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
                if (h >= settings.tariffHours[0] && h < settings.tariffHours[1])
                    sum[0] += settings.hours[h][i];
                else if (h >= settings.tariffHours[1] && h < settings.tariffHours[2])
                    sum[1] += settings.hours[h][i];
                else if (h >= settings.tariffHours[2] || h < settings.tariffHours[0])
                    sum[2] += settings.hours[h][i];
                settings.hours[h][i] = 0;
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
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
DataManagerClass DataManager;
#endif

#endif