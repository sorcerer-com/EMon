#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <ESP8266WebServer.h>
#include <FS.h>
#include <StreamString.h>
#include <Hash.h>

#include "src/NTPClient.h"

class WebHandler
{
private:
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
        SPIFFS.begin();

        server.on("/login", HTTP_GET, [&]() { handleLogin(HTTP_GET); });
        server.on("/login", HTTP_POST, [&]() { handleLogin(HTTP_POST); });
        server.on("/data.js", HTTP_GET, [&]() { handleDataFile(); });
        server.on("/settings", HTTP_POST, [&]() { handleSettings(); }, [&]() { handleUpdate(); });
        server.on("/raw", HTTP_GET, [&]() { handleRaw(); });
        server.on("/data", HTTP_GET, [&]() { handleRawData(); });
        server.on("/restart", HTTP_GET, [&]() { handleRestart(); });

        server.onNotFound([&]() {
            if (!handleFileRead(server.uri()))
            {
                server.send(404, "text/plain", F("404: Not Found"));
            }
        });
    }

    void handleLogin(const HTTPMethod &method) const
    {
        digitalWrite(LED_BUILTIN, LOW);
        if (method == HTTP_GET)
        {
            if (authenticate(false))
                server.send(200, "text/html", "");
            else
                server.send(401, "text/html", "");
        }
        else if (method == HTTP_POST)
        {
            if (server.arg("password") == DataManager.data.settings.password)
            {
                DEBUGLOG("WebHandler", "Login");
                // set cookie with hash of remoteIp and password with max age 20 min
                String hash = sha1(server.client().remoteIP().toString() + DataManager.data.settings.password);
                server.sendHeader("Set-Cookie", hash + ";Max-Age=1200;path=./");

                server.sendHeader("Location", server.arg("redirect"), true);
                server.send(302, "text/plain", "");
            }
            else
            {
                DEBUGLOG("WebHandler", "Wrong password");
                server.sendHeader("Set-Cookie", "login fail;Max-Age=3;path=./");
                server.sendHeader("Location", "./", true);
                server.send(302, "text/plain", "");
            }
        }
        digitalWrite(LED_BUILTIN, HIGH);
    }

    bool authenticate(const bool &redirect = true) const
    {
        if (strcmp(DataManager.data.settings.password, "") == 0) // empty password
            return true;

        bool res = false;
        if (server.hasHeader("Cookie"))
        {
            String hash = sha1(server.client().remoteIP().toString() + DataManager.data.settings.password);
            res = (server.header("Cookie") == hash);
        }

        if (!res && redirect)
        {
            server.sendHeader("Location", "./", true);
            server.send(302, "text/plain", "");
        }
        return res;
    }

    void handleDataFile() const
    {
        if (!authenticate())
            return;

        digitalWrite(LED_BUILTIN, LOW);

        unsigned long timer = millis();
        String result = SF("var data = {};\n");
        result += SF("data.monitorsCount = ") + String(MONITORS_COUNT) + SF(";\n");
        result += SF("data.tariffsCount = ") + String(TARIFFS_COUNT) + SF(";\n");

        date_time dt = DataManager.getCurrentTime();
        result += SF("data.time = '") + dateTimeToString(dt, true) + SF("';\n");
        result += SF("data.startTime = ") + String(DataManager.data.startTime) + SF(";\n");
        result += SF("\n");

        result += SF("data.settings = {};\n");
        result += SF("data.settings.timeZone = ") + String(DataManager.data.settings.timeZone) + SF(";\n");
        result += SF("data.settings.tariffStartHours = [];\n");
        result += SF("data.settings.tariffPrices = [];\n");
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            result += SF("data.settings.tariffStartHours[") + String(t) + SF("] = ");
            result += String(DataManager.data.settings.tariffStartHours[t]) + SF(";\n");
            result += SF("data.settings.tariffPrices[") + String(t) + SF("] = ");
            result += String(DataManager.data.settings.tariffPrices[t], 5) + SF(";\n");
        }
        result += SF("data.settings.billDay = ") + String(DataManager.data.settings.billDay) + SF(";\n");
        result += SF("data.settings.currencySymbols = '") + String(DataManager.data.settings.currencySymbols) + SF("';\n");
        result += SF("data.settings.monitorsNames = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("data.settings.monitorsNames[") + String(m) + SF("] = '");
            result += String(DataManager.data.settings.monitorsNames[m]) + SF("';\n");
        }
        result += SF("data.settings.coefficient = ") + String(DataManager.data.settings.coefficient) + SF(";\n");
        result += SF("data.settings.wifi_ssid = '") + String(DataManager.data.settings.wifi_ssid) + SF("';\n");
        result += SF("data.settings.wifi_passphrase = '") + String(DataManager.data.settings.wifi_passphrase) + SF("';\n");
        result += SF("data.settings.wifi_ip = '") + String(IPAddress(DataManager.data.settings.wifi_ip).toString()) + SF("';\n");
        result += SF("data.settings.wifi_gateway = '") + String(IPAddress(DataManager.data.settings.wifi_gateway).toString()) + SF("';\n");
        result += SF("data.settings.wifi_subnet = '") + String(IPAddress(DataManager.data.settings.wifi_subnet).toString()) + SF("';\n");
        result += SF("data.settings.wifi_dns = '") + String(IPAddress(DataManager.data.settings.wifi_dns).toString()) + SF("';\n");
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
                result += String(DataManager.data.hours[h][m]);
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
                    result += String(DataManager.data.days[d][t][m]);
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
                    result += String(DataManager.data.months[i][t][m]);
                    if (i < 11)
                        result += SF(", ");
                }
                result += SF("];\n");
            }
        }
        result += SF("// ") + String(millis() - timer) + SF(", ") + String(result.length());
        DEBUGLOG("WebHandler", "Generating data.js (%d bytes) for %d millisec", result.length(), (millis() - timer));

        server.send(200, "application/javascript", result);
        digitalWrite(LED_BUILTIN, HIGH);
    }

    bool handleFileRead(String path) const
    {
        digitalWrite(LED_BUILTIN, LOW);

        DEBUGLOG("WebHandler", "HandleFileRead: %s", path.c_str());
        if (path.endsWith("/"))
            path += "index.html";
        // if not authenticated and it's html file and it's not index.html
        if (path.endsWith(".html") && !path.endsWith("index.html") &&
            !authenticate())
            return true;
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

            digitalWrite(LED_BUILTIN, HIGH);
            return true;
        }
        DEBUGLOG("WebHandler", "\tFile Not Found: %s", path.c_str());

        digitalWrite(LED_BUILTIN, HIGH);
        return false; // If the file doesn't exist, return false
    }

    inline String getContentType(const String &filename) const
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

    void handleSettings()
    {
        if (!authenticate())
            return;

        digitalWrite(LED_BUILTIN, LOW);

        bool restart = false;
        int listParamIdx = 0;
        for (int i = 0; i < server.args(); i++)
        {
            const String &name = server.argName(i);
            const String &value = server.arg(i);
            DEBUGLOG("WebHandler", "Set setting %s: %s", name.c_str(), value.c_str());

            if (i > 0 && name != server.argName(i - 1))
                listParamIdx = 0;

            // basic settings
            if (name == "timeZone")
                DataManager.data.settings.timeZone = value.toInt();
            else if (name == "tariffStartHours[]")
            {
                DataManager.data.settings.tariffStartHours[listParamIdx] = value.toInt();
                listParamIdx++;
            }
            else if (name == "tariffPrices[]")
            {
                DataManager.data.settings.tariffPrices[listParamIdx] = value.toFloat();
                listParamIdx++;
            }
            else if (name == "billDay")
                DataManager.data.settings.billDay = value.toInt();
            else if (name == "currencySymbols")
                strcpy(DataManager.data.settings.currencySymbols, value.c_str());
            else if (name == "monitorsNames[]")
            {
                strcpy(DataManager.data.settings.monitorsNames[listParamIdx], value.c_str());
                listParamIdx++;
            }
            // advanced settings
            else if (name == "password" && value != "*****")
                strcpy(DataManager.data.settings.password, value.c_str());
            else if (name == "coefficient")
                DataManager.data.settings.coefficient = value.toFloat();
            // WiFi settings
            else if (name == "wifi_ssid")
            {
                if (strcmp(DataManager.data.settings.wifi_ssid, value.c_str()) != 0)
                    restart = true;
                strcpy(DataManager.data.settings.wifi_ssid, value.c_str());
            }
            else if (name == "wifi_passphrase")
            {
                if (strcmp(DataManager.data.settings.wifi_passphrase, value.c_str()) != 0)
                    restart = true;
                strcpy(DataManager.data.settings.wifi_passphrase, value.c_str());
            }
            else if (name == "wifi_ip")
            {
                IPAddress ip = 0;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_ip != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_ip = ip;
            }
            else if (name == "wifi_gateway")
            {
                IPAddress ip = 0;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_gateway != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_gateway = ip;
            }
            else if (name == "wifi_subnet")
            {
                IPAddress ip = 0;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_subnet != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_subnet = ip;
            }
            else if (name == "wifi_dns")
            {
                IPAddress ip = 0;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_dns != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_dns = ip;
            }
            // update time
            else if (name == "time")
            {
                DataManager.setStartTime(value.toInt());
            }
            // reset
            else if (name == "factory_reset")
            {
                DataManager.data.reset();
                DataManager.data.writeEEPROM(true);
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
            DEBUGLOG("WebHandler", response.c_str());

            server.client().setNoDelay(true);
            response = SF("<META http-equiv=\"refresh\" content=\"15;URL=./\">") + response + SF("! Rebooting...\n");
            server.send(200, "text/html", response);
            delay(100);
            server.client().stop();
            ESP.restart();
        }
        else
        {
            DataManager.data.writeEEPROM(true);
            if (restart)
            {
                server.client().setNoDelay(true);
                server.send(200, "text/html", SF("<META http-equiv=\"refresh\" content=\"5;URL=./\">Rebooting...\n"));
                delay(100);
                server.client().stop();
                ESP.restart();
            }
            else
            {
                server.sendHeader("Location", server.header("Referer"), true);
                server.send(302, "text/plain", "");
            }
        }

        digitalWrite(LED_BUILTIN, HIGH);
    }

    void handleUpdate()
    {
        if (!authenticate())
            return;

        digitalWrite(LED_BUILTIN, LOW);
        // from ESP8266HTTPUpdateServer
        // handler for the file upload, get's the sketch bytes, and writes
        // them through the Update object
        HTTPUpload &upload = server.upload();
        int command = U_FLASH;
        if (upload.name == "update_spiffs")
            command = U_FS;

        if (upload.status == UPLOAD_FILE_START)
        {
            WiFiUDP::stopAll();
            DEBUGLOG("WebHandler", "Update: %s", upload.filename.c_str());
            uint32_t maxSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (command == U_FS)
            {
                maxSpace = ((size_t)&_FS_end - (size_t)&_FS_start);
                close_all_fs();
            }
            if (!Update.begin(maxSpace, command)) //start with max available size
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
        }
        else if (upload.status == UPLOAD_FILE_WRITE && !Update.hasError())
        {
            DEBUGLOG("WebHandler", "Processing");
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
                DEBUGLOG("WebHandler", "Update Success: %u", upload.totalSize);
            }
            else
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
            if (upload.totalSize > 0)
                updated = true;
        }
        else if (upload.status == UPLOAD_FILE_ABORTED)
        {
            Update.end();
            DEBUGLOG("WebHandler", "Update was aborted");
            if (upload.totalSize > 0)
                updated = true;
        }
        delay(0);

        digitalWrite(LED_BUILTIN, HIGH);
    }

    void handleRaw() const
    {
        if (!authenticate())
            return;

        digitalWrite(LED_BUILTIN, LOW);

        FSInfo fs_info;
        SPIFFS.info(fs_info);

        date_time dt = DataManager.getCurrentTime();
        uint32_t values[TARIFFS_COUNT];

        int month = dt.Month;
        if (dt.Day < DataManager.data.settings.billDay)
            month--;
        if (month < 1)
            month += 12;

        String result = SF("WiFi Mode: ") + (WiFi.getMode() == 0 ? SF("WIFI_OFF") : (WiFi.getMode() == 1 ? SF("WIFI_STA") : (WiFi.getMode() == 2 ? SF("WIFI_AP") : SF("WIFI_AP_STA")))) + SF("; ");
        result += SF("WiFi: ") + WiFi.SSID() + SF(", ") + WiFi.localIP().toString() + SF(", ") + String(WiFi.RSSI()) + SF("; ");
        result += SF("WiFi AP: ") + WiFi.softAPSSID() + SF(", ") + WiFi.softAPIP().toString() + SF(", ") + String(WiFi.softAPgetStationNum());
        result += SF("<br/>\n");

        result += "Local Time: ";
        result += dateTimeToString(dt);
        result += SF(" (millis: ") + String(millis()) + SF(")");
        result += SF("<br/>\n");

        result += SF("StartTime: ") + String(DataManager.data.startTime) + SF(", ");
        result += SF("LastSaveHour: ") + String(DataManager.data.lastSaveHour) + SF(", ");
        result += SF("LastSaveDay: ") + String(DataManager.data.lastSaveDay) + SF(", ");
        result += SF("LastSaveMonth: ") + String(DataManager.data.lastSaveMonth) + SF(", ");
        result += SF("Tariffs: ");
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            result += String(DataManager.data.settings.tariffStartHours[t]);
            result += SF(" (") + String(DataManager.data.settings.tariffPrices[t]) + SF("), ");
        }
        result += SF("BillDay: ") + String(DataManager.data.settings.billDay) + SF(", ");
        result += SF("CurrSymbols: '") + String(DataManager.data.settings.currencySymbols) + SF("', ");
        result += SF("Monitors: ");
        for (int m = 0; m < MONITORS_COUNT; m++)
            result += String(DataManager.data.settings.monitorsNames[m]) + SF("; ");
        result += SF("Password: ") + String(DataManager.data.settings.password) + SF(", ");
        result += SF("Coefficient: ") + String(DataManager.data.settings.coefficient) + SF(", ");
        result += SF("WiFi SSID: '") + String(DataManager.data.settings.wifi_ssid) + SF("', ");
        result += SF("WiFi Passphrase: '") + String(DataManager.data.settings.wifi_passphrase) + SF("', ");
        result += SF("WiFi IP: ") + String(IPAddress(DataManager.data.settings.wifi_ip).toString()) + SF(", ");
        result += SF("WiFi Gateway: ") + String(IPAddress(DataManager.data.settings.wifi_gateway).toString()) + SF(", ");
        result += SF("WiFi Subnet: ") + String(IPAddress(DataManager.data.settings.wifi_subnet).toString()) + SF(", ");
        result += SF("WiFi DNS: ") + String(IPAddress(DataManager.data.settings.wifi_dns).toString()) + SF(", ");
        result += SF("Data size: ") + String(sizeof(DataManager.data));
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
                    result += toString(DataManager.data.hours[i][m]) + SF(" ");
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
                        result += toString(DataManager.data.days[i][t][m]) + SF(" ");
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
                        result += toString(DataManager.data.months[i][t][m]) + SF(" ");
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

    inline String toString(const uint32_t &value) const
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

    void handleRawData() const
    {
        if (!authenticate())
            return;

        digitalWrite(LED_BUILTIN, LOW);

        String result = "[\n";
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            uint32_t values[TARIFFS_COUNT];
            DataManager.getCurrentMonthEnergy(m, values);
            uint32_t sum = 0;
            for (int t = 0; t < TARIFFS_COUNT; t++)
                sum += values[t];

            // power usage
            result += "\t{\n";
            result += "\t\t \"name\": \"PowerUsage" + String(m + 1) + "\",\n";
            result += "\t\t \"value\": " + String(DataManager.getEnergy(m)) + ",\n";
            result += "\t\t \"aggrType\": \"avg\",\n";
            result += "\t\t \"desc\": \"Power usage for: " + String(DataManager.data.settings.monitorsNames[m]) + "\"\n";
            result += "\t},\n";

            // consumed power
            result += "\t{\n";
            result += "\t\t \"name\": \"ConsumedPower" + String(m + 1) + "\",\n";
            result += "\t\t \"value\": " + String(sum * 0.01) + ",\n";
            result += "\t\t \"aggrType\": \"sum\",\n";
            result += "\t\t \"desc\": \"Consumed power for: " + String(DataManager.data.settings.monitorsNames[m]) + "\"\n";
            result += "\t}";
            if (m < MONITORS_COUNT - 1)
                result += ",";
            result += "\n";
        }
        result += "]";

        server.send(200, "text/plain", result);
        digitalWrite(LED_BUILTIN, HIGH);
    }

    void handleRestart() const
    {
        if (!authenticate())
            return;

        DEBUGLOG("WebHandler", "Restart");
        server.client().setNoDelay(true);
        server.send(200, "text/html", F("<META http-equiv=\"refresh\" content=\"5;URL=./\">Rebooting...\n"));
        delay(100);
        server.client().stop();
        ESP.restart();
    }
};

#endif