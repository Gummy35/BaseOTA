#include <ControllerWebServer.h>
#include <WiFi.h>
#include <ElegantOTA.h>
#include <WebSerial.h>
#include <Logger.h>
#define WIFI_CREDENTIALS_FILE "/wifi_credentials.txt"
#define AP_SSID "ESP32_AP"

ControllerWebServerClass::ControllerWebServerClass()
{
  _server = new AsyncWebServer(80);
  wifi_ssid = "";
  wifi_password = "";
}

bool ControllerWebServerClass::_readWiFiCredentials(String &ssid, String &password)
{
  File file = LittleFS.open(WIFI_CREDENTIALS_FILE, FILE_READ);
  if (!file)
  {
    Serial.println("Failed to open wifi credentials file");
    return false;
  }

  ssid = file.readStringUntil('\n');
  password = file.readStringUntil('\n');

  // Remove newline characters
  ssid.trim();
  password.trim();

  file.close();

  return !ssid.isEmpty() && !password.isEmpty();
}

void ControllerWebServerClass::_startAPMode()
{
  WiFi.softAP(AP_SSID);

  Serial.println("Access Point started:");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void ControllerWebServerClass::_connectToWiFi(const String &ssid, const String &password)
{
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10)
  {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Connected to WiFi successfully.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Failed to connect to WiFi. Starting AP mode.");
    _startAPMode();
  }
}

bool ControllerWebServerClass::_writeWiFiCredentials(const String &ssid, const String &password)
{
  File file = LittleFS.open(WIFI_CREDENTIALS_FILE, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open wifi credentials file for writing");
    return false;
  }

  file.println(ssid);
  file.println(password);

  file.close();
  return true;
}

void ControllerWebServerClass::_onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success)
  {
    Logger.Log("OTA update complete");
    _writeWiFiCredentials(wifi_ssid, wifi_password);
    LittleFS.end();
  }
  else
  {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void ControllerWebServerClass::_handleGetJsonFile(AsyncWebServerRequest *request, const char *filename)
{
  if (LittleFS.exists(filename))
    request->send(LittleFS, filename, "application/json");
  else
    request->send(200, "application/json", "{}");
}

void ControllerWebServerClass::_saveJsonToFile(AsyncWebServerRequest *request, JsonVariant &json, const char *filename)
{
  JsonDocument data;
  if (json.is<JsonArray>())
  {
    data = json.as<JsonArray>();
  }
  else if (json.is<JsonObject>())
  {
    data = json.as<JsonObject>();
  }

  File file = LittleFS.open(filename, "w");
  if (!file)
  {
    Logger.Log("Failed to open file for writing");
    request->send(500, "application/json", "{error:\"Failed to open file for writing\"}");
    return;
  }

  if (serializeJson(data, file) == 0)
  {
    Logger.Log("Failed to write to file");
    request->send(500, "application/json", "{error:\"Failed to write to file\"}");
    file.close();
    return;
  }

  file.close();
  // String response;
  // serializeJson(data, response);
  // request->send(200, "application/json", response);
  // Serial.println(response);
  Logger.Log("Config saved. Restarting...");
  request->send(200, "text/plain", "file saved");
  LittleFS.end();
  ESP.restart();
}

void ControllerWebServerClass::_handleSaveWiFi(AsyncWebServerRequest *request)
{
  if (request->hasArg("ssid") && request->hasArg("password"))
  {
    String ssid = request->arg("ssid");
    String password = request->arg("password");

    if (_writeWiFiCredentials(ssid, password))
    {
      request->send(200, "text/plain", "Credentials saved. Please restart the device.");
      LittleFS.end();
      ESP.restart();
    }
    else
    {
      request->send(500, "text/plain", "Failed to save credentials.");
    }
  }
  else
  {
    request->send(400, "text/plain", "Invalid request.");
  }
}

void ControllerWebServerClass::_handleClearWiFi(AsyncWebServerRequest *request)
{
  if (LittleFS.remove(WIFI_CREDENTIALS_FILE))
  {
    WiFi.eraseAP();
    ESP.restart();
    request->send(200, "text/plain", "Credentials cleared.");
  }
  else
  {
    request->send(500, "text/plain", "Failed to clear credentials.");
  }
}


String ControllerWebServerClass::_htmlProcessor(const String &var)
{
  if (var == "WIFI_SSID")
    return wifi_ssid;

  return String();
}

void ControllerWebServerClass::_handlePage(AsyncWebServerRequest *request, const char *pagePath)
{
  request->send(LittleFS, pagePath, String(), false, [this](const String &str)
                { return this->_htmlProcessor(str); });
}

void ControllerWebServerClass::begin()
{
  if (_readWiFiCredentials(wifi_ssid, wifi_password))
  {
    Serial.println("WiFi credentials read successfully.");
    Serial.printf("SSID: %s, Password: %s\n", wifi_ssid.c_str(), wifi_password.c_str());
    _connectToWiFi(wifi_ssid, wifi_password);
  }
  else
  {
    Serial.println("WiFi credentials not found or invalid. Starting AP mode.");
    _startAPMode();
  }
  _server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
              { this->_handlePage(request, "/html/index.html"); });
  _server->on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request)
              { this->_handlePage(request, "/html/wifi.html"); });
  _server->on("/savewifi", HTTP_POST, [this](AsyncWebServerRequest *request)
              { this->_handleSaveWiFi(request); });
  _server->on("/clearwifi", HTTP_POST, [this](AsyncWebServerRequest *request)
              { this->_handleClearWiFi(request); });
  _server->serveStatic("/static/", LittleFS, "/html/");

  ElegantOTA.onEnd([this](bool success)
                   { this->_onOTAEnd(success); });

  WebSerial.begin(_server);
  ElegantOTA.begin(_server); // Start ElegantOTA
  _server->begin();
}

void ControllerWebServerClass::showIp()
{
  if (WiFi.getMode() == WIFI_MODE_STA)
    Logger.Log(WiFi.localIP().toString().c_str());
  else if (WiFi.getMode() == WIFI_MODE_AP)
    Logger.Log(WiFi.softAPIP().toString().c_str());
}

void ControllerWebServerClass::loop()
{
  WebSerial.loop();
  ElegantOTA.loop();
}

ControllerWebServerClass ControllerWebServer;