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
#define SECONDS_IN_AN_HOUR 3600
#define MILLIS_IN_AN_HOUR 3600000

#define MONITORS_COUNT 4
#define TARIFFS_COUNT 3
#define MONITOR_NAME_LENGTH 50

struct Settings
{
    // xTARIFFS_COUNT for different tariffs
    uint32_t months[12][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t days[31][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t hours[24][MONITORS_COUNT];
    uint8_t lastDistributeDay = 1;

    int8_t timeZone = 0;
    uint8_t tariffStartHours[TARIFFS_COUNT] = {0, 0, 0};
    uint8_t billDay = 1;
    double tariffPrices[TARIFFS_COUNT] = {0.0, 0.0, 0.0};
    char currencySymbols[5];
    char monitorsNames[MONITORS_COUNT][MONITOR_NAME_LENGTH];
    // add new values in the end
};

void readEEPROM(Settings &settings)
{
    if (EEPROM.getDataPtr() == NULL) // if cannot read the EEPROM
        return;

    EEPROM.get(0, settings);

    // TODO: remove
    settings.lastDistributeDay = 11;
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
}

void writeEEPROM(const Settings &settings)
{
    DEBUGLOG("Settings", "Write settings with size: %d", sizeof(settings));
    EEPROM.put(0, settings);
    EEPROM.commit();
}

#endif