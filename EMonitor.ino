#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

// TODO:
#define REMOTE_DEBUG
#include "src/RemoteDebugger.h"
#include "DataManager.h"
#include "WebHandler.h"

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WebHandler webHandler(server);

// TODO: hardware reset way, if cannot connect to WiFi (or create AP), if forget the login password
// TODO: WiFi settings - ssid, pass, static ip, gateway, subnet, dns
// TODO: maybe define real monitors count
// TODO: maybe import/export data -> csv (from data.js)
void setup()
{
  Serial.begin(9600);

  WiFi.hostname("EnergyMonitor");
  WiFi.mode(WIFI_STA);
  //WiFi.mode(WIFI_AP_STA);
  //String ssid = SF("EnergyMonitor_") + String(ESP.getChipId(), HEX);
  //WiFi.softAP(ssid.c_str(), "12345678");
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

  if (MDNS.begin("emon"))
  {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }

  DataManager.setup();

  httpUpdater.setup(&server, "admin", "admin");

  //ask server to track these headers
  const char *headerkeys[] = {"Referer", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();

  SPIFFS.begin();

#ifdef REMOTE_DEBUG
  RemoteDebugger.begin(server);
#endif
}

void loop()
{
  DataManager.update();

  MDNS.update();
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