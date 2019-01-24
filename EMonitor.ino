#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
//#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <FS.h>

#include "Settings.h"
#include "DataManager.h"
#include "WebHandler.h"
// TODO:
#include "ESP8266HTTPUpdateServer2.h"

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer2 httpUpdater;
WebHandler webHandler(server);

// TODO: restart every day
void setup()
{
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("m68", "bekonche");
  wifiMulti.addAP("WL0242_2.4G_13A608", "bekonche");

  // Wait for connection
  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print(WiFi.SSID());
  Serial.print(" ");
  Serial.println(WiFi.localIP().toString());
  Serial.println();

  DataManager.setup();

  ArduinoOTA.begin();

  httpUpdater.setup(&server);
  server.begin();

  SPIFFS.begin();
}

void loop()
{
  DataManager.update();

  ArduinoOTA.handle();
  server.handleClient();

  if (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.println("Re-connecting...");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.localIP().toString());
    Serial.println();
  }
}