#ifndef SETTINGS_H
#define SETTINGS_H

#include "src/LittleFS/LITTLEFS.h"

#define SF(str) String(F(str))

#define MILLIS_IN_A_SECOND 1000
#define SECONDS_IN_A_MINUTE 60
#define MINUTES_IN_AN_HOUR 60
#define MILLIS_IN_A_MINUTE MILLIS_IN_A_SECOND *SECONDS_IN_A_MINUTE
#define SECONDS_IN_AN_HOUR SECONDS_IN_A_MINUTE *MINUTES_IN_AN_HOUR

#define MONITORS_COUNT 4
#define TARIFFS_COUNT 3
#define CURRENCY_SYMBOLS_LENGTH 5
#define MONITOR_NAME_LENGTH 50
#define PASSWORD_LENGTH 10

class Data
{
public:
    struct
    {
        // in 0.01 W
        uint32_t months[12][TARIFFS_COUNT][MONITORS_COUNT];
        uint32_t days[31][TARIFFS_COUNT][MONITORS_COUNT];
        uint32_t hours[24][MONITORS_COUNT];
        uint32_t startTime = 0;
        uint8_t lastSavedHour = 0;
        uint8_t lastSavedDay = 0;
        uint8_t lastSavedMonth = 0;
    } base;

    uint32_t minutes[MONITORS_COUNT][60]; // data for the last hour (per minute)

    struct
    {
        // basic settings
        int8_t timeZone = 0;
        uint8_t tariffStartHours[TARIFFS_COUNT] = {0, 0, 0};
        double tariffPrices[TARIFFS_COUNT] = {0.0, 0.0, 0.0};
        uint8_t billDay = 1;
        char currencySymbols[CURRENCY_SYMBOLS_LENGTH];
        char monitorsNames[MONITORS_COUNT][MONITOR_NAME_LENGTH];

        // advanced settings
        char password[PASSWORD_LENGTH];
        double coefficient = 1.0;

        // WiFi settings
        char wifi_ssid[32];
        char wifi_passphrase[64];
        uint32_t wifi_ip;
        uint32_t wifi_gateway;
        uint32_t wifi_subnet;
        uint32_t wifi_dns;
        // add new values in the end (in handleDataFile, handleSettings, handleRaw, reset below)
    } settings;

    enum SaveFlags
    {
        Base = 1 << 0,
        Minutes = 1 << 1,
        Settings = 1 << 2,
        ResetMinutes = 1 << 3,
    };

    void load()
    {
        DEBUGLOG("Data", "Read data with size: %d", sizeof(*this));
        reset(); // set default values first

        if (LITTLEFS.exists("/base.dat"))
            readFile("/base.dat", (uint8_t *)&base, sizeof(base));
        else
            DEBUGLOG("Data", "Data file doesn't exist");

        if (LITTLEFS.exists("/minutes.dat"))
            readFile("/minutes.dat", (uint8_t *)minutes, sizeof(minutes));
        else
            DEBUGLOG("Data", "Minutes file doesn't exist");

        if (LITTLEFS.exists("/settings.dat"))
            readFile("/settings.dat", (uint8_t *)&settings, sizeof(settings));
        else
            DEBUGLOG("Data", "Settings file doesn't exist");
    }

    void save(const int &saveFlags)
    {
        if (saveFlags & SaveFlags::Base)
        {
            DEBUGLOG("Data", "Write base data with size: %d", sizeof(base));
            writeFile("/base.dat", (uint8_t *)&base, sizeof(base));
        }

        if (saveFlags & SaveFlags::Minutes)
        {
            DEBUGLOG("Data", "Write minutes data with size: %d", sizeof(minutes));
            writeFile("/minutes.dat", (uint8_t *)minutes, sizeof(minutes));
        }
        if (saveFlags & SaveFlags::ResetMinutes)
        {
            DEBUGLOG("Data", "Reset minutes data");
            memset(minutes, 0xFF, sizeof(minutes));
        }

        if (saveFlags & SaveFlags::Settings)
        {
            DEBUGLOG("Data", "Write settings with size: %d", sizeof(settings));
            writeFile("/settings.dat", (uint8_t *)&settings, sizeof(settings));
        }
    }

    void reset()
    {
        DEBUGLOG("Data", "Reset data");

        memset(base.months, 0, sizeof(base.months));
        memset(base.days, 0, sizeof(base.days));
        memset(base.hours, 0, sizeof(base.hours));
        base.startTime = 0;
        base.lastSavedHour = 0;
        base.lastSavedDay = 0;
        base.lastSavedMonth = 0;

        memset(minutes, 0xFF, sizeof(minutes));

        // basic settings
        settings.timeZone = 0;
        memset(settings.tariffStartHours, 0, sizeof(settings.tariffStartHours));
        memset(settings.tariffPrices, 0, sizeof(settings.tariffPrices));
        settings.billDay = 1;
        memset(settings.currencySymbols, 0, sizeof(settings.currencySymbols));
        memset(settings.monitorsNames, 0, sizeof(settings.monitorsNames));

        // advanced settings
        memset(settings.password, 0, sizeof(settings.password));
        settings.coefficient = 1.0;

        // WiFi settings
        memset(settings.wifi_ssid, 0, sizeof(settings.wifi_ssid));
        memset(settings.wifi_passphrase, 0, sizeof(settings.wifi_passphrase));
        settings.wifi_ip = 0;
        settings.wifi_gateway = 0;
        settings.wifi_subnet = 0;
        settings.wifi_dns = 0;
    }

private:
    inline bool readFile(const char *path, uint8_t *buf, size_t size)
    {
        DEBUGLOG("Data", "Reading file: %s", path);

        File file = LITTLEFS.open(path, FILE_READ);
        if (!file)
        {
            DEBUGLOG("Data", "Failed to open file for reading");
            return false;
        }

        if (file.read(buf, size) != size)
        {
            DEBUGLOG("Data", "Read of the file failed");
            file.close();
            return false;
        }

        file.close();
        return true;
    }

    inline bool writeFile(const char *path, const uint8_t *buf, size_t size)
    {
        DEBUGLOG("Data", "Writing file: %s", path);

        File file = LITTLEFS.open(path, FILE_WRITE);
        if (!file)
        {
            DEBUGLOG("Data", "Failed to open file for writing");
            return false;
        }

        if (file.write(buf, size) != size)
        {
            DEBUGLOG("Data", "Write to file failed");
            file.close();
            return false;
        }

        file.close();
        return true;
    }
};

#endif