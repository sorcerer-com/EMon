#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <WiFiUdp.h>

#define DBG_PORT Serial

#define DEBUG_NTPCLIENT

#ifdef DEBUG_NTPCLIENT
#define DEBUGLOG(...) DBG_PORT.printf(__VA_ARGS__)
#else
#define DEBUGLOG(...)
#endif

#define DEFAULT_NTP_SERVER "pool.ntp.org" // Default international NTP server
#define DEFAULT_NTP_PORT 123              // Default local udp port
#define NTP_TIMEOUT 1500                  // Response timeout for NTP requests

// leap year calculator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

#define SEVENTY_YEARS 2208988800UL

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

static  const uint8_t monthDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // API starts months from 1, this array starts from 0

typedef struct
{
    uint8_t Second;
    uint8_t Minute;
    uint8_t Hour;
    uint8_t Wday; // day of week, sunday is day 1
    uint8_t Day;
    uint8_t Month;
    uint8_t Year; // offset from 1970;
} date_time;

bool sendNTPpacket(const char *address, UDP &udp, uint8_t *ntpPacketBuffer);

date_time breakTime(uint32_t time)
{
    // break the given time into time components
    // this is a more compact version of the C library localtime function
    // note that year is offset from 1970 !!!
    date_time dt;

    uint8_t year;
    uint8_t month, monthLength;
    unsigned long days;

    time = (uint32_t)time;
    dt.Second = time % 60;
    time /= 60; // now it is minutes
    dt.Minute = time % 60;
    time /= 60; // now it is hours
    dt.Hour = time % 24;
    time /= 24;                     // now it is days
    dt.Wday = ((time + 4) % 7) + 1; // Sunday is day 1

    year = 0;
    days = 0;
    while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time)
    {
        year++;
    }
    dt.Year = year; // year is offset from 1970

    days -= LEAP_YEAR(year) ? 366 : 365;
    time -= days; // now it is days in this year, starting at 0

    days = 0;
    month = 0;
    monthLength = 0;
    for (month = 0; month < 12; month++)
    {
        if (month == 1)
        { // february
            if (LEAP_YEAR(year))
            {
                monthLength = 29;
            }
            else
            {
                monthLength = 28;
            }
        }
        else
        {
            monthLength = monthDays[month];
        }

        if (time >= monthLength)
        {
            time -= monthLength;
        }
        else
        {
            break;
        }
    }
    dt.Month = month + 1; // jan is month 1
    dt.Day = time + 1;    // day of month
    return dt;
}

// get UNIX time
uint32_t getTime()
{
    uint8_t ntpPacketBuffer[NTP_PACKET_SIZE]; //Buffer to store response message

    DEBUGLOG("Starting UDP\n");
    WiFiUDP udp;
    udp.begin(DEFAULT_NTP_PORT);
    DEBUGLOG("UDP port: %d\n", udp.localPort());
    while (udp.parsePacket() > 0)
        ; // discard any previously received packets

    sendNTPpacket(DEFAULT_NTP_SERVER, udp, ntpPacketBuffer);
    uint32_t beginWait = millis();
    while (millis() - beginWait < NTP_TIMEOUT)
    {
        int size = udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            DEBUGLOG("-- Receive NTP Response\n");
            udp.read(ntpPacketBuffer, NTP_PACKET_SIZE); // read packet into the buffer
            unsigned long timeValue;
            // convert four bytes starting at location 40 to a long integer
            timeValue = (unsigned long)ntpPacketBuffer[40] << 24;
            timeValue |= (unsigned long)ntpPacketBuffer[41] << 16;
            timeValue |= (unsigned long)ntpPacketBuffer[42] << 8;
            timeValue |= (unsigned long)ntpPacketBuffer[43];
            udp.stop();
            return timeValue - SEVENTY_YEARS;
        }
    }
    DEBUGLOG("-- No NTP Response :-(\n");
    udp.stop();
    return 0; // return 0 if unable to get the time
}

bool sendNTPpacket(const char *address, UDP &udp, uint8_t *ntpPacketBuffer)
{
    // set all bytes in the buffer to 0
    memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    ntpPacketBuffer[0] = 0b11100011; // LI, Version, Mode
    ntpPacketBuffer[1] = 0;          // Stratum, or type of clock
    ntpPacketBuffer[2] = 6;          // Polling Interval
    ntpPacketBuffer[3] = 0xEC;       // Peer Clock Precision
                                     // 8 bytes of zero for Root Delay & Root Dispersion
    ntpPacketBuffer[12] = 49;
    ntpPacketBuffer[13] = 0x4E;
    ntpPacketBuffer[14] = 49;
    ntpPacketBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    udp.beginPacket(address, DEFAULT_NTP_PORT); //NTP requests are to port 123
    udp.write(ntpPacketBuffer, NTP_PACKET_SIZE);
    udp.endPacket();
    return true;
}

#endif