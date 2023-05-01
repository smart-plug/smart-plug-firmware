#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUDP.h>
#include <NTPClient.h>

const char *ssid     = "<ssid>";
const char *password = "<password>";

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
}

void loop() {
  timeClient.update();

  // Get formated date-time
  // Serial.println(timeClient.getFormattedTime());
  
  // Get timestamp: time in seconds since Jan. 1, 1970
  Serial.println(timeClient.getEpochTime());

  delay(1000);
}