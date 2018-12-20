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
        server.on("/current.html", [&]() { handleFile("/current.html"); });
        server.on("/javascript.js", [&]() { handleFile("/javascript.js"); });
        server.on("/style3.css", [&]() { handleFile("/style3.css"); });

        server.on("/data.js", HTTP_GET, [&]() { handleDataFile(); });
    }

    void handleDataFile()
    {
        // TODO: maybe split in multiple files
        String result = "var data = {};\n";
        result += "data.monitorsCount = " + String(MONITORS_COUNT) + ";\n";
        result += "data.tariffsCount = " + String(TARIFFS_COUNT) + ";\n";
        result += "\n";

        result += "data.settings = {};\n";
        result += "data.settings['Time Zone'] = " + String(DataManager.settings.timeZone) + ";\n";
        result += "data.settings['Tariff Hours'] = [];\n";
        for (int t = 0; t < TARIFFS_COUNT; t++)
        {
            result += "data.settings['Tariff Hours'][" + String(t) + "] = ";
            result += String(DataManager.settings.tariffHours[t]) + ";\n";
        }
        result += "data.settings['Bill Day'] = " + String(DataManager.settings.billDay) + ";\n";
        result += "\n";

        result += "data.current = {};\n";
        result += "data.current.energy = [];\n";
        result += "data.current.hour = [];\n";
        result += "data.current.day = [];\n";
        result += "data.current.month = [];\n";
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += "data.current.energy[" + String(m) + "] = ";
            result += String(DataManager.getEnergy(m)) + ";\n";
            result += "data.current.hour[" + String(m) + "] = ";
            result += String(DataManager.getCurrentHourEnergy(m)) + ";\n";
            
            uint32_t values[TARIFFS_COUNT];
            result += "data.current.day[" + String(m) + "] = [];\n";
            DataManager.getCurrentDayEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += "data.current.day[" + String(m) + "][" + String(t) + "] = ";
                result += String(values[t]) + ";\n";
            }
            
            result += "data.current.month[" + String(m) + "] = [];\n";
            DataManager.getCurrentMonthEnergy(m, values);
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += "data.current.month[" + String(m) + "][" + String(t) + "] = ";
                result += String(values[t]) + ";\n";
            }
        }
        result += "\n";

        result += "data.hours = [];\n";
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += "// Monitor " + String(m) + "\n";
            result += "data.hours[" + String(m) + "] = [];\n";
            for (int h = 0; h < 24; h++)
            {
                result += "data.hours[" + String(m) + "][" + String(h) + "] = ";
                result += String(DataManager.settings.hours[h][m]) + ";\n";
            }
        }
        result += "\n";

        result += "data.days = [];\n";
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += "// Monitor " + String(m) + "\n";
            result += "data.days[" + String(m) + "] = [];\n";
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += "// Tariff " + String(t) + "\n";
                result += "data.days[" + String(m) + "][" + String(t) + "] = [];\n";
                for (int d = 0; d < 31; d++)
                {
                    result += "data.days[" + String(m) + "][" + String(t) + "][" + String(d) + "] = ";
                    result += String(DataManager.settings.days[d][t][m]) + ";\n";
                }
            }
        }
        result += "\n";

        result += "data.months = [];\n";
        for (int m = 0; m < MONITORS_COUNT; m++)
        {
            result += "// Monitor " + String(m) + "\n";
            result += "data.months[" + String(m) + "] = [];\n";
            for (int t = 0; t < TARIFFS_COUNT; t++)
            {
                result += "// Tariff " + String(t) + "\n";
                result += "data.months[" + String(m) + "][" + String(t) + "] = [];\n";
                for (int i = 0; i < 12; i++)
                {
                    result += "data.months[" + String(m) + "][" + String(t) + "][" + String(i) + "] = ";
                    result += String(DataManager.settings.months[i][t][m]) + ";\n";
                }
            }
        }

        server.send(200, "application/javascript", result);
    }

    void handleFile(String filename)
    {
        if (SPIFFS.exists(filename))
        {
            String contentType = getContentType(filename);
            File file = SPIFFS.open(filename, "r");             // Open it
            size_t sent = server.streamFile(file, contentType); // And send it to the client
            file.close();                                       // Then close the file again
        }
        else
            server.send(404, "text/plain", "404: Not Found");
    }

    String getContentType(String filename)
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
};

#endif