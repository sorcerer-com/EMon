#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <ESP8266WebServer.h>
#include <FS.h>
#include <StreamString.h>

class WebHandler
{
  private:
    const int ledPin = 2;
    ESP8266WebServer &server;
    bool updated = false;

  public:
    WebHandler(ESP8266WebServer &server) : server(server)
    {
        setup();
    }

  private:
    void setup()
    {
        server.on("/data.js", [&]() { handleDataFile(); });
        server.on("/settings", HTTP_POST, [&]() { handleSettings(); }, [&]() { handleUpdate(); });
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
        result += SF("data.settings.timeZone = ") + String(DataManager.settings.timeZone) + SF(";\n");
        result += SF("data.settings.tariffStartHours = [];\n");
        result += SF("data.settings.tariffPrices = [];\n");
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            result += SF("data.settings.tariffStartHours[") + String(t) + SF("] = ");
            result += String(DataManager.settings.tariffStartHours[t]) + SF(";\n");
            result += SF("data.settings.tariffPrices[") + String(t) + SF("] = ");
            result += String(DataManager.settings.tariffPrices[t], 5) + SF(";\n");
        }
        result += SF("data.settings.billDay = ") + String(DataManager.settings.billDay) + SF(";\n");
        result += SF("data.settings.currencySymbols = '") + String(DataManager.settings.currencySymbols) + SF("';\n");
        result += SF("data.settings.monitorsNames = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("data.settings.monitorsNames[") + String(m) + SF("] = '");
            result += String(DataManager.settings.monitorsNames[m]) + SF("';\n");
        }
        result += SF("data.settings.coefficient = ") + String(DataManager.settings.coefficient) + SF(";\n");
        result += SF("data.settings.wifi_ssid = '") + String(DataManager.settings.wifi_ssid) + SF("';\n");
        result += SF("data.settings.wifi_passphrase = '") + String(DataManager.settings.wifi_passphrase) + SF("';\n");
        result += SF("data.settings.wifi_ip = '") + String(IPAddress(DataManager.settings.wifi_ip).toString()) + SF("';\n");
        result += SF("data.settings.wifi_gateway = '") + String(IPAddress(DataManager.settings.wifi_gateway).toString()) + SF("';\n");
        result += SF("data.settings.wifi_subnet = '") + String(IPAddress(DataManager.settings.wifi_subnet).toString()) + SF("';\n");
        result += SF("data.settings.wifi_dns = '") + String(IPAddress(DataManager.settings.wifi_dns).toString()) + SF("';\n");
        result += SF("\n");

        result += SF("data.current = {};\n");
        result += SF("data.current.energy = [];\n");
        result += SF("data.current.hour = [];\n");
        result += SF("data.current.day = [];\n");
        result += SF("data.current.month = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.current.energy[") + String(m) + SF("] = ");
            result += String(DataManager.getEnergy(m)) + SF(";\n");
            result += SF("data.current.hour[") + String(m) + SF("] = ");
            result += String(DataManager.getCurrentHourEnergy(m)) + SF(";\n");

            uint32_t values[TARIFFS_COUNT];
            DataManager.getCurrentDayEnergy(m, values);
            result += SF("data.current.day[") + String(m) + SF("] = [");
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += String(values[t]);
                if (t < TARIFFS_COUNT - 1)
                    result += SF(", ");
            }
            result += SF("];\n");

            DataManager.getCurrentMonthEnergy(m, values);
            result += SF("data.current.month[") + String(m) + SF("] = [");
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += String(values[t]);
                if (t < TARIFFS_COUNT - 1)
                    result += SF(", ");
            }
            result += SF("];\n");
        }
        result += SF("\n");

        result += SF("data.hours = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.hours[") + String(m) + SF("] = [");
            for (int h = 0; h < 24; h++)
            {
                result += String(DataManager.settings.hours[h][m]);
                if (h < 23)
                    result += SF(", ");
            }
            result += SF("];\n");
        }
        result += SF("\n");

        result += SF("data.days = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.days[") + String(m) + SF("] = [];\n");
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                //result += SF("// Tariff ") + String(t) + SF("\n");
                result += SF("data.days[") + String(m) + SF("][") + String(t) + SF("] = [");
                for (int d = 0; d < 31; d++)
                {
                    result += String(DataManager.settings.days[d][t][m]);
                    if (d < 30)
                        result += SF(", ");
                }
                result += SF("];\n");
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
                //result += SF("// Tariff ") + String(t) + SF("\n");
                result += SF("data.months[") + String(m) + SF("][") + String(t) + SF("] = [");
                for (int i = 0; i < 12; i++)
                {
                    result += String(DataManager.settings.months[i][t][m]);
                    if (i < 11)
                        result += SF(", ");
                }
                result += SF("];\n");
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

    void handleSettings()
    {
        digitalWrite(ledPin, LOW);

        String temp ;
        int listParamIdx = 0;
        for (int i = 0; i < server.args(); i++)
        {
            const String &name = server.argName(i);
            const String &value = server.arg(i);
            temp += name + ": " + value + ", ";

            if (!name.endsWith("[]"))
                listParamIdx = 0;

            // basic settings
            if (name == "timeZone")
                DataManager.settings.timeZone = value.toInt();
            else if (name == "tariffStartHours[]")
            {
                DataManager.settings.tariffStartHours[listParamIdx] = value.toInt();
                listParamIdx++;
            }
            else if (name == "tariffPrices[]")
            {
                DataManager.settings.tariffPrices[listParamIdx] = value.toFloat();
                listParamIdx++;
            }
            else if (name == "billDay")
                DataManager.settings.billDay = value.toInt();
            else if (name == "currencySymbols")
                strcpy(DataManager.settings.currencySymbols, value.c_str());
            else if (name == "monitorsNames[]")
            {
                strcpy(DataManager.settings.monitorsNames[listParamIdx], value.c_str());
                listParamIdx++;
            }
            // advanced settings
            else if (name == "password" && value != "*****")
                strcpy(DataManager.settings.password, value.c_str());
            else if (name == "coefficient")
                DataManager.settings.coefficient = value.toFloat();
            // WiFi settings
            else if (name == "wifi_ssid")
                strcpy(DataManager.settings.wifi_ssid, value.c_str());
            else if (name == "wifi_passphrase")
                strcpy(DataManager.settings.wifi_passphrase, value.c_str());
            else if (name == "wifi_ip")
            {
                IPAddress ip;
                if (value != "" && ip.fromString(value))
                    DataManager.settings.wifi_ip = ip;
                else
                    DataManager.settings.wifi_ip = 0;
            }
            else if (name == "wifi_gateway")
            {
                IPAddress ip;
                if (value != "" && ip.fromString(value))
                    DataManager.settings.wifi_gateway = ip;
                else
                    DataManager.settings.wifi_gateway = 0;
            }
            else if (name == "wifi_subnet")
            {
                IPAddress ip;
                if (value != "" && ip.fromString(value))
                    DataManager.settings.wifi_subnet = ip;
                else
                    DataManager.settings.wifi_subnet = 0;
            }
            else if (name == "wifi_dns")
            {
                IPAddress ip;
                if (value != "" && ip.fromString(value))
                    DataManager.settings.wifi_dns = ip;
                else
                    DataManager.settings.wifi_dns = 0;
            }
            // reset
            else if (name == "factory_reset")
            {
                resetSettings(DataManager.settings);
            }
        }

        // update
        if (updated)
        {
            updated = false;
            String response;
            if (Update.hasError())
            {
                StreamString str;
                Update.printError(str);
                response = SF("Update error: ") + str.c_str();
            }
            else
                response = SF("Update Success");

            server.client().setNoDelay(true);
            response = SF("<META http-equiv=\"refresh\" content=\"15;URL=/\">") + response + SF("! Rebooting...\n");
            server.send(200, "text/html", response);
            delay(100);
            server.client().stop();
            ESP.restart();
        }
        else
        {
            // TODO: save settings (it'll break minute buffer saving, so save it too)
            server.sendHeader("Location", server.header("Referer"), true);
            server.send(302, "text/plain", "");
        }

        digitalWrite(ledPin, HIGH);
    }

    void handleUpdate()
    {
        digitalWrite(ledPin, LOW);
        // from ESP8266HTTPUpdateServer
        // handler for the file upload, get's the sketch bytes, and writes
        // them through the Update object
        HTTPUpload &upload = server.upload();
        int command = U_FLASH;
        if (upload.name == "update_spiffs")
            command = U_SPIFFS;

        if (upload.status == UPLOAD_FILE_START)
        {
            WiFiUDP::stopAll();
            DEBUGLOG("WebHandler", "Update: %s", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace, command)) //start with max available size
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
        }
        else if (upload.status == UPLOAD_FILE_WRITE && !Update.hasError())
        {
            DEBUGLOG("WebHandler", ".");
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
        }
        else if (upload.status == UPLOAD_FILE_END && !Update.hasError())
        {
            if (Update.end(true)) //true to set the size to the current progress
            {
                DEBUGLOG("WebHandler", "Update Success: %u\nRebooting...\n", upload.totalSize);
            }
            else
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
            updated = true;
        }
        else if (upload.status == UPLOAD_FILE_ABORTED)
        {
            Update.end();
            DEBUGLOG("WebHandler", "Update was aborted");
            updated = true;
        }
        delay(0);

        digitalWrite(ledPin, HIGH);
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
        // TODO: maybe add AP info too

        result += "Local Time: ";
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
        result += SF("Password: ") + String(DataManager.settings.password) + SF(", ");
        result += SF("Coefficient: ") + String(DataManager.settings.coefficient) + SF(", ");
        result += SF("WiFi SSID: '") + String(DataManager.settings.wifi_ssid) + SF("', ");
        result += SF("WiFi Passphrase: '") + String(DataManager.settings.wifi_passphrase) + SF("', ");
        result += SF("WiFi IP: ") + String(IPAddress(DataManager.settings.wifi_ip).toString()) + SF(", ");
        result += SF("WiFi Gateway: ") + String(IPAddress(DataManager.settings.wifi_gateway).toString()) + SF(", ");
        result += SF("WiFi Subnet: ") + String(IPAddress(DataManager.settings.wifi_subnet).toString()) + SF(", ");
        result += SF("WiFi DNS: ") + String(IPAddress(DataManager.settings.wifi_dns).toString()) + SF(", ");
        result += SF("Settings size: ") + String(sizeof(DataManager.settings)) + " + " + String(60 * MONITORS_COUNT * sizeof(uint32_t));
        result += SF(" = ") + String(sizeof(DataManager.settings) + 60 * MONITORS_COUNT * sizeof(uint32_t));
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
        result += String(result.length());

        server.send(200, "text/html", result);
        digitalWrite(2, HIGH);
    }

    inline String toString(const uint32_t &value)
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
        server.send(200, "text/html", F("<META http-equiv=\"refresh\" content=\"5;URL=/\">Rebooting...\n"));
        delay(100);
        server.client().stop();
        ESP.restart();
    }
};

#endif