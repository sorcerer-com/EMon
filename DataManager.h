#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "Data.h"
#include "src/NTPClient.h"
#include "src/VoltageMonitor.h"
#include "src/EnergyMonitor.h"

#define VERSION "2.0.0"

class DataManagerClass
{
private:
    uint32_t startTime;
    uint32_t timer;
    bool internet; // if the device is connected to the Internet

    EnergyMonitor monitor1;
    EnergyMonitor monitor2;
    EnergyMonitor monitor3;
    EnergyMonitor monitor4;

public:
    Data data;

    DataManagerClass() : monitor1(34), monitor2(35), monitor3(32), monitor4(33)
    {
        startTime = 0;
        timer = 0;
        internet = false;
    }

    void setup()
    {
        // retry 5 times to get the time, else try every minute on update
        for (int i = 0; i < 5; i++)
        {
            if (setStartTime(getTime()))
            {
                internet = true;
                break;
            }
        }

        date_time dt = getCurrentTime();
        DEBUGLOG("DataManager", "Start time: %s (%d)", dateTimeToString(dt).c_str(),
                 startTime + data.settings.timeZone * SECONDS_IN_AN_HOUR + (millis() / MILLIS_IN_A_SECOND));
        timer = millis() - dt.Second * MILLIS_IN_A_SECOND;
    }

    void update()
    {
        for (int m = 0; m < MONITORS_COUNT; m++)
            getMonitor(m).update();
        VoltageMonitor.update();

        uint32_t elapsed = millis() - timer;
        if (elapsed > MILLIS_IN_A_MINUTE)
        {
            // if millis rollover
            if (timer > millis() && internet)
            {
                if (internet) // if there is internet fix the start time by reseting
                    startTime = 0;
                else
                {
                    // if there is no internet add max millis value to startTime and write to storage
                    startTime += ((uint32_t)-1) / MILLIS_IN_A_SECOND;
                    data.base.startTime = startTime + millis() / MILLIS_IN_A_SECOND; // set current time
                    data.save(Data::SaveFlags::Base);
                }
                DEBUGLOG("DataManager", "Millis rollover, internet: %d", internet)
            }

            // if time isn't received in the setup try again to get it
            if (startTime == 0)
            {
                if (setStartTime(getTime()))
                    internet = true;
            }

            date_time dt = getCurrentTime();
            DEBUGLOG("DataManager", "Current time: %s", dateTimeToString(dt).c_str());

            // if new hour begins
            if (dt.Hour != data.base.lastSavedHour)
            {
                data.base.lastSavedHour = dt.Hour;

                int prevHour = dt.Hour - 1;
                if (prevHour < 0)
                    prevHour += 24;
                DEBUGLOG("DataManager", "Save data for %d hour", prevHour);
                for (int m = 0; m < MONITORS_COUNT; m++)
                {
                    uint32_t sum = 0;
                    for (int i = 0; i < 60; i++)
                    {
                        if (data.minutes[m][i] != 0xFFFFFFFF)
                            sum += data.minutes[m][i];
                    }
                    data.base.hours[prevHour][m] = sum;

                    DEBUGLOG("DataManager", "Consumption for %d hour monitor %d: %d",
                             prevHour, m, data.base.hours[prevHour][m]);
                }

                distributeData(dt);

                data.base.startTime = startTime + millis() / MILLIS_IN_A_SECOND; // set current time
                data.save(Data::SaveFlags::Base | Data::SaveFlags::ResetMinutes);
            }

            for (int m = 0; m < MONITORS_COUNT; m++)
                data.minutes[m][dt.Minute] = getMonitor(m).getEnergy();
            data.save(Data::SaveFlags::Minutes);

            timer = millis() - dt.Second * MILLIS_IN_A_SECOND - (elapsed - MILLIS_IN_A_MINUTE);
            if (millis() - timer > 50 * MILLIS_IN_A_SECOND)
                timer += 60 * MILLIS_IN_A_SECOND;
        }
    }

    inline double getVoltage()
    {
        return VoltageMonitor.voltage;
    }

    inline double getCurrent(const int &monitorIdx)
    {
        return getMonitor(monitorIdx).current * data.settings.coefficient;
    }

    inline double getEnergy(const int &monitorIdx)
    {
        EnergyMonitor &monitor = getMonitor(monitorIdx);
        return monitor.current * VoltageMonitor.voltage * data.settings.coefficient;
    }

    inline uint32_t getCurrentHourEnergy(const int &monitorIdx)
    {
        uint32_t sum = 0;
        for (int i = 0; i < 60; i++)
        {
            if (data.minutes[monitorIdx][i] != 0xFFFFFFFF)
                sum += data.minutes[monitorIdx][i];
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
                value = data.base.hours[h][monitorIdx];
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
                        values[t] += data.base.days[d][t][monitorIdx];
                    else
                        values[t] += currentDay[t];
                }
            }
        }
    }

    inline date_time getCurrentTime() const
    {
        uint32_t sTime = startTime != 0 ? startTime : data.base.startTime;
        return breakTime(sTime + data.settings.timeZone * SECONDS_IN_AN_HOUR + (millis() / MILLIS_IN_A_SECOND));
    }

    inline bool setStartTime(const uint32_t &value)
    {
        if (value == 0)
            return false;

        data.base.startTime = value;
        startTime = value;
        startTime -= millis() / MILLIS_IN_A_SECOND;
        return true;
    }

private:
    void distributeData(const date_time &dt)
    {
        if (dt.Day == data.base.lastSavedDay) // if not the first update for the new day
            return;
        data.base.lastSavedDay = dt.Day;
        // re-sync current time once per day if there is internet
        if (internet)
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
                    sum[0] += data.base.hours[h][i];
                else if (h >= data.settings.tariffStartHours[1] && h < data.settings.tariffStartHours[2])
                    sum[1] += data.base.hours[h][i];
                else if (h >= data.settings.tariffStartHours[2] || h < data.settings.tariffStartHours[0])
                    sum[2] += data.base.hours[h][i];
            }
            data.base.days[prevDay - 1][0][i] = sum[0];
            data.base.days[prevDay - 1][1][i] = sum[1];
            data.base.days[prevDay - 1][2][i] = sum[2];

            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, data.base.days[prevDay - 1][0][i]);
            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, data.base.days[prevDay - 1][1][i]);
            DEBUGLOG("DataManager", "Consumption for %d day monitor %d: %d",
                     prevDay, i, data.base.days[prevDay - 1][2][i]);
            yield();
        }

        // the data for the month is full or if we miss the bill day
        if ((dt.Month != data.base.lastSavedMonth && dt.Day >= data.settings.billDay) ||
            dt.Month > data.base.lastSavedMonth + 1)
        {
            data.base.lastSavedMonth = dt.Month;

            DEBUGLOG("DataManager", "Save data for %d month", prevMonth);
            for (int i = 0; i < MONITORS_COUNT; i++)
            {
                for (int t = 0; t < TARIFFS_COUNT; t++)
                {
                    uint32_t sum = 0;
                    for (int d = 0; d < daysCount; d++)
                    {
                        sum += data.base.days[d][t][i];
                    }
                    data.base.months[prevMonth - 1][t][i] = sum;

                    DEBUGLOG("DataManager", "Consumption for %d month (%d tariff) monitor %d: %d",
                             prevMonth, t, i, data.base.months[prevMonth - 1][t][i]);
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

#if !defined(NO_GLOBAL_INSTANCES)
DataManagerClass DataManager;
#endif

#endif