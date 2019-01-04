#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <ESP8266WebServer.h>
#include <FS.h>

class WebHandler
{
  private:
    ESP8266WebServer &server;

  public:
    WebHandler(ESP8266WebServer &server) : server(server)
    {
        setup();
    }

  private:
    void setup()
    {
        server.onNotFound([&]() {
            if (!handleFileRead(server.uri()))
            {
                server.send(404, "text/plain", "404: Not Found");
            }
        });

        server.on("/data.js", HTTP_GET, [&]() { handleDataFile(); });
    }

    void handleDataFile()
    {
        // TODO: maybe split in multiple files
        DEBUGLOG("WebHandler", "Generating data.js");
        unsigned long timer = millis();
        String result = SF("var data = {};\n");
        result += SF("data.monitorsCount = ") + String(MONITORS_COUNT) + SF(";\n");
        result += SF("data.tariffsCount = ") + String(TARIFFS_COUNT) + SF(";\n");

        date_time dt = DataManager.getCurrentTime();
        result += SF("data.time = '") + datetimeToISOString(dt) + SF("';\n");
        result += SF("\n");

        result += SF("data.settings = {};\n");
        result += SF("data.settings['Time Zone'] = ") + String(DataManager.settings.timeZone) + SF(";\n");
        result += SF("data.settings['Tariff Start Hours'] = [];\n");
        result += SF("data.settings['Tariff Prices'] = [];\n");
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            result += SF("data.settings['Tariff Start Hours'][") + String(t) + SF("] = ");
            result += String(DataManager.settings.tariffStartHours[t]) + SF(";\n");
            result += SF("data.settings['Tariff Prices'][") + String(t) + SF("] = ");
            result += String(DataManager.settings.tariffPrices[t], 5) + SF(";\n");
        }
        result += SF("data.settings['Bill Day'] = ") + String(DataManager.settings.billDay) + SF(";\n");
        result += SF("data.settings['Currency Symbols'] = '") + String(DataManager.settings.currencySymbols) + SF("';\n");
        result += SF("data.settings['Monitors Names'] = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("data.settings['Monitors Names'][") + String(m) + SF("] = '");
            result += String(DataManager.settings.monitorsNames[m]) + SF("';\n");
        }
        result += SF("\n");

        result += SF("data.current = {};\n");
        result += SF("data.current.energy = [];\n");
        result += SF("data.current.hour = [];\n");
        result += SF("data.current.day = [];\n");
        result += SF("data.current.month = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("data.current.energy[") + String(m) + SF("] = ");
            result += String(DataManager.getEnergy(m)) + SF(";\n");
            result += SF("data.current.hour[") + String(m) + SF("] = ");
            result += String(DataManager.getCurrentHourEnergy(m)) + SF(";\n");

            uint32_t values[TARIFFS_COUNT];
            result += SF("data.current.day[") + String(m) + SF("] = [];\n");
            DataManager.getCurrentDayEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += SF("data.current.day[") + String(m) + SF("][") + String(t) + SF("] = ");
                result += String(values[t]) + SF(";\n");
            }

            result += SF("data.current.month[") + String(m) + SF("] = [];\n");
            DataManager.getCurrentMonthEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += SF("data.current.month[") + String(m) + SF("][") + String(t) + SF("] = ");
                result += String(values[t]) + SF(";\n");
            }
        }
        result += SF("\n");

        result += SF("data.hours = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.hours[") + String(m) + SF("] = [];\n");
            for (int h = 0; h < 24; h++)
            {
                result += SF("data.hours[") + String(m) + SF("][") + String(h) + SF("] = ");
                result += String(DataManager.settings.hours[h][m]) + SF(";\n");
            }
        }
        result += SF("\n");

        result += SF("data.days = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.days[") + String(m) + SF("] = [];\n");
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += SF("// Tariff ") + String(t) + SF("\n");
                result += SF("data.days[") + String(m) + SF("][") + String(t) + SF("] = [];\n");
                for (int d = 0; d < 31; d++)
                {
                    result += SF("data.days[") + String(m) + SF("][") + String(t) + SF("][") + String(d) + SF("] = ");
                    result += String(DataManager.settings.days[d][t][m]) + SF(";\n");
                }
            }
        }
        result += SF("\n");

        result += SF("data.months = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.months[") + String(m) + SF("] = [];\n");
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += SF("// Tariff ") + String(t) + SF("\n");
                result += SF("data.months[") + String(m) + SF("][") + String(t) + SF("] = [];\n");
                for (int i = 0; i < 12; i++)
                {
                    result += SF("data.months[") + String(m) + SF("][") + String(t) + SF("][") + String(i) + SF("] = ");
                    result += String(DataManager.settings.months[i][t][m]) + SF(";\n");
                }
            }
        }
        result += SF("// ") + String(millis() - timer) + SF(", ") + String(result.length());
        DEBUGLOG("WebHandler", "Generating data.js (%d) for %d", result.length(), (millis() - timer));

        server.send(200, "application/javascript", result);
    }

    bool handleFileRead(String path)
    {
        DEBUGLOG("WebHandler", "HandleFileRead: %s", path.c_str());
        if (path.endsWith("/"))
            path += "index.html";
        String contentType = getContentType(path);
        String pathWithGz = path + ".gz";
        if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
        {
            if (SPIFFS.exists(pathWithGz))
                path = pathWithGz;
            File file = SPIFFS.open(path, "r");
            size_t sent = server.streamFile(file, contentType);
            file.close();
            DEBUGLOG("WebHandler", "\tSent file: %s", path.c_str());
            return true;
        }
        DEBUGLOG("WebHandler", "\tFile Not Found: %s", path.c_str());
        return false; // If the file doesn't exist, return false
    }

    inline String getContentType(const String &filename)
    {
        if (filename.endsWith(".htm"))
            return "text/html";
        else if (filename.endsWith(".html"))
            return "text/html";
        else if (filename.endsWith(".css"))
            return "text/css";
        else if (filename.endsWith(".js"))
            return "application/javascript";
        else if (filename.endsWith(".png"))
            return "image/png";
        else if (filename.endsWith(".gif"))
            return "image/gif";
        else if (filename.endsWith(".jpg"))
            return "image/jpeg";
        else if (filename.endsWith(".ico"))
            return "image/x-icon";
        else if (filename.endsWith(".xml"))
            return "text/xml";
        else if (filename.endsWith(".pdf"))
            return "application/x-pdf";
        else if (filename.endsWith(".zip"))
            return "application/x-zip";
        else if (filename.endsWith(".gz"))
            return "application/x-gzip";
        return "text/plain";
    }

    inline String datetimeToISOString(const date_time &dt)
    {
        String result = String(dt.Year) + "-";
        if (dt.Month < 10)
            result += "0";
        result += String(dt.Month) + "-";
        if (dt.Day < 10)
            result += "0";
        result += String(dt.Day) + "T";
        if (dt.Hour < 10)
            result += "0";
        result += String(dt.Hour) + ":";
        if (dt.Minute < 10)
            result += "0";
        result += String(dt.Minute) + ":";
        if (dt.Second < 10)
            result += "0";
        result += String(dt.Second) + "Z";
        return result;
    }
};

#endif