#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <ESP8266WebServer.h>
#include <FS.h>

class WebHandler
{
  private:
    const int ledPin = 2;
    ESP8266WebServer &server;

  public:
    WebHandler(ESP8266WebServer &server) : server(server)
    {
        setup();
    }

  private:
    void setup()
    {
        server.on("/data.js", [&]() { handleDataFile(); });
        server.on("/raw", [&]() { handleRaw(); });
        server.on("/data", [&]() { handleRawData(); });
        server.on("/restart", [&]() { handleRestart(); });

        server.onNotFound([&]() {
            if (!handleFileRead(server.uri()))
            {
                server.send(404, "text/plain", F("404: Not Found"));
            }
        });
    }

    void handleDataFile()
    {
        digitalWrite(ledPin, LOW);

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
        digitalWrite(ledPin, HIGH);
    }

    bool handleFileRead(String path)
    {
        digitalWrite(ledPin, LOW);

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

            digitalWrite(ledPin, HIGH);
            return true;
        }
        DEBUGLOG("WebHandler", "\tFile Not Found: %s", path.c_str());

        digitalWrite(ledPin, HIGH);
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

    void handleRaw()
    {
        digitalWrite(ledPin, LOW);

        FSInfo fs_info;
        SPIFFS.info(fs_info);

        date_time dt = DataManager.getCurrentTime();
        uint32_t values[TARIFFS_COUNT];

        int month = dt.Month;
        if (dt.Day < DataManager.settings.billDay)
            month--;
        if (month < 1)
            month += 12;

        String result = SF("WiFi: ") + WiFi.SSID() + SF(", ") + WiFi.localIP().toString() + SF(", ") + String(WiFi.RSSI());
        result += SF("<br/>\n");

        result += String(dt.Hour) + SF(":") + String(dt.Minute) + SF(":") + String(dt.Second) + SF(" ") +
                  String(dt.Day) + SF("/") + String(dt.Month) + SF("/") + String(dt.Year);
        result += SF(" (millis: ") + String(millis()) + SF(")");
        result += SF("<br/>\n");

        result += SF("LastDistrDay: ") + String(DataManager.settings.lastDistributeDay) + SF(", ");
        result += SF("Tariffs: ");
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            result += String(DataManager.settings.tariffStartHours[t]);
            result += SF(" (") + String(DataManager.settings.tariffPrices[t]) + SF("), ");
        }
        result += SF("BillDay: ") + String(DataManager.settings.billDay) + SF(", ");
        result += SF("CurrSymbols: '") + String(DataManager.settings.currencySymbols) + SF("', ");
        result += SF("Monitors: ");
        for (int m = 0; m < MONITORS_COUNT; m++)
            result += String(DataManager.settings.monitorsNames[m]) + SF("; ");
        result += SF("Settings size: ") + String(sizeof(DataManager.settings));
        result += SF("<br/>\n");

        result += SF("SPIFFS: ") + String(fs_info.usedBytes) + SF("/") + String(fs_info.totalBytes);
        result += SF("<br/>\n<br/>\n");

        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("Current: ") + String(DataManager.getCurrent(m)) + SF(" A, ");
            result += SF("Energy: ") + String(DataManager.getEnergy(m)) + SF(" W ");
            result += SF("<br/>\n");
        }
        result += SF("<br/>");

        result += SF("<table style='width:100%'>\n");
        result += SF("\t<tr><td></td>");
        for (int i = 0; i < 31; i++)
        {
            result += SF("<td>");
            result += String(i) + SF(" ");
            result += SF("</td>");
        }
        result += SF("</tr>\n");

        result += SF("<tr><td>");
        result += SF("Hours:");
        result += SF("</td></tr>\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("\t<tr><td>");
            result += SF("Monitor ") + String(m) + SF(": ");
            result += SF("</td>");
            for (int i = 0; i < 24; i++)
            {
                result += SF("<td>");
                if (i != dt.Hour)
                    result += toString(DataManager.settings.hours[i][m]) + SF(" ");
                else
                {
                    result += SF("<font color='red'>");
                    result += toString(DataManager.getCurrentHourEnergy(m));
                    result += SF("</font> ");
                }
                result += SF("</td>");
            }
            result += SF("</tr>\n");
        }

        result += SF("\n");
        result += SF("<tr><td>");
        result += SF("Days:");
        result += SF("</td></tr>\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("\t<tr><td>");
            result += SF("Monitor ") + String(m);
            result += SF("</td></tr>\n");
            DataManager.getCurrentDayEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += SF("\t\t<tr><td>");
                result += SF("Tariff ") + String(t) + SF(": ");
                result += SF("</td>");
                for (int i = 0; i < 31; i++)
                {
                    result += SF("<td>");
                    if (i != dt.Day - 1)
                        result += toString(DataManager.settings.days[i][t][m]) + SF(" ");
                    else
                    {
                        result += SF("<font color='red'>");
                        result += toString(values[t]);
                        result += SF("</font> ");
                    }
                    result += SF("</td>");
                }
                result += SF("</tr>\n");
            }
        }

        result += SF("\n");
        result += SF("<tr><td>");
        result += SF("Months:");
        result += SF("</td></tr>\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("\t<tr><td>");
            result += SF("Monitor ") + String(m);
            result += SF("</td></tr>\n");
            DataManager.getCurrentMonthEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += SF("\t\t<tr><td>");
                result += SF("Tariff ") + String(t) + SF(": ");
                result += SF("</td>");
                for (int i = 0; i < 12; i++)
                {
                    result += SF("<td>");
                    if (i != month - 1)
                        result += toString(DataManager.settings.months[i][t][m]) + SF(" ");
                    else
                    {
                        result += SF("<font color='red'>");
                        result += toString(values[t]);
                        result += SF("</font> ");
                    }
                    result += SF("</td>");
                }
                result += SF("</tr>\n");
            }
        }
        result += SF("</table>");

        server.send(200, "text/html", result);
        digitalWrite(2, HIGH);
    }

    String toString(const uint32_t &value)
    {
        String str = "";
        if (value > 1000 * 100)
        {
            str += String(value / (1000 * 100)) + " ";
            if ((value / 100) % 1000 < 100)
                str += "0";
            if ((value / 100) % 1000 < 10)
                str += "0";
        }
        str += String((value / 100) % 1000) + ".";
        if (value > 100 && value % 100 < 10)
            str += "0";
        str += String(value % 100);
        return str;
    }

    void handleRawData()
    {
        digitalWrite(ledPin, LOW);

        String result = "[";
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            uint32_t values[TARIFFS_COUNT];
            DataManager.getCurrentMonthEnergy(m, values);
            uint32_t sum = 0;
            for (int t = 0; t < TARIFFS_COUNT; t++)
                sum += values[t];
            result += "[" + String(DataManager.getEnergy(m)) + ", ";
            result += String(sum * 0.01) + "]";
            if (m < MONITORS_COUNT - 1)
                result += ", ";
        }
        result += "]";

        server.send(200, "text/plain", result);
        digitalWrite(ledPin, HIGH);
    }

    void handleRestart()
    {
        server.client().setNoDelay(true);
        server.send(200, "text/html", F("<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...\n")); 
        delay(100);
        server.client().stop();
        ESP.restart();
    }
};

#endif