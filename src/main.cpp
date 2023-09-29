#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <TM1637TinyDisplay.h>
#include <TM1637TinyDisplay6.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Timezone.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>

// Define Digital Pins
// Instantiate TM1637TinyDisplay Class
// CLK AND DIO NUMBERS USE GPIO NUMBERS NOT D numbers
TM1637TinyDisplay y_display(5, 4);
TM1637TinyDisplay w_display(0, 2);
TM1637TinyDisplay b_display(14, 12);
TM1637TinyDisplay6 g_display(3, 1); // 6-Digit Display Class
const char* URL = "https://theairtraffic.com/api/display.json";
const char* DISPLAY_KEY = "REDACTED";
// NTP server information
const char* NTP_SERVER = "pool.ntp.org";
const long UTC_OFFSET_SECONDS = 0; // UTC time zone (in seconds)


WiFiUDP ntpUDP;
// Create an NTPClient object to communicate with the NTP server
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET_SECONDS);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //UTC - 5 hours
Timezone usET(usEDT, usEST);

void setup() {

  // Initialize Display
  w_display.setBrightness(BRIGHT_LOW);
  y_display.setBrightness(BRIGHT_HIGH);
  b_display.setBrightness(BRIGHT_LOW);
  g_display.setBrightness(BRIGHT_HIGH);
  b_display.clear();
  g_display.clear();
  g_display.showString("start");
  y_display.clear();
  w_display.clear();
  Serial.begin(9600);

  WiFiManager wifiManager;
  y_display.showString("WIFI");
  wifiManager.autoConnect("TelemDisplay");
  Serial.print("Connected to WIFI");
  y_display.showString("GOOD");
  delay(2);
  y_display.clear();

  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Initialize Display
  timeClient.begin();
}

void loop() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  // Get the UTC time
  time_t utc = epochTime;

  // Get the offset from UTC to EST

  time_t localTime = usET.toLocal(epochTime);

  // Get the hour and minute of the EST time
  int utcHour = hour(utc);
  int estHour = hourFormat12(localTime);
  int utcMinute = minute(localTime);

  Serial.println("Connecting to " + String(URL));
  WiFiClientSecure client;

  //client->setFingerprint(fingerprint_sni_cloudflaressl_com);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
  client.setInsecure();
  HTTPClient http;
  http.begin(client, URL);
  http.addHeader("display-key", DISPLAY_KEY);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();

    // Find the position of the first '{' character in the response
    int startPos = payload.indexOf('{');

    // Find the position of the last '}' character in the response
    int endPos = payload.lastIndexOf('}');

    // Extract the JSON data from the response
    String jsonData = payload.substring(startPos, endPos + 1);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      b_display.clear();
      g_display.clear();
      y_display.clear();
      w_display.clear();
      y_display.showString("JE");
      return;
    } else if (doc.size() == 0) {
      Serial.println("JSON is empty");
    }
    else {
      Serial.print("Got JSON");
      Serial.println(jsonData);
    }
    http.end();



    // Blue
    Serial.println("Blue: ");

    if (doc.containsKey("blue")){
      b_display.showString(doc["blue"]);
    }
    else {
      Serial.println(utcHour+" : "+ utcMinute);
      b_display.showNumberDec(utcHour, 0b01000000, false, 2, 0);
      b_display.showNumber(utcMinute, true, 2, 2);

    }


    if (doc.containsKey("green")){
      g_display.showString(doc["green"]);

      if (strlen(doc["green"]) < 6 && doc.containsKey("baro_rate")){
        int baro_rate = doc["baro_rate"];
        if (baro_rate > 0){
          g_display.setSegments((const uint8_t[]) {0b00000010}, 1, 5);
        } else if (baro_rate < 0) {
          g_display.setSegments((const uint8_t[]) {0b00000100}, 1, 5);
        }
      }
    }
    else {
      g_display.clear();
    }

    if (doc.containsKey("white")){
      w_display.showString(doc["white"]);
    }
    else {
      w_display.clear();
    }
    Serial.println("Yellow: ");
    if (doc.containsKey("yellow")){
      y_display.showString(doc["yellow"]);
    }
    else {
      Serial.print(estHour+" : "+ utcMinute);
      y_display.showNumberDec(estHour, 0b01000000, false, 2, 0);
      y_display.showNumber(utcMinute, true, 2, 2);
    }

  }
  else {
    Serial.print("\nHTTP Connection failed: ");
    Serial.print(httpCode);
    y_display.showString("H_CE");
  }
  delay(15000);
}