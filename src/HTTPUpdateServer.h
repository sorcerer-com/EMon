#ifndef __HTTP_UPDATE_SERVER_H
#define __HTTP_UPDATE_SERVER_H

#include<SPIFFS.h>
#include <StreamString.h>
#include <Update.h>

#include "ESPAsyncWebServer/ESPAsyncWebServer.h"

// https://github.com/lbernstone/asyncUpdate/blob/master/AsyncUpdate.ino
static const char serverIndex[] PROGMEM =
R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <form method='POST' action='' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Update Firmware'>
     </form>
     <form method='POST' action='' enctype='multipart/form-data'>
         FileSystem:<br>
         <input type='file' accept='.bin,.bin.gz,.image' name='filesystem'>
         <input type='submit' value='Update FileSystem'>
     </form>
     </body>
     </html>)";
static const char successResponse[] PROGMEM =
"<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...";

class HTTPUpdateServer
{
public:
    HTTPUpdateServer(bool serial_debug=false) {
        _serial_output = serial_debug;
        _server = NULL;
        _username = emptyString;
        _password = emptyString;
        _authenticated = false;
    }

    void setup(AsyncWebServer *server)
    {
        setup(server, emptyString, emptyString);
    }

    void setup(AsyncWebServer *server, const String& path)
    {
        setup(server, path, emptyString, emptyString);
    }

    void setup(AsyncWebServer *server, const String& username, const String& password)
    {
        setup(server, "/update", username, password);
    }

    void setup(AsyncWebServer *server, const String& path, const String& username, const String& password)
    {
        _server = server;
        _username = username;
        _password = password;

        // handler for the /update form page
        _server->on(path.c_str(), HTTP_GET, [&](AsyncWebServerRequest *request) {
            if (_username != emptyString && _password != emptyString && !request->authenticate(_username.c_str(), _password.c_str()))
                return request->requestAuthentication();
            request->send_P(200, PSTR("text/html"), serverIndex);
            });

        // handler for the /update form POST (once file upload finishes)
        _server->on(path.c_str(), HTTP_POST, [&](AsyncWebServerRequest *request) {
            if (!_authenticated)
                return request->requestAuthentication();
            if (Update.hasError()) {
                request->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
            }
            else {
                request->client()->setNoDelay(true);
                request->send_P(200, PSTR("text/html"), successResponse);
                delay(100);
                request->client()->stop();
                ESP.restart();
            }
            }, [&](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {

                if (index == 0) { // upload starts
                    _updaterError.clear();
                    if (_serial_output)
                        Serial.setDebugOutput(true);

                    _authenticated = (_username == emptyString || _password == emptyString || request->authenticate(_username.c_str(), _password.c_str()));
                    if (!_authenticated) {
                        if (_serial_output)
                            Serial.printf("Unauthenticated Update\n");
                        return;
                    }

                    if (_serial_output)
                        Serial.printf("Update: %s\n", filename.c_str());
                    if (filename.indexOf("spiffs") > -1) { // if filename contains 'spiffs'
                        SPIFFS.end();
                        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {//start with max available size
                            if (_serial_output) Update.printError(Serial);
                        }
                    }
                    else {
                        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {//start with max available size
                            _setUpdaterError();
                        }
                    }
                }
                if (_authenticated && !_updaterError.length()) {
                    if (_serial_output) Serial.printf(".");
                    if (Update.write(data, len) != len) {
                        _setUpdaterError();
                    }
                }
                if (_authenticated && final && !_updaterError.length()) {
                    if (Update.end(true)) { //true to set the size to the current progress
                        if (_serial_output) Serial.printf("Update Success: %u\nRebooting...\n", (index + len));
                    }
                    else {
                        _setUpdaterError();
                    }
                    if (_serial_output) Serial.setDebugOutput(false);
                }
                delay(0);
            });
    }

    void updateCredentials(const String& username, const String& password)
    {
        _username = username;
        _password = password;
    }

protected:
    void _setUpdaterError()
    {
        if (_serial_output) Update.printError(Serial);
        StreamString str;
        Update.printError(str);
        _updaterError = str.c_str();
    }

private:
    bool _serial_output;
    AsyncWebServer *_server;
    String _username;
    String _password;
    bool _authenticated;
    String _updaterError;
};


#endif