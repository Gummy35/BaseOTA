#pragma once

#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

class ControllerWebServerClass
{
public:
    ControllerWebServerClass();
    void begin();
    void loop();
    void showIp();

private:
    bool _readWiFiCredentials(String &ssid, String &password);
    void _startAPMode();
    void _connectToWiFi(const String &ssid, const String &password);
    bool _writeWiFiCredentials(const String &ssid, const String &password);
    void _onOTAEnd(bool success);
    void _handleGetJsonFile(AsyncWebServerRequest *request, const char *filename);
    void _saveJsonToFile(AsyncWebServerRequest *request, JsonVariant &json, const char *filename);
    void _handleSaveWiFi(AsyncWebServerRequest *request);
    void _handleClearWiFi(AsyncWebServerRequest *request);
    void _handlePage(AsyncWebServerRequest *request, const char *pagePath = "/html/index.html");
    String _htmlProcessor(const String &var);

    String wifi_ssid;
    String wifi_password;
    AsyncWebServer *_server;
};

extern ControllerWebServerClass ControllerWebServer;