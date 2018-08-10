#include <MAX6675_Thermocouple.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

int SCK_PIN = D5;
int CS_PIN = D6;
int SO_PIN = D7;
MAX6675_Thermocouple thermocouple(SCK_PIN, CS_PIN, SO_PIN);

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
SSD1306  display(0x3c, 5, 4); // Initialize the OLED display using Wire library

/* DHT22 */
#include "DHT.h"
#define DHTPIN D3
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
int localHum = 0;
float localTemp = 0;
float ktypeTemp = 0;

//WIFI
const char* ssid = "GM_Net";
const char* password = "134679825";
WiFiClient client;

//Thingspeak settings
const int channelID = 412051;
String writeAPIKey = "SVMFBKG4CY91RFXU";
const char* tsServer = "api.thingspeak.com";
long lastUpdate; //The last update
const int durationUpate = 60000; //The frequency of check kazan and puffer temps

void setup()
{
  Serial.begin(115200);
  display.init(); // Initialising the UI will init the display too.
  display.flipScreenVertically();

  //Setup WIFI
  WiFi.begin(ssid, password);
  Serial.println("");

  //Wait for WIFI connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(DHTPIN, INPUT);
  lastUpdate = millis();
  localHum = 0;
  localTemp = 0;

}

void loop()
{
  getDHT();
  display.clear();
  drawDHT();
  display.display();
  delay (2000);

  long now = millis();
  if ( now - lastUpdate > durationUpate ) {
    UdateThinkSpeakChannel();
    lastUpdate = now;
  }
}

/***************************************************
  Get indoor Temp/Hum data
****************************************************/
void getDHT()
{
  ktypeTemp = thermocouple.readCelsius();
  
  float tempIni = localTemp;
  float humIni = localHum;
  localTemp = dht.readTemperature();
  localHum = dht.readHumidity();
  if (isnan(localHum) || isnan(localTemp))   // Check if any reads failed and exit early (to try again).
  {
    Serial.println("Failed to read from DHT sensor!");
    localTemp = tempIni;
    localHum = humIni;
    return;
  }
}

/***************************************************
  Draw Indoor Page
****************************************************/
void drawDHT()
{
  int x = -1;
  int y = 0;
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0 + x, 10 + y, "Hum");

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(43 + x, y, "INDOOR");

  display.setFont(ArialMT_Plain_24);
  String hum = String(localHum) + "%";
  display.drawString(0 + x, 25 + y, hum);
  int humWidth = display.getStringWidth(hum);

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(95 + x, 10 + y, "Temp");

  display.setFont(ArialMT_Plain_24);
  String temp = String(localTemp, 1) + "°C";
  //String temp = String(ktypeTemp, 1) + "°C";
  display.drawString(60 + x, 25 + y, temp);
  int tempWidth = display.getStringWidth(temp);
}

/***************************************************
  Upload to Thingspeak
****************************************************/
void UdateThinkSpeakChannel () {
  if (client.connect(tsServer, 80)) {

    // Construct API request body
    String postStr = writeAPIKey;
    postStr += "&field1=";
    postStr += String(localTemp);
    postStr += "&field2=";
    postStr += String(localHum);
    postStr += "&field3=";
    postStr += String(ktypeTemp);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    client.print("\n\n");

  }
  client.stop();
}
