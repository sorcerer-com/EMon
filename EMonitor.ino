#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>

#include "Settings.h"
#include "DataManager.h"

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

// TODO: restart every day
String toString(const uint32_t &value);

void handleRoot()
{
  digitalWrite(2, LOW);
  String message = "Energy Monitor (" + WiFi.SSID() + ") 4\n";
  date_time dt = DataManager.getCurrentTime();
  message += "<br/>";
  message += String(dt.Hour) + ":" + String(dt.Minute) + ":" + String(dt.Second) + " " +
             String(dt.Day) + "/" + String(dt.Month) + "/" + String(dt.Year) + "\n";
  message += " " + String(DataManager.settings.tariffHours[0]) + " " +
             String(DataManager.settings.tariffHours[1]) + " " +
             String(DataManager.settings.tariffHours[2]) + "\n";
  message += " " + String(DataManager.settings.billDay) + "\n";
  message += "<br/>";
  for (int m = 0; m < MONITORS_COUNT; m++)
  {
    double current = DataManager.getCurrent(m);
    message += "Current: " + String(current) + " A, ";
    message += "Energy: " + String(current * 230) + " W ";
    message += "<br/>";
    message += "\n";
  }
  message += "<br/>";

  message += "<table style='width:100%'>";
  message += "\n\t\t<tr><td></td>";
  for (int i = 0; i < 31; i++)
  {
    message += "<td>";
    message += String(i) + " ";
    message += "</td>";
  }
  message += "</tr>";

  message += "\n";
  message += "<tr><td>";
  message += "Hours:\n";
  message += "</td></tr>";
  for (int m = 0; m < MONITORS_COUNT; m++)
  {
    message += "<tr><td>";
    message += "\tMonitor " + String(m) + ": ";
    message += "</td>";
    for (int i = 0; i < 24; i++)
    {
      message += "<td>";
      message += toString(DataManager.settings.hours[i][m]) + " ";
      message += "</td>";
    }
    message += "</tr>";
    message += "\n";
  }

  message += "\n";
  message += "<tr><td>";
  message += "Days:\n";
  message += "</td></tr>";
  for (int m = 0; m < MONITORS_COUNT; m++)
  {
    message += "<tr><td>";
    message += "\tMonitor " + String(m) + "\n";
    message += "</td></tr>";
    for (int t = 0; t < TARIFFS_COUNT; t++)
    {
      message += "<tr>";
      message += "<td>";
      message += "\t\tTariff " + String(t) + ": ";
      message += "</td>";
      for (int i = 0; i < 31; i++)
      {
        message += "<td>";
        message += toString(DataManager.settings.days[i][t][m]) + " ";
        message += "</td>";
      }
      message += "</tr>";
      message += "\n";
    }
  }

  message += "\n";
  message += "<tr><td>";
  message += "Months:\n";
  message += "</td></tr>";
  for (int m = 0; m < MONITORS_COUNT; m++)
  {
    message += "<tr><td>";
    message += "\tMonitor " + String(m) + "\n";
    message += "</td></tr>";
    for (int t = 0; t < TARIFFS_COUNT; t++)
    {
      message += "<tr>";
      message += "<td>";
      message += "\t\tTariff " + String(t) + ": ";
      message += "</td>";
      for (int i = 0; i < 12; i++)
      {
        message += "<td>";
        message += toString(DataManager.settings.months[i][t][m]) + " ";
        message += "</td>";
      }
      message += "</tr>";
      message += "\n";
    }
  }
  message += "</table>";

  server.send(200, "text/html", message);
  digitalWrite(2, HIGH);
}

void setup()
{
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("m68", "bekonche");
  wifiMulti.addAP("WL0242_2.4G_13A608", "bekonche");

  // Wait for connection
  int count = 0;
  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print(WiFi.SSID());
  Serial.print(" ");
  Serial.println(WiFi.localIP());
  Serial.println();

  DataManager.setup();

  server.on("/", handleRoot);
  server.on("/restart", []() { ESP.restart(); });
  server.on("/save", []() { writeEEPROM(DataManager.settings); });

  httpUpdater.setup(&server);
  server.begin();
}

void loop()
{
  DataManager.update();

  server.handleClient();
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
  str += String(value % 100);
  return str;
}