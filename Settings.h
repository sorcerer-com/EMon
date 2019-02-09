#ifndef SETTINGS_H
#define SETTINGS_H

#include <EEPROM.h>

#define DEBUG
#ifdef DEBUG
#define DEBUGLOG(category, ...)       \
    Serial.printf("%-15s", category); \
    Serial.printf(__VA_ARGS__);       \
    Serial.println()
#else
#define DEBUGLOG(category, ...)
#endif

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

// TODO: rename to Data
struct Settings
{
    // xTARIFFS_COUNT for different tariffs
    uint32_t months[12][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t days[31][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t hours[24][MONITORS_COUNT];
    uint8_t lastDistributeDay = 1;

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
};

void readEEPROM(Settings &settings)
{
    if (EEPROM.getDataPtr() == NULL) // if cannot read the EEPROM
        return;

    EEPROM.get(0, settings);

    // TODO: remove
    //settings.lastDistributeDay = 11;
    settings.timeZone = 2;
    settings.tariffStartHours[0] = 7;
    settings.tariffStartHours[1] = 23;
    settings.tariffStartHours[2] = 23;
    settings.tariffPrices[0] = 0.17732;
    settings.tariffPrices[1] = 0.10223;
    settings.tariffPrices[2] = 0.10223;
    settings.billDay = 6;
    strcpy(settings.currencySymbols, String("lv.").c_str());
    strcpy(settings.monitorsNames[0], String("Lampi, Kuhnia, Kotle").c_str());
    strcpy(settings.monitorsNames[1], String("Furna, Hol, Kotle").c_str());
    strcpy(settings.monitorsNames[2], String("Boiler, Spalnia, Detska, Kotle").c_str());
    strcpy(settings.monitorsNames[3], "");
    //
    strcpy(settings.password, "");
    settings.coefficient = 1.0;
    //
    strcpy(settings.wifi_ssid, "m68");
    strcpy(settings.wifi_passphrase, "bekonche");
    //settings.wifi_ip = IPAddress(192, 168, 0, 105);
    //settings.wifi_gateway = IPAddress(192, 168, 0, 1);
    //settings.wifi_subnet = IPAddress(255, 255, 255, 0);
    //settings.wifi_dns = IPAddress(192, 168, 0, 1);
}

void writeEEPROM(const Settings &settings)
{
    DEBUGLOG("Settings", "Write settings with size: %d", sizeof(settings));
    // clear the data buffer first
    memset(EEPROM.getDataPtr(), 0xFF, EEPROM.length());
    EEPROM.put(0, settings);
    EEPROM.commit();
}

void resetSettings(Settings &settings)
{
    DEBUGLOG("Settings", "Reset settings");

    memset(settings.months, 0, sizeof(settings.months));
    memset(settings.days, 0, sizeof(settings.days));
    memset(settings.hours, 0, sizeof(settings.hours));
    settings.lastDistributeDay = 0;

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

#endif