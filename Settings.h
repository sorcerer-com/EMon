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

struct Settings
{
    int16_t timeZone = 0;
};

#endif