#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define REMOTE_DEBUG // TODO: remove
#include "src/RemoteDebugger.h"
#include "DataManager.h"
#include "WebHandler.h"

ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WebHandler webHandler(server);

unsigned long reconnectTimer = millis() - 5 * MILLIS_IN_A_SECOND;

// TODO: print readme, high voltage warning label
// TODO: hardware reset way, if cannot connect to WiFi (or create AP), if forget the login password?
// TODO: maybe define real monitors count
// TODO: maybe define(determine) the real tarriffs count
// TODO: skip first data - wrong one
void setup()
{
  Serial.begin(9600);
#ifdef REMOTE_DEBUG
  RemoteDebugger.begin(server);
#endif

  // setup WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname("EnergyMonitor");
  if (WiFi.config(DataManager.data.settings.wifi_ip, DataManager.data.settings.wifi_gateway,
                  DataManager.data.settings.wifi_subnet, DataManager.data.settings.wifi_dns))
  {
    DEBUGLOG("EMonitor", "Config WiFi - IP: %s, Gateway: %s, Subnet: %s, DNS: %s",
             IPAddress(DataManager.data.settings.wifi_ip).toString().c_str(),
             IPAddress(DataManager.data.settings.wifi_gateway).toString().c_str(),
             IPAddress(DataManager.data.settings.wifi_subnet).toString().c_str(),
             IPAddress(DataManager.data.settings.wifi_dns).toString().c_str());
  }
  else
  {
    DEBUGLOG("EMonitor", "Cannot config Wifi");
  }

  wifiMulti.addAP(DataManager.data.settings.wifi_ssid, DataManager.data.settings.wifi_passphrase);

  // Wait for connection
  DEBUGLOG("EMonitor", "Connecting...");
  for (int i = 0; i < 10; i++)
  {
    if (wifiMulti.run() == WL_CONNECTED)
    {
      DEBUGLOG("EMonitor", "WiFi: %s, IP: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      break;
    }
    delay(500);
  }

  if (MDNS.begin("emon"))
  {
    DEBUGLOG("EMonitor", "MDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }

  //ask server to track these headers
  const char *headerkeys[] = {"Referer", "Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();

  httpUpdater.setup(&server, "admin", "admin");

  DataManager.setup();
}

void loop()
{
  DataManager.update();

  MDNS.update();
  server.handleClient();

  // TODO: maybe try to reconnect more often, if not connected
  if (millis() - reconnectTimer > 5 * MILLIS_IN_A_SECOND)
  {
    reconnectTimer = millis();
    if (wifiMulti.run() != WL_CONNECTED)
    {
      DEBUGLOG("EMonitor", "Fail to reconnect...");
      if (WiFi.getMode() == WIFI_STA)
      {
        // 192.168.244.1
        DEBUGLOG("EMonitor", "Create AP");
        WiFi.mode(WIFI_AP_STA);
        if (!WiFi.softAPConfig(DataManager.data.settings.wifi_ip, DataManager.data.settings.wifi_gateway,
                               DataManager.data.settings.wifi_subnet))
        {
          DEBUGLOG("EMonitor", "Config WiFi AP - IP: %s, Gateway: %s, Subnet: %s, DNS: %s",
                   IPAddress(DataManager.data.settings.wifi_ip).toString().c_str(),
                   IPAddress(DataManager.data.settings.wifi_gateway).toString().c_str(),
                   IPAddress(DataManager.data.settings.wifi_subnet).toString().c_str(),
                   IPAddress(DataManager.data.settings.wifi_dns).toString().c_str());
        }
        else
        {
          DEBUGLOG("EMonitor", "Cannot config Wifi AP");
        }

        String ssid = SF("EnergyMonitor_") + String(ESP.getChipId(), HEX);
        WiFi.softAP(ssid.c_str(), "12345678");
        DEBUGLOG("EMonitor", "AP WiFi: %s, IP: %s", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());
      }
    }
    else if (WiFi.getMode() != WIFI_STA)
    {
      WiFi.mode(WIFI_STA);
      DEBUGLOG("EMonitor", "Reconnected WiFi: %s, IP: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
    }
  }
}