#ifndef SETTINGS_H
#define SETTINGS_H

#include <EEPROM.h>

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

extern "C" uint32_t _EEPROM_start;

class Data
{
public:
    // in 0.01 W
    uint32_t months[12][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t days[31][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t hours[24][MONITORS_COUNT];
    uint32_t startTime = 0;
    uint8_t lastSaveHour = 0;
    uint8_t lastSaveDay = 0;
    uint8_t lastSaveMonth = 0;

    struct Settings
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

    uint32_t minutesBuffer[MONITORS_COUNT][60]; // data for the last hour (per minute)

    void readEEPROM(const bool &includeMinutesBuffer = true)
    {
        if (EEPROM.getDataPtr() == NULL) // if cannot read the EEPROM
            return;

        DEBUGLOG("Data", "Read data with size: %d", sizeof(*this));
        int addr = 0;
        EEPROM.get(addr, months);
        addr += sizeof(months);
        EEPROM.get(addr, days);
        addr += sizeof(days);
        EEPROM.get(addr, hours);
        addr += sizeof(hours);
        EEPROM.get(addr, startTime);
        addr += sizeof(startTime);
        EEPROM.get(addr, lastSaveHour);
        addr += sizeof(lastSaveHour);
        EEPROM.get(addr, lastSaveDay);
        addr += sizeof(lastSaveDay);
        EEPROM.get(addr, lastSaveMonth);
        addr += sizeof(lastSaveMonth);

        EEPROM.get(addr, settings);
        addr += sizeof(settings);

        if (includeMinutesBuffer)
        {
            /* TODO:
            // read minutes buffer from the end of the EEPROM
            uint32_t EEPROM_end = ((uint32_t)&_EEPROM_start - 0x40200000) + SPI_FLASH_SEC_SIZE - 1;
            noInterrupts();
            spi_flash_read(EEPROM_end - sizeof(minutesBuffer), (uint32_t *)minutesBuffer, sizeof(minutesBuffer));
            interrupts();
            */
        }
    }

    void writeEEPROM(const bool &includeMinutesBuffer = false)
    {
        DEBUGLOG("Data", "Write data with size: %d", sizeof(*this));
        // clear the EEPROM data buffer first
        memset(EEPROM.getDataPtr(), 0xFF, EEPROM.length());

        int addr = 0;
        EEPROM.put(addr, months);
        addr += sizeof(months);
        EEPROM.put(addr, days);
        addr += sizeof(days);
        EEPROM.put(addr, hours);
        addr += sizeof(hours);
        EEPROM.put(addr, startTime);
        addr += sizeof(startTime);
        EEPROM.put(addr, lastSaveHour);
        addr += sizeof(lastSaveHour);
        EEPROM.put(addr, lastSaveDay);
        addr += sizeof(lastSaveDay);
        EEPROM.put(addr, lastSaveMonth);
        addr += sizeof(lastSaveMonth);

        EEPROM.put(addr, settings);
        addr += sizeof(settings);

        EEPROM.commit();

        // write minutes buffer in the end of the EEPROM
        if (includeMinutesBuffer)
        {
            /* TODO:
            uint32_t EEPROM_end = ((uint32_t)&_EEPROM_start - 0x40200000) + SPI_FLASH_SEC_SIZE - 1;
            noInterrupts();
            spi_flash_write(EEPROM_end - sizeof(minutesBuffer), reinterpret_cast<uint32_t *>(minutesBuffer), sizeof(minutesBuffer));
            interrupts();
            */
        }
        else
        {
            memset(minutesBuffer, 0xFF, sizeof(minutesBuffer));
        }
    }

    void reset()
    {
        DEBUGLOG("Data", "Reset data");

        memset(months, 0, sizeof(months));
        memset(days, 0, sizeof(days));
        memset(hours, 0, sizeof(hours));
        startTime = 0;
        lastSaveHour = 0;
        lastSaveDay = 0;
        lastSaveMonth = 0;

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

        memset(minutesBuffer, 0xFF, sizeof(minutesBuffer));
    }

    inline bool writeToMinutesBuffer(const int &monitorIdx, const int &valueIdx, uint32_t value)
    {
        if (monitorIdx < 0 || monitorIdx >= MONITORS_COUNT ||
            valueIdx < 0 || valueIdx >= 60)
            return false;

        minutesBuffer[monitorIdx][valueIdx] = value;

        /* TODO:
        // write only this value to EEPROM
        uint32_t EEPROM_end = ((uint32_t)&_EEPROM_start - 0x40200000) + SPI_FLASH_SEC_SIZE - 1;
        int idx = monitorIdx * 60 + valueIdx;
        return spi_flash_write(EEPROM_end - sizeof(minutesBuffer) + idx * sizeof(value), &value, sizeof(value)) == SPI_FLASH_RESULT_OK;
        */
    }
};

#endif