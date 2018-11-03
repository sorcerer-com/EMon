#ifndef SETTINGS_H
#define SETTINGS_H

#define DEBUG
#ifdef DEBUG
#define DEBUGLOG(category, ...)       \
    Serial.printf("%-15s", category); \
    Serial.printf(__VA_ARGS__);       \
    Serial.println()
#else
#define DEBUGLOG(category, ...)
#endif

#include <EEPROM.h>

#define MONITORS_COUNT 4
#define TARIFFS_COUNT 3

struct Settings
{
    int8_t timeZone = 0;
    uint8_t tariffHours[TARIFFS_COUNT] = {0, 0, 0};

    // x3 for different tariffs
    uint32_t months[12][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t days[31][TARIFFS_COUNT][MONITORS_COUNT];
    uint32_t hours[24][MONITORS_COUNT];

    // TODO: remove / init by setup page
    Settings()
    {
        timeZone = 2;
        tariffHours[0] = 7;
        tariffHours[1] = 23;
        tariffHours[2] = 23;
    }
};

void readEEPROM(Settings &settings)
{
    if (EEPROM.getDataPtr() == NULL) // if cannot read the EEPROM
        return;

    EEPROM.get(0, settings);
}

void writeEEPROM(const Settings &settings)
{
    DEBUGLOG("Settings", "Write settings with size: %d", sizeof(settings));
    EEPROM.put(0, settings);
    EEPROM.commit();
}

#endif