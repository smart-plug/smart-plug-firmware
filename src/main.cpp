#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <ResponsiveAnalogRead.h>
#include <NTPClient.h>

#define AP_SSID "tomada-inteligente"
#define AP_PASSWORD "123457890"

#define MQTT_HOST "192.168.18.245" //"test.mosquitto.org"
#define MQTT_PORT 1883

#define NTP_POOL_SERVER "time.google.com"

#define MEASUREMENT_INTERVAL_MS 30000

#define MQTT_MEASUREMENT_TOPIC "smart-plug/measurement"
#define MQTT_STATUS_TOPIC "smart-plug/status"
#define MQTT_STATUS_CHANGED_TOPIC "smart-plug/load_change"

#define DNS_PORT 53

#define WIFI_SCAN_NOT_TRIGGERED -2

#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASSWORD_MAX_LENGTH 63

#define EEPROM_SIZE 512
#define EEPROM_START_ADDR 0

#define SENSOR_PIN 33
#define OFFSET_SAMPLES 10
#define RESOLUTION 4095
#define VREF 3.3
#define SENSIBILITY 0.04
#define FREQUENCY 60
#define MOVING_AVERAGE_SAMPLES 10
#define ACTIVITY_THRESHOLD 0.05

#define VOLTAGE 127.00

const char index_html[] PROGMEM = "<!DOCTYPE html><html lang=\"pt\"><head> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Configurações</title> <link href='https://fonts.googleapis.com/css?family=Roboto Condensed' rel='stylesheet'> <link href='https://fonts.googleapis.com/css?family=Roboto' rel='stylesheet'> <style>*{font-family: \"Roboto\";}.flex-column{display: flex; flex-direction: column;}body{box-sizing: border-box; height: 100vh; width: 100vw; margin: 0; padding: 0 64px; justify-content: space-evenly;}.title{font-family: \"Roboto Condensed\", \"Roboto\"; font-size: 30px; line-height: 30px; letter-spacing: 0.1px; color: #472d68;}.font-xsmall{font-size: 12px; line-height: 16px;}.font-small{font-size: 14px; line-height: 20px;}.font-medium,.input-field{font-size: 16px; line-height: 24px;}.content{gap: 12px;}form{gap: 24px;}.form-group{display: flex; position: relative; border: 1px solid #79747e; border-radius: 4px;}label{position: absolute; top: -8px; left: 16px; color: #49454f; background-color: white; padding: 0 4px;}.form-group:has(.input-field:focus) label{color: #6750a4;}.input-field{flex: 1; border: none; border-radius: 4px; padding: 8px 0px 8px 16px; letter-spacing: 0.5px; color: #1d1b20; background-color: white;}.input-field:focus{outline: 2px solid #6750a4;}.options{box-sizing: border-box; position: absolute; top: 41px; z-index: 1; width: 100%; background: #f3edf7; border-radius: 4px; transition: max-height 0.5s ease;}.options.options-opened{box-shadow: 0px 1px 2px rgba(0, 0, 0, 0.3), 0px 2px 6px 2px rgba(0, 0, 0, 0.15); max-height: 176px; overflow-y: scroll;}.options:not(.options-opened){max-height: 0; overflow: hidden;}.options > *{padding: 16px 12px; cursor: pointer;}.options > *:hover{background-color: #6750a499; color: white;}button{border: none; background: #6750a4; border-radius: 100px; padding: 10px; color: white;}button:focus{opacity: 90%;}span{display: block; color: #49454f; text-align: center;}.card{background: #fef7ff; padding: 12px 16px;}.card > h1{margin: 0; letter-spacing: 0.5px; color: #1d1b20;}.card > p{margin: 0; letter-spacing: 0.25px; color: #49454f;}.overlay{position: absolute; top: 0; left: 0; height: 100vh; width: 100vw; background-color: white; opacity: 0.9; display: flex; align-items: center; justify-content: center;}.loader{border: 8px solid #f3f3f3; border-top: 8px solid #6750a4; border-radius: 50%; width: 40px; height: 40px; animation: spin 2s linear infinite;}@keyframes spin{0%{transform: rotate(0deg);}100%{transform: rotate(360deg);}}</style></head><body class=\"flex-column\"> <h1 class=\"title\">tomada<br>inteligente</h1> <div class=\"flex-column content\"> <form class=\"flex-column\"> <div class=\"form-group\"> <label for=\"ssid\" class=\"font-xsmall\">Rede</label> <input type=\"text\" id=\"ssid\" name=\"ssid\" class=\"input-field\" autocomplete=\"off\" onfocus=\"openSelectOptions('ssid-options')\" onblur=\"closeSelectOptions('ssid-options')\" onkeyup=\"filterOptions()\"/> <div id=\"ssid-options\" class=\"options\"> <div class=\"font-medium\">Procurando...</div></div></div><div class=\"form-group\"> <label for=\"password\" class=\"font-xsmall\">Senha</label> <input type=\"password\" id=\"password\" name=\"password\" class=\"input-field\"/> </div><button type=\"button\" class=\"font-small\" onclick=\"setNetworkConfig()\">Conectar</button> </form> <span class=\"font-xsmall\">Seu dispositivo está <b id=\"status\">desconectado</b>.</span> </div><div class=\"flex-column card\"> <h1 id=\"device-id\" class=\"font-medium\"></h1> <p class=\"font-small\">Utilize este identificador único para cadastro do seu dispositivo na plataforma da <b>tomada inteligente</b>.</p></div><div id=\"loading-container\" class=\"overlay\"> <div class=\"loader\"></div></div><script>const baseUri=\"\";const selectElement=document.getElementById(\"ssid-options\");const ssidElement=document.getElementById(\"ssid\");const passwordElement=document.getElementById(\"password\");const deviceStatusElement=document.getElementById(\"status\");const deviceIdElement=document.getElementById(\"device-id\");const loadingElement=document.getElementById(\"loading-container\");function openSelectOptions(){cleanOptionsFilter(); selectElement.classList.add(\"options-opened\");}function closeSelectOptions(){setTimeout(()=>{selectElement.classList.remove(\"options-opened\");}, 200);}function selectSsid(value){ssidElement.value=value;}function filterOptions(){const value=ssidElement.value.toLowerCase(); const options=selectElement.childNodes; options.forEach(option=>{option.style.display=option.id.toLowerCase().indexOf(value) > -1 ? '' : 'none';});}function cleanOptionsFilter(){const options=selectElement.childNodes; options.forEach(option=>{if (option){if (option.style){option.style.display='';}}});}async function getDeviceInfo(){const endpoint=\"/network/info\"; const url=baseUri + endpoint; const result=await fetch(url); const data=await result.json(); return data;}async function getAvailableNetworks(){const endpoint=\"/network/scan\"; const url=baseUri + endpoint; let completed=false; let result=await fetch(url); while (result.status===202){result=await new Promise((resolve)=>{setTimeout(()=>{resolve(fetch(url));}, 5000);});}const data=await result.json(); return data;}async function setNetworkConfig(){const endpoint=\"/network/configure\"; const url=baseUri + endpoint; const data={ssid: ssidElement.value, password: passwordElement.value,}; const result=await fetch(url,{method: \"POST\", headers:{\"Content-Type\": \"application/json\"}, body: JSON.stringify(data),}); if (result.status===201){loadingElement.style.display=\"flex\"; setTimeout(()=>{location.reload();}, 5000); return;}alert(\"Desculpe, não foi possível configurar a rede!\");}async function onload(){const{id, ipv4, ssid, status}=await getDeviceInfo(); deviceIdElement.innerHTML=id; ssidElement.value=ssid; deviceStatusElement.innerHTML=status ? \"conectado\" : \"desconectado\"; getAvailableNetworks().then((networks)=>{if (!networks.length){return;}networks=networks.sort((a, b)=> b.signal - a.signal); selectElement.innerHTML=\"\"; networks.forEach(({ssid, signal, secure})=>{const option=document.createElement(\"div\"); option.classList.add(\"font-medium\"); option.id=ssid; option.innerHTML=`${ssid} (${signal}%)${secure ? \" *\" : \"\"}`; option.onclick=()=> selectSsid(ssid); selectElement.appendChild(option);});}); loadingElement.style.display=\"none\";}onload();</script></body></html>";

const int ssid_address = EEPROM_START_ADDR;
const int password_address = EEPROM_START_ADDR + WIFI_SSID_MAX_LENGTH + 1;

String wifi_ssid;
String wifi_password;

DNSServer dns_server;
AsyncWebServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_POOL_SERVER);

AsyncMqttClient mqttClient;

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
TimerHandle_t measureAndPublishTimer;

const uint64_t device_id = ESP.getEfuseMac();

ResponsiveAnalogRead analog(0, true);

uint16_t offset = 0;

const uint32_t period_us = 1000000 / FREQUENCY;

struct MovingAverage
{
  float history[MOVING_AVERAGE_SAMPLES];
  int position = 0;
  float aggregated = 0.0;
  float average = 0.0;
};

struct MovingAverage movingAverage;

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

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  if (!wifi_ssid.isEmpty()) {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  } else {
    WiFi.disconnect();
  }
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void setupNTPClient() {
  timeClient.begin();
  Serial.println(timeClient.forceUpdate() ? "NTP was updated" : "Could not update NTP!");
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  bool time_updated = false;
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      setupNTPClient();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0);
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT");
  mqttClient.subscribe(MQTT_STATUS_TOPIC, 0);
  xTimerStart(measureAndPublishTimer, 0);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  xTimerStop(measureAndPublishTimer, 0);

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

float movingAverageFilter(struct MovingAverage *data, float newValue) {
  data->aggregated += newValue - data->history[data->position];
  data->history[data->position] = newValue;
  data->average = (float)data->aggregated / MOVING_AVERAGE_SAMPLES;
  data->position = (data->position + 1) % MOVING_AVERAGE_SAMPLES;
  return (float)data->average;
}

void cleanMovingAverageHistory(struct MovingAverage *data) {
  for (int i = 0; i < MOVING_AVERAGE_SAMPLES; i++) {
    data->history[i] = 0;
  }
}

uint16_t getSensorZeroOffset() {
  unsigned long value = 0;

  for (int i = 0; i < OFFSET_SAMPLES; i++) {
    value += analogRead(SENSOR_PIN);
  }

  return value / OFFSET_SAMPLES;
}

void measureAndPublish() {
  if (!mqttClient.connected()) {
    Serial.println("[Measurement] Mqtt not connected");
    return;
  }

  uint32_t Isum = 0, measurements_count = 0;
  int32_t Inow;

  uint32_t t_start_us = micros();
  while (micros() - t_start_us < period_us)
  {
    uint16_t reading = analogRead(SENSOR_PIN);
    analog.update(reading);

    Inow = analog.getValue() - offset;
    Isum += Inow * Inow;

    measurements_count++;
  }

  float Irms = sqrt(Isum / measurements_count) / RESOLUTION * VREF / SENSIBILITY;
  float IrmsAverage = movingAverageFilter(&movingAverage, Irms);

  float Ifinal = IrmsAverage < ACTIVITY_THRESHOLD ? 0 : IrmsAverage;

  char buf[100];
  sprintf(buf, 
    "{\"device_id\": %llu, \"current\": %.2f, \"voltage\": %.2f, \"timestamp\": %lu}",
    device_id, Ifinal, VOLTAGE, timeClient.getEpochTime());

  Serial.print("Sending ");
  Serial.print(strlen(buf));
  Serial.println(" bytes to mqtt:");
  Serial.println(buf);
  
  uint8_t qos = 1;
  bool retain = false;
  mqttClient.publish(MQTT_MEASUREMENT_TOPIC, qos, retain, buf);
}

void setup()
{
  Serial.begin(115200);

  pinMode(SENSOR_PIN, INPUT);
  analog.setAnalogResolution(RESOLUTION + 1);

  cleanMovingAverageHistory(&movingAverage);

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

  server.on("/network/info", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"id\":" + String(device_id) + ",";
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

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));
  measureAndPublishTimer = xTimerCreate("measurementTimer", pdMS_TO_TICKS(MEASUREMENT_INTERVAL_MS), pdTRUE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(measureAndPublish));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  offset = getSensorZeroOffset();

  connectToWifi();
}

void loop()
{
  dns_server.processNextRequest();
  if (WiFi.isConnected()) {
    timeClient.update();
  }
  delay(100);
}