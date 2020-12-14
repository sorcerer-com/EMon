#ifndef WEB_HANDLER_H
#define WEB_HANDLER_H

#include <FS.h>
#include <StreamString.h>
#include <mbedtls/sha1.h>
#include <SPIFFS.h>
#include <Update.h>

#include "src/ESPAsyncWebServer/AsyncJson.h"
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
        server.on("/data.json", HTTP_GET, [&](AsyncWebServerRequest *request) { handleDataFile(request); });
        server.on("/settings", HTTP_POST, [&](AsyncWebServerRequest *request) { handleSettings(request); },
            [&](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) { handleUpdate(request, filename, index, data, len, final); });
        server.on("/raw", HTTP_GET, [&](AsyncWebServerRequest *request) { handleRaw(request); });
        server.on("/data", HTTP_GET, [&](AsyncWebServerRequest *request) { handleRawData(request); });
        server.on("/restart", HTTP_GET, [&](AsyncWebServerRequest *request) { handleRestart(request); });

        AsyncCallbackJsonWebHandler* importHandler = new AsyncCallbackJsonWebHandler("/import", [&](AsyncWebServerRequest *request, JsonVariant &json) { handleImport(request, json); }, 16384);
        importHandler->setMethod(HTTP_POST);
        server.addHandler(importHandler);

        server.onNotFound([&](AsyncWebServerRequest *request) { request->send(404, "text/plain", F("404: Not Found")); });
    }

private:
    void handleLogin(AsyncWebServerRequest *request) const
    {
        digitalWrite(ledPin, LOW);
        if (request->method() == HTTP_GET)
        {
            if (authenticate(request, false))
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
                String hash = sha1(request->client()->remoteIP().toString() + DataManager.data.settings.password);
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
            String hash = sha1(request->client()->remoteIP().toString() + DataManager.data.settings.password);
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

        AsyncJsonResponse *response = new AsyncJsonResponse(false, 16384);

        unsigned long timer = millis();
        JsonObject root = response->getRoot();
        root["version"] = VERSION;
        root["monitorsCount"] = MONITORS_COUNT;
        root["tariffsCount"] = TARIFFS_COUNT;

        date_time dt = DataManager.getCurrentTime();
        root["time"] = dateTimeToString(dt, true);
        root["startTime"] = DataManager.data.base.startTime;

        // settings
        root["settings"]["timeZone"] = DataManager.data.settings.timeZone;
        root["settings"]["dst"] = DataManager.data.settings.dst;
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            root["settings"]["tariffStartHours"][t] = DataManager.data.settings.tariffStartHours[t];
            root["settings"]["tariffPrices"][t] = DataManager.data.settings.tariffPrices[t];
        }
        root["settings"]["billDay"] = DataManager.data.settings.billDay;
        root["settings"]["currencySymbols"] = DataManager.data.settings.currencySymbols;
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            root["settings"]["monitorsNames"][m] = DataManager.data.settings.monitorsNames[m];
        }
        root["settings"]["coefficient"] = DataManager.data.settings.coefficient;
        root["settings"]["wifi_ssid"] = DataManager.data.settings.wifi_ssid;
        root["settings"]["wifi_passphrase"] = DataManager.data.settings.wifi_passphrase;
        root["settings"]["wifi_ip"] = DataManager.data.settings.wifi_ip;
        root["settings"]["wifi_gateway"] = DataManager.data.settings.wifi_gateway;
        root["settings"]["wifi_subnet"] = DataManager.data.settings.wifi_subnet;
        root["settings"]["wifi_dns"] = DataManager.data.settings.wifi_dns;

        // current data
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            root["current"]["energy"][m] = DataManager.getEnergy(m);
            root["current"]["hour"][m] = DataManager.getCurrentHourEnergy(m);

            uint32_t values[TARIFFS_COUNT];
            DataManager.getCurrentDayEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                root["current"]["day"][m][t] = values[t];
            }

            DataManager.getCurrentMonthEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                root["current"]["month"][m][t] = values[t];
            }
        }

        root["current"]["voltage"] = DataManager.getVoltage();

        // last 24 hours
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            for (int h = 0; h < 24; h++)
            {
                root["hours"][m][h] = DataManager.data.base.hours[h][m];
            }
        }

        // last 31 days
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                for (int d = 0; d < 31; d++)
                {
                    root["days"][m][t][d] = DataManager.data.base.days[d][t][m];
                }
            }
        }

        // last 12 months
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                for (int i = 0; i < 12; i++)
                {
                    root["months"][m][t][i] = DataManager.data.base.months[i][t][m];
                }
            }
        }

        size_t len = response->setLength();
        DEBUGLOG("WebHandler", "Generating data.json (%d bytes) for %d millisec", len, (millis() - timer));

        request->send(response);
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
            else if (name == "dst")
                DataManager.data.settings.dst = (value == "true");
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
            response = SF("<META http-equiv=\"refresh\" content=\"5;URL=./\">") + response + SF("! Rebooting...\n");
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

    void handleImport(AsyncWebServerRequest *request, JsonVariant &json)
    {
        if (!authenticate(request))
            return;

        digitalWrite(ledPin, LOW);

        JsonObject jsonObj = json.as<JsonObject>();
        for (const JsonPair kv : jsonObj)
        {
            DEBUGLOG("WebHandler", "Importing data for '%s'", kv.key().c_str());
            int m = 0;
            for (const JsonVariant monitorData : kv.value().as<JsonArray>())
            {
                if (kv.key() == "hours")
                {
                    int h = 0;
                    for (const JsonVariant hourData : monitorData.as<JsonArray>())
                    {
                        DataManager.data.base.hours[h][m] = hourData.as<uint32_t>();
                        h++;
                    }
                }
                else
                {
                    int t = 0;
                    for (const JsonVariant tariffData : monitorData.as<JsonArray>())
                    {
                        if (kv.key() == "days")
                        {
                            int d = 0;
                            for (const JsonVariant dayData : tariffData.as<JsonArray>())
                            {
                                DataManager.data.base.days[d][t][m] = dayData.as<uint32_t>();
                                d++;
                            }
                        }
                        else // months
                        {
                            int i = 0;
                            for (const JsonVariant monthData : tariffData.as<JsonArray>())
                            {
                                DataManager.data.base.months[i][t][m] = monthData.as<uint32_t>();
                                i++;
                            }
                        }
                        t++;
                    }
                }
                m++;
            }
        }
        DataManager.data.save(Data::SaveFlags::Base);

        request->send(200, "text/html", "");
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
        result += SF("DST: ") + String(DataManager.data.settings.dst) + SF(", ");
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
            result += SF("'") + String(DataManager.data.settings.monitorsNames[m]) + SF("'; ");
        result += SF("Password: '") + String(DataManager.data.settings.password) + SF("', ");
        result += SF("Coefficient: ") + String(DataManager.data.settings.coefficient) + SF(", ");
        result += SF("WiFi SSID: '") + String(DataManager.data.settings.wifi_ssid) + SF("', ");
        result += SF("WiFi Passphrase: '") + String(DataManager.data.settings.wifi_passphrase) + SF("', ");
        result += SF("WiFi IP: ") + String(IPAddress(DataManager.data.settings.wifi_ip).toString()) + SF(", ");
        result += SF("WiFi Gateway: ") + String(IPAddress(DataManager.data.settings.wifi_gateway).toString()) + SF(", ");
        result += SF("WiFi Subnet: ") + String(IPAddress(DataManager.data.settings.wifi_subnet).toString()) + SF(", ");
        result += SF("WiFi DNS: ") + String(IPAddress(DataManager.data.settings.wifi_dns).toString()) + SF(", ");
        result += SF("Data size: ") + String(sizeof(DataManager.data)) + SF(" (") + String(sizeof(DataManager.data.base)) + SF("/") + String(sizeof(DataManager.data.minutes)) + SF("/") + String(sizeof(DataManager.data.settings)) + SF(")");
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

        // voltage
        result += "\t{\n";
        result += "\t\t \"name\": \"Voltage\",\n";
        result += "\t\t \"value\": " + String(DataManager.getVoltage()) + ",\n";
        result += "\t\t \"aggrType\": \"avg\",\n";
        result += "\t\t \"desc\": \"Voltage value\"\n";
        result += "\t},\n";

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

    static String sha1(const String &payload)
    {
        mbedtls_sha1_context ctx;
        uint8_t hash[20];

        mbedtls_sha1_init(&ctx);
        mbedtls_sha1_starts(&ctx);
        mbedtls_sha1_update(&ctx, (const unsigned char *)payload.c_str(), payload.length());
        mbedtls_sha1_finish(&ctx, hash);
        mbedtls_sha1_free(&ctx);

        String hashStr((const char *)nullptr);
        hashStr.reserve(20 * 2 + 1);
        for (uint16_t i = 0; i < 20; i++)
        {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", hash[i]);
            hashStr += hex;
        }
        return hashStr;
    }
};

#endif