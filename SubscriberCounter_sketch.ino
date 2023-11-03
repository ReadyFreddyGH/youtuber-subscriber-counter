// Library Imports
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> 
#include <Wire.h>  
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <string>

// File imports
#include "secrets.h"
#include "FontSubs.h"

// Uncomment according to your hardware type
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
//#define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW

#define MAX_DEVICES 4
#define CS_PIN 5

MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);


WiFiClientSecure client;

unsigned long api_mtbs = 10000; // Change length of time before the next API call (Default: 10,000 -> 10 sec)
unsigned long api_lasttime;
long subs = 0;
long views = 0;

String apiUrl = "";
StaticJsonDocument<1000> doc;

bool showSubs = false;

void setup() {
  // Start Console
  Serial.begin(9600);

  // Setup screen,
  Display.begin();
  Display.setIntensity(0);
  Display.setFont(fontSubs);
  Display.setTextAlignment(PA_CENTER);

  // Connect to wifi
  Serial.print("Connecting to Wifi...");
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  Display.print(" WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); 
    delay(500);
  }
  Serial.println("");
  Serial.println(WiFi.localIP());
  Display.print("Connected");
  delay(2000);

  // Start process of getting data
  Display.displayClear();
  Display.print("fetching");
  delay(250);

  client.setInsecure();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - api_lasttime > api_mtbs)  {
      // Instantiate http client
      // We use http client instead of YoutubeApi.h library to allow us to fetch data from any site in future
      HTTPClient http;

      // Convert the C-style strings to std::string objects
      std::string channelID(SECRET_CHANNELID);
      std::string apiKey(SECRET_APIKEY);

      // Create Api URL
      apiUrl = ("https://www.googleapis.com/youtube/v3/channels?part=statistics&id=" + channelID + "&key=" + apiKey).c_str();

      // Run API call and get status code
      http.begin(apiUrl);
      int httpCode = http.GET();

      if (httpCode > 0) {
        String payload = http.getString();

        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, payload);

        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }

        // Display Sub or View count
        Serial.print(payload);
        if(showSubs) {
          subs = doc["items"][0]["statistics"]["subscriberCount"];
          String subsCount = num_format(subs);
          Serial.println(subsCount);
          Display.print("*" + subsCount); 
        }else{
          views = doc["items"][0]["statistics"]["viewCount"];
          String viewsCount = num_format(views);
          Serial.println(viewsCount);
          Display.print("*" + viewsCount); 
        }

        // Change from subs to views
        showSubs = !showSubs;
      }
      api_lasttime = millis();
    }
  }
}

// Code from The Swedish Maker
// https://www.youtube.com/@TheSwedishMaker
String num_format(long num){
     String num_s;
     long num_original = num;
     if (num>99999 && num<=999999){
        num = num / 1000;
        long fraction = num_original%1000;
        String num_fraction = String(fraction);
        String decimal = num_fraction.substring(0,1);
        num_s = String(num) + "." + decimal + "K";          
    }
    else if(num>999999){
        num = num / 1000000;
        long fraction = num_original%1000000;
        String num_fraction = String(fraction);
        String decimal = num_fraction.substring(0,1);
        if (num_original<100000000){
          num_s = " " + String(num) + "." + decimal + "M";      
        }
        else{
        num_s = String(num) + "." + decimal + "M";
        }
    }
    else{
        int num_l = String(num).length();
        char num_f[15];
        int blankDigits = 6 - num_l;
        for(int i = 0; i < blankDigits; i++){
          num_f[i] = ' '; 
        }
        num_f[blankDigits] = '\0';
        num_s = num_f + String(num);
    }
    return num_s;
}    