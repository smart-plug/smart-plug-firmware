#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define AP_SSID "esp-captive"
#define AP_PASSWORD ""

#define DNS_PORT 53

#define WIFI_SCAN_NOT_TRIGGERED -2

#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASSWORD_MAX_LENGTH 63

#define EEPROM_SIZE 512
#define EEPROM_START_ADDR 0

const char index_html[] PROGMEM = "<!DOCTYPE html><html><head><title>Captive Portal</title></head><body><h1>Hello World!</h1></body></html>";

const int ssid_address = EEPROM_START_ADDR;
const int password_address = EEPROM_START_ADDR + WIFI_SSID_MAX_LENGTH + 1;

String wifi_ssid;
String wifi_password;

DNSServer dns_server;
AsyncWebServer server(80);
  
class CaptiveRequestHandler : public AsyncWebHandler
{
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request)
  {
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html);
  }
};

uint8_t dBm2quality(int32_t dBm)
{
  if (dBm <= -100)
    return 0;
  
  if (dBm >= -50)
    return 100;

  return (uint8_t)(2 * (dBm + 100));
}

void setup()
{
  Serial.begin(115200);

  const bool eeprom_started = EEPROM.begin(EEPROM_SIZE);

  if (!eeprom_started)
  {
    Serial.println("Failed to initialise EEPROM!");
  }

  wifi_ssid = EEPROM.readString(ssid_address);
  wifi_password = EEPROM.readString(password_address);

  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  dns_server.start(DNS_PORT, "*", WiFi.softAPIP());

  if (wifi_ssid) 
  {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  }

  server.on("/network/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"id\":" + String(ESP.getEfuseMac()) + ",";
    json += "\"ssid\":\"" + wifi_ssid + "\",";
    json += "\"status\":" + String(WiFi.isConnected() ? "true" : "false") + ",";
    json += "\"ipv4\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    request->send(200, "text/json", json);
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/network/configure", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject obj = json.as<JsonObject>();
    
    const char* ssid = obj["ssid"];
    EEPROM.writeString(ssid_address, ssid);
    
    const char* password = obj["password"];
    EEPROM.writeString(password_address, password);

    EEPROM.commit();

    wifi_ssid = ssid;
    wifi_password = password;

    WiFi.disconnect();
    WiFi.begin(ssid, password);
    
    request->send(201);
  });

  server.addHandler(handler);

  server.on("/network/scan", HTTP_GET, [](AsyncWebServerRequest *request) {    
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_NOT_TRIGGERED)
    {
      WiFi.scanNetworks(true);
    }

    if (n == WIFI_SCAN_NOT_TRIGGERED || n == WIFI_SCAN_RUNNING)
    {
      request->send(202);
      return;
    }

    String json = "[";
    
    if (n > 0)
    {
      for (int i = 0; i < n; ++i) 
      {
        if (i) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ",\"signal\":" + String(dBm2quality(WiFi.RSSI(i)));
        json += ",\"secure\":" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true");
        json += "}";
      }

      WiFi.scanDelete();
    }
    
    json += "]";
    
    request->send(200, "text/json", json);
    
    json = String();
  });

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
}

void loop()
{
  dns_server.processNextRequest();
}