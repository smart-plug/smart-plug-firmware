#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"

#define AP_SSID "tomada-inteligente"
#define AP_PASSWORD "123457890"

#define DNS_PORT 53

#define WIFI_SCAN_NOT_TRIGGERED -2

#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASSWORD_MAX_LENGTH 63

#define EEPROM_SIZE 512
#define EEPROM_START_ADDR 0

const char index_html[] PROGMEM = "<!DOCTYPE html><html lang=\"pt\"><head> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Configurações</title> <link href='https://fonts.googleapis.com/css?family=Roboto Condensed' rel='stylesheet'> <link href='https://fonts.googleapis.com/css?family=Roboto' rel='stylesheet'> <style>*{font-family: \"Roboto\";}.flex-column{display: flex; flex-direction: column;}body{box-sizing: border-box; height: 100vh; width: 100vw; margin: 0; padding: 0 64px; justify-content: space-evenly;}.title{font-family: \"Roboto Condensed\", \"Roboto\"; font-size: 30px; line-height: 30px; letter-spacing: 0.1px; color: #472d68;}.font-xsmall{font-size: 12px; line-height: 16px;}.font-small{font-size: 14px; line-height: 20px;}.font-medium,.input-field{font-size: 16px; line-height: 24px;}.content{gap: 12px;}form{gap: 24px;}.form-group{display: flex; position: relative; border: 1px solid #79747e; border-radius: 4px;}label{position: absolute; top: -8px; left: 16px; color: #49454f; background-color: white; padding: 0 4px;}.form-group:has(.input-field:focus) label{color: #6750a4;}.input-field{flex: 1; border: none; border-radius: 4px; padding: 8px 0px 8px 16px; letter-spacing: 0.5px; color: #1d1b20; background-color: white;}.input-field:focus{outline: 2px solid #6750a4;}.options{box-sizing: border-box; position: absolute; top: 41px; z-index: 1; width: 100%; background: #f3edf7; border-radius: 4px; transition: max-height 0.5s ease;}.options.options-opened{box-shadow: 0px 1px 2px rgba(0, 0, 0, 0.3), 0px 2px 6px 2px rgba(0, 0, 0, 0.15); max-height: 176px; overflow-y: scroll;}.options:not(.options-opened){max-height: 0; overflow: hidden;}.options > *{padding: 16px 12px; cursor: pointer;}.options > *:hover{background-color: #6750a499; color: white;}button{border: none; background: #6750a4; border-radius: 100px; padding: 10px; color: white;}button:focus{opacity: 90%;}span{display: block; color: #49454f; text-align: center;}.card{background: #fef7ff; padding: 12px 16px;}.card > h1{margin: 0; letter-spacing: 0.5px; color: #1d1b20;}.card > p{margin: 0; letter-spacing: 0.25px; color: #49454f;}.overlay{position: absolute; top: 0; left: 0; height: 100vh; width: 100vw; background-color: white; opacity: 0.9; display: flex; align-items: center; justify-content: center;}.loader{border: 8px solid #f3f3f3; border-top: 8px solid #6750a4; border-radius: 50%; width: 40px; height: 40px; animation: spin 2s linear infinite;}@keyframes spin{0%{transform: rotate(0deg);}100%{transform: rotate(360deg);}}</style></head><body class=\"flex-column\"> <h1 class=\"title\">tomada<br>inteligente</h1> <div class=\"flex-column content\"> <form class=\"flex-column\"> <div class=\"form-group\"> <label for=\"ssid\" class=\"font-xsmall\">Rede</label> <input type=\"text\" id=\"ssid\" name=\"ssid\" class=\"input-field\" autocomplete=\"off\" onfocus=\"openSelectOptions('ssid-options')\" onblur=\"closeSelectOptions('ssid-options')\" onkeyup=\"filterOptions()\"/> <div id=\"ssid-options\" class=\"options\"> <div class=\"font-medium\">Procurando...</div></div></div><div class=\"form-group\"> <label for=\"password\" class=\"font-xsmall\">Senha</label> <input type=\"password\" id=\"password\" name=\"password\" class=\"input-field\"/> </div><button type=\"button\" class=\"font-small\" onclick=\"setNetworkConfig()\">Conectar</button> </form> <span class=\"font-xsmall\">Seu dispositivo está <b id=\"status\">desconectado</b>.</span> </div><div class=\"flex-column card\"> <h1 id=\"device-id\" class=\"font-medium\"></h1> <p class=\"font-small\">Utilize este identificador único para cadastro do seu dispositivo na plataforma da <b>tomada inteligente</b>.</p></div><div id=\"loading-container\" class=\"overlay\"> <div class=\"loader\"></div></div><script>const baseUri=\"\";const selectElement=document.getElementById(\"ssid-options\");const ssidElement=document.getElementById(\"ssid\");const passwordElement=document.getElementById(\"password\");const deviceStatusElement=document.getElementById(\"status\");const deviceIdElement=document.getElementById(\"device-id\");const loadingElement=document.getElementById(\"loading-container\");function openSelectOptions(){cleanOptionsFilter(); selectElement.classList.add(\"options-opened\");}function closeSelectOptions(){setTimeout(()=>{selectElement.classList.remove(\"options-opened\");}, 200);}function selectSsid(value){ssidElement.value=value;}function filterOptions(){const value=ssidElement.value.toLowerCase(); const options=selectElement.childNodes; options.forEach(option=>{option.style.display=option.id.toLowerCase().indexOf(value) > -1 ? '' : 'none';});}function cleanOptionsFilter(){const options=selectElement.childNodes; options.forEach(option=>{if (option){if (option.style){option.style.display='';}}});}async function getDeviceInfo(){const endpoint=\"/network/info\"; const url=baseUri + endpoint; const result=await fetch(url); const data=await result.json(); return data;}async function getAvailableNetworks(){const endpoint=\"/network/scan\"; const url=baseUri + endpoint; let completed=false; let result=await fetch(url); while (result.status===202){result=await new Promise((resolve)=>{setTimeout(()=>{resolve(fetch(url));}, 5000);});}const data=await result.json(); return data;}async function setNetworkConfig(){const endpoint=\"/network/configure\"; const url=baseUri + endpoint; const data={ssid: ssidElement.value, password: passwordElement.value,}; const result=await fetch(url,{method: \"POST\", headers:{\"Content-Type\": \"application/json\"}, body: JSON.stringify(data),}); if (result.status===201){loadingElement.style.display=\"flex\"; setTimeout(()=>{location.reload();}, 5000); return;}alert(\"Desculpe, não foi possível configurar a rede!\");}async function onload(){const{id, ipv4, ssid, status}=await getDeviceInfo(); deviceIdElement.innerHTML=id; ssidElement.value=ssid; deviceStatusElement.innerHTML=status ? \"conectado\" : \"desconectado\"; getAvailableNetworks().then((networks)=>{if (!networks.length){return;}networks=networks.sort((a, b)=> b.signal - a.signal); selectElement.innerHTML=\"\"; networks.forEach(({ssid, signal, secure})=>{const option=document.createElement(\"div\"); option.classList.add(\"font-medium\"); option.id=ssid; option.innerHTML=`${ssid} (${signal}%)${secure ? \" *\" : \"\"}`; option.onclick=()=> selectSsid(ssid); selectElement.appendChild(option);});}); loadingElement.style.display=\"none\";}onload();</script></body></html>";

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

  if (wifi_ssid) {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  } else {
    WiFi.disconnect();
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

    if (wifi_ssid) {
      WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    }
    
    request->send(201);
  });

  server.addHandler(handler);

  server.on("/network/scan", HTTP_GET, [](AsyncWebServerRequest *request) {    
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_NOT_TRIGGERED)
    {
      WiFi.scanNetworks(true, false, true, 100U);
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

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(404);
  });

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
}

void loop()
{
  dns_server.processNextRequest();
}