/*
  1) Upload code to the device
  2) Upload spiffs image to the device through "/update"
*/

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>

#define DEBUG
//#define VOLTAGE_MONITOR // TODO: remove it since all devices will have voltage monitor

#include "src/ESPAsyncWebServer/ESPAsyncWebServer.h"
#include "src/ESPAsyncWebServer/SPIFFSEditor.h"
#include "src/HTTPUpdateServer.h"
#include "src/RemoteDebugger.h"
#include "DataManager.h"
#include "WebHandler.h"

// TODO: revise all pins / ADS too
static const uint8_t ledPin = 22;
static const uint8_t buttonPin = 0; // GPIO0 / D3
unsigned long buttonTimer = 0;

unsigned long reconnectTimer = millis() - 5 * MILLIS_IN_A_SECOND;

WiFiMulti wifiMulti;
AsyncWebServer server(80);
HTTPUpdateServer httpUpdater;
WebHandler webHandler(server, ledPin);

TaskHandle_t Task1;

void setup()
{
  Serial.begin(9600);
#ifdef REMOTE_DEBUG
  RemoteDebugger.begin(server);
#endif

  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(ledPin, HIGH);
  
  SPIFFS.begin(); // TODO: if SPIFFS not ok, everything is broken - no UI - return message?
  if (LITTLEFS.begin())
      DataManager.data.load();
  else // TODO: data is broken - message (in UI?)
      DataManager.data.reset();

  // setup WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("EnergyMonitor"); // TODO: test
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

  server.begin();
  //server.addHandler(new SPIFFSEditor(LITTLEFS, "admin", "admin"));

  httpUpdater.setup(&server, "admin", "admin");

  DataManager.setup();
  webHandler.setup();

  xTaskCreatePinnedToCore(
    loop0,       /* Task function. */
    "loop0Task", /* name of task. */
    8192,        /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
}

void loop()
{
  // Core 1 (APP_CORE)
  DataManager.update();
}

void loop0(void * pvParameters) {
  // Core 0 (PRO_CORE)
  for (;;) {

    // Reset button
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW) // button down
    {
      if (buttonTimer == 0)
        buttonTimer = millis();
      else if (millis() - buttonTimer > 5 * MILLIS_IN_A_SECOND)
      {
        DataManager.data.reset();
        DataManager.data.save(Data::SaveFlags::Base | Data::SaveFlags::Minutes | Data::SaveFlags::Settings);
        server.end();
        ESP.restart();
      }
    }
    if (buttonState == HIGH && buttonTimer > 0) // button up
    {
      server.end();
      ESP.restart();
    }

    // Try to reconnect to WiFi
    if (millis() - reconnectTimer > 15 * MILLIS_IN_A_SECOND)
    {
      reconnectTimer = millis();
      if (wifiMulti.run() != WL_CONNECTED)
      {
        DEBUGLOG("EMonitor", "Fail to reconnect...");
        if (WiFi.getMode() == WIFI_STA)
        {
          // 192.168.244.1
          // TODO: set default AP IP address
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

          String ssid = SF("EnergyMonitor_") + String((unsigned long)ESP.getEfuseMac(), 16);
          WiFi.softAP(ssid.c_str(), "12345678");
          DEBUGLOG("EMonitor", "AP WiFi: %s, IP: %s", ssid.c_str(), WiFi.softAPIP().toString().c_str());
        }
      }
      else if (WiFi.getMode() != WIFI_STA)
      {
        WiFi.mode(WIFI_STA);
        DEBUGLOG("EMonitor", "Reconnected WiFi: %s, IP: %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
      }
    }

    // set wifi credentials by serial
    if (Serial.available())
    {
      if (Serial.readString() == "set wifi")
      {
        Serial.println("Waiting for wifi ssid...");
        while(!Serial.available()) { delay(1000); }
        strcpy(DataManager.data.settings.wifi_ssid, Serial.readString().c_str());
        Serial.println("Waiting for wifi passphrase...");
        while(!Serial.available()) { delay(1000); }
        strcpy(DataManager.data.settings.wifi_passphrase, Serial.readString().c_str());
        DataManager.data.save(Data::SaveFlags::Settings);
        ESP.restart();
      }
    }
    delay(1);
  }
}
