#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <FS.h>
#include <StreamString.h>
// TODO: missing in ESP32
//#include <Hash.h>
#include<SPIFFS.h>
#include<Update.h>

#include "src/ESPAsyncWebServer/ESPAsyncWebServer.h"
#include "src/NTPClient.h"

class WebHandler
{
private:
    AsyncWebServer &server;
    const uint8_t ledPin;
    bool updated = false;

public:
    WebHandler(AsyncWebServer &server, const uint8_t &ledPin) : server(server), ledPin(ledPin)
    {
    }

    void setup()
    {
        server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setCacheControl("max-age=86400");

        server.on("/login", HTTP_GET, [&](AsyncWebServerRequest *request) { handleLogin(request); });
        server.on("/login", HTTP_POST, [&](AsyncWebServerRequest *request) { handleLogin(request); });
        server.on("/data.js", HTTP_GET, [&](AsyncWebServerRequest *request) { handleDataFile(request); });
        server.on("/settings", HTTP_POST, [&](AsyncWebServerRequest *request) { handleSettings(request); },
            [&](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) { handleUpdate(request, filename, index, data, len, final); });
        server.on("/raw", HTTP_GET, [&](AsyncWebServerRequest *request) { handleRaw(request); });
        server.on("/data", HTTP_GET, [&](AsyncWebServerRequest *request) { handleRawData(request); });
        server.on("/restart", HTTP_GET, [&](AsyncWebServerRequest *request) { handleRestart(request); });
    }

private:
    void handleLogin(AsyncWebServerRequest *request) const
    {
        digitalWrite(ledPin, LOW);
        if (request->method() == HTTP_GET)
        {
            if (authenticate(false))
                request->send(200, "text/html", "");
            else
                request->send(401, "text/html", "");
        }
        else if (request->method() == HTTP_POST)
        {
            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
            if (request->arg("password") == DataManager.data.settings.password)
            {
                DEBUGLOG("WebHandler", "Login");
                response->addHeader("Location", request->arg("redirect"));
                // set cookie with hash of remoteIp and password with max age 20 min
                String hash = ""; // TODO: sha1(server.client().remoteIP().toString() + DataManager.data.settings.password);
                response->addHeader("Set-Cookie", hash + ";Max-Age=1200;path=./");
            }
            else
            {
                DEBUGLOG("WebHandler", "Wrong password");
                response->addHeader("Location", "./");
                response->addHeader("Set-Cookie", "login fail;Max-Age=3;path=./");
            }
            request->send(response);
        }
        digitalWrite(ledPin, HIGH);
    }

    bool authenticate(AsyncWebServerRequest *request, const bool &redirect = true) const
    {
        if (strcmp(DataManager.data.settings.password, "") == 0) // empty password
            return true;

        bool res = false;
        if (request->hasHeader("Cookie"))
        {
            String hash = "";// TODO: sha1(server.client().remoteIP().toString() + DataManager.data.settings.password);
            res = (request->header("Cookie") == hash);
        }

        if (!res && redirect)
        {
            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
            response->addHeader("Location", "./");
            request->send(response);
        }
        return res;
    }

    void handleDataFile(AsyncWebServerRequest *request) const
    {
        if (!authenticate(request))
            return;

        digitalWrite(ledPin, LOW);

        unsigned long timer = millis();
        String result = SF("var data = {};\n");
        result += SF("data.version = '") + String(VERSION) + SF("';\n");
        result += SF("data.monitorsCount = ") + String(MONITORS_COUNT) + SF(";\n");
        result += SF("data.tariffsCount = ") + String(TARIFFS_COUNT) + SF(";\n");

        date_time dt = DataManager.getCurrentTime();
        result += SF("data.time = '") + dateTimeToString(dt, true) + SF("';\n");
        result += SF("data.startTime = ") + String(DataManager.data.base.startTime) + SF(";\n");
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

#ifdef VOLTAGE_MONITOR
        result += SF("data.current.voltage = ") + DataManager.getVoltage() + SF(";\n\n");
#endif

        result += SF("data.hours = [];\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("// Monitor ") + String(m) + SF("\n");
            result += SF("data.hours[") + String(m) + SF("] = [");
            for (int h = 0; h < 24; h++)
            {
                result += String(DataManager.data.base.hours[h][m]);
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
                    result += String(DataManager.data.base.days[d][t][m]);
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
                    result += String(DataManager.data.base.months[i][t][m]);
                    if (i < 11)
                        result += SF(", ");
                }
                result += SF("];\n");
            }
        }
        result += SF("// ") + String(millis() - timer) + SF(", ") + String(result.length());
        DEBUGLOG("WebHandler", "Generating data.js (%d bytes) for %d millisec", result.length(), (millis() - timer));

        request->send(200, "application/javascript", result);
        digitalWrite(ledPin, HIGH);
    }

    void handleSettings(AsyncWebServerRequest *request)
    {
        if (!authenticate(request))
            return;

        digitalWrite(ledPin, LOW);

        bool restart = false;
        int listParamIdx = 0;
        for (int i = 0; i < request->args(); i++)
        {
            const String &name = request->argName(i);
            const String &value = request->arg(i);
            DEBUGLOG("WebHandler", "Set setting %s: %s", name.c_str(), value.c_str());

            if (i > 0 && name != request->argName(i - 1))
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
                IPAddress ip = INADDR_NONE;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_ip != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_ip = ip;
            }
            else if (name == "wifi_gateway")
            {
                IPAddress ip = INADDR_NONE;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_gateway != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_gateway = ip;
            }
            else if (name == "wifi_subnet")
            {
                IPAddress ip = INADDR_NONE;
                ip.fromString(value);
                if (DataManager.data.settings.wifi_subnet != (uint32_t)ip)
                    restart = true;
                DataManager.data.settings.wifi_subnet = ip;
            }
            else if (name == "wifi_dns")
            {
                IPAddress ip = INADDR_NONE;
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
                DataManager.data.save(Data::SaveFlags::Base | Data::SaveFlags::Minutes | Data::SaveFlags::Settings);
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

            request->client()->setNoDelay(true);
            response = SF("<META http-equiv=\"refresh\" content=\"15;URL=./\">") + response + SF("! Rebooting...\n");
            request->send(200, "text/html", response);
            delay(100);
            request->client()->stop();
            ESP.restart();
        }
        else
        {
            DataManager.data.save(Data::SaveFlags::Settings);
            if (restart)
            {
                request->client()->setNoDelay(true);
                request->send(200, "text/html", SF("<META http-equiv=\"refresh\" content=\"5;URL=./\">Rebooting...\n"));
                delay(100);
                request->client()->stop();
                ESP.restart();
            }
            else
            {
                AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
                response->addHeader("Location", request->header("Referer"));
                request->send(response);
            }
        }

        digitalWrite(ledPin, HIGH);
    }

    void handleUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
    {
        if (!authenticate(request))
            return;

        digitalWrite(ledPin, LOW);
        // from ESP8266HTTPUpdateServer
        // handler for the file upload, get's the sketch bytes, and writes
        // them through the Update object
        int command = U_FLASH;
        if (filename.indexOf("spiffs") > -1) // if filename contains 'spiffs'
            command = U_SPIFFS;

        if (index == 0) // upload starts
        {
            DEBUGLOG("WebHandler", "Update: %s", filename.c_str());
            if (command == U_SPIFFS)
                SPIFFS.end();

            if (!Update.begin(UPDATE_SIZE_UNKNOWN, command, ledPin)) //start with max available size
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
        }
        if (!Update.hasError())
        {
            DEBUGLOG("WebHandler", "Processing");
            if (Update.write(data, len) != len)
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
        }
        if (final && !Update.hasError())
        {
            if (Update.end(true)) //true to set the size to the current progress
            {
                DEBUGLOG("WebHandler", "Update Success: %u", (index + len));
            }
            else
            {
#ifdef DEBUG
                Update.printError(Serial);
#endif
            }
            if (index + len > 0)
                updated = true;
        }
        delay(0);

        digitalWrite(ledPin, HIGH);
    }

    void handleRaw(AsyncWebServerRequest *request) const
    {
        if (!authenticate(request))
            return;

        digitalWrite(ledPin, LOW);

        date_time dt = DataManager.getCurrentTime();
        uint32_t values[TARIFFS_COUNT];

        int month = dt.Month;
        if (dt.Day < DataManager.data.settings.billDay)
            month--;
        if (month < 1)
            month += 12;

        // TODO: review data here:
        String result = SF("Version: ") + SF(VERSION) + SF("<br/>\n");
        result += SF("WiFi Mode: ") + (WiFi.getMode() == 0 ? SF("WIFI_OFF") : (WiFi.getMode() == 1 ? SF("WIFI_STA") : (WiFi.getMode() == 2 ? SF("WIFI_AP") : SF("WIFI_AP_STA")))) + SF("; ");
        result += SF("WiFi: ") + WiFi.SSID() + SF(", ") + WiFi.localIP().toString() + SF(", ") + String(WiFi.RSSI()) + SF("; ");
        result += SF("WiFi AP: ") + SF("EnergyMonitor_") + String((unsigned long)ESP.getEfuseMac(), 16) + SF(", ") + WiFi.softAPIP().toString() + SF(", ") + String(WiFi.softAPgetStationNum());
        result += SF("<br/>\n");

        result += "Local Time: ";
        result += dateTimeToString(dt);
        result += SF(" (millis: ") + String(millis()) + SF(")");
        result += SF("<br/>\n");

        result += SF("StartTime: ") + String(DataManager.data.base.startTime) + SF(", ");
        result += SF("lastSavedHour: ") + String(DataManager.data.base.lastSavedHour) + SF(", ");
        result += SF("lastSavedDay: ") + String(DataManager.data.base.lastSavedDay) + SF(", ");
        result += SF("lastSavedMonth: ") + String(DataManager.data.base.lastSavedMonth) + SF(", ");
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
        result += SF("Data size: ") + String(sizeof(DataManager.data);
        result += SF("<br/>\n");

        result += SF("SPIFFS: ") + String(SPIFFS.usedBytes()) + SF("/") + String(SPIFFS.totalBytes()) + SF(", ");
        result += SF("LITTLEFS: ") + String(LITTLEFS.usedBytes()) + SF("/") + String(LITTLEFS.totalBytes());
        result += SF("<br/>\n<br/>\n");
        result += SF("\n");

        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("Current: ") + String(DataManager.getCurrent(m)) + SF(" A, ");
            result += SF("Energy: ") + String(DataManager.getEnergy(m)) + SF(" W ");
            result += SF("<br/>\n");
        }
        result += SF("Voltage: ") + String(DataManager.getVoltage()) + SF(" V <br/>\n");
        result += SF("<br/>\n");
        result += SF("\n");

        result += SF("<table style='width:100%'>\n");
        result += SF("\t<tr><td></td>");
        for (int i = 0; i < 31; i++)
        {
            result += SF("<td>");
            result += String(i) + SF(" ");
            result += SF("</td>");
        }
        result += SF("</tr>\n");
        result += SF("\n");

        result += SF("<tr><td>");
        result += SF("Minutes:");
        result += SF("</td></tr>\n");
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += SF("\t<tr><td>");
            result += SF("Monitor ") + String(m) + SF(": ");
            result += SF("</td>");
            for (int i = 0; i < 30; i++)
            {
                result += SF("<td>");
                if (DataManager.data.minutes[m][i * 2] != 0xFFFFFFFF)
                    result += toString(DataManager.data.minutes[m][i * 2]);
                else
                    result += "X";
                result += "<br/>";
                if (DataManager.data.minutes[m][i * 2 + 1] != 0xFFFFFFFF)
                    result += toString(DataManager.data.minutes[m][i * 2 + 1]);
                else
                    result += "X";
                result += SF("</td>");
            }
            result += SF("</tr>\n");
        }
        result += SF("\n");

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
                    result += toString(DataManager.data.base.hours[i][m]) + SF(" ");
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
                        result += toString(DataManager.data.base.days[i][t][m]) + SF(" ");
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
                        result += toString(DataManager.data.base.months[i][t][m]) + SF(" ");
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

        request->send(200, "text/html", result);
        digitalWrite(ledPin, HIGH);
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

    void handleRawData(AsyncWebServerRequest *request) const
    {
        if (!authenticate(request))
            return;

        digitalWrite(ledPin, LOW);

        String result = "[\n";
#ifdef VOLTAGE_MONITOR
        // voltage
        result += "\t{\n";
        result += "\t\t \"name\": \"Voltage\",\n";
        result += "\t\t \"value\": " + String(DataManager.getVoltage()) + ",\n";
        result += "\t\t \"aggrType\": \"avg\",\n";
        result += "\t\t \"desc\": \"Voltage value\"\n";
        result += "\t},\n";
#endif
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

        request->send(200, "text/plain", result);
        digitalWrite(ledPin, HIGH);
    }

    void handleRestart(AsyncWebServerRequest *request) const
    {
        if (!authenticate(request))
            return;

        DEBUGLOG("WebHandler", "Restart");
        request->client()->setNoDelay(true);
        request->send(200, "text/html", F("<META http-equiv=\"refresh\" content=\"5;URL=./\">Rebooting...\n"));
        delay(100);
        request->client()->stop();
        ESP.restart();
    }
};

#endif