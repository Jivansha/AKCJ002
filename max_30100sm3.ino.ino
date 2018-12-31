
#include <ESP8266WiFi.h>
#include <MeanFilterLib.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include<math.h>
#include <Adafruit_ADS1015.h>
 
String apiKey = "xxxxxxxxxxxxx";     //  Enter your Write API key from ThingSpeak

const char *ssid =  "Your wifi Network name";     // replace with your wifi ssid and wpa2 key
const char *pass =  "Network password";
const char* server = "api.thingspeak.com";

Adafruit_ADS1115 ads1115;                  // for the 16-bit version 

#define REPORTING_PERIOD_MS     1000

PulseOximeter hrspo2;
uint16_t tsLastReport = 0;
MeanFilter<float> meanfilter(10);

void display_max30100(uint16_t hr, uint16_t spo2, uint16_t temp, uint16_t gsr) {
  Serial.print("Heart rate: ");
  Serial.print(hr);
  Serial.print(" bpm  *****  SpO2: ");
  Serial.print(spo2);
  Serial.print("%  *****  Temp: ");
  Serial.print(temp);
  Serial.print(" C  ***** GSR: ");
  Serial.print(gsr);
  Serial.println(" ");
}

void rem_dc_hr(uint16_t &hr, uint16_t hrp, int alpha = 0.95) {
  uint16_t wt;
  wt = hr + (alpha * hrp);
  hr = wt - hrp;
}

WiFiClient client;
 
void setup() 
{
       Serial.begin(115200);
       delay(10);
       Serial.println("Initializing MAX30100");
       hrspo2.begin();
       ads1115.begin();
 
       Serial.println("Connecting to ");
       Serial.println(ssid);
 
       WiFi.begin(ssid, pass);
 
      while (WiFi.status() != WL_CONNECTED) 
     {
            delay(500);
            Serial.print(".");
     }
      Serial.println("");
      Serial.println("WiFi connected");
      
 
}
 
void loop() 
{
  
  hrspo2.update();
  uint16_t hr, spo2, temp, hr_prev,hr_f,gsr;
  hr_prev = hrspo2.getHeartRate();
  delay(1000);

              // For both, a value of 0 means "invalid"
              if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
              hr = hrspo2.getHeartRate();
              spo2 = hrspo2.getSpO2();
              temp = hrspo2.getTemperature();
              gsr = ads1115.readADC_SingleEnded(0);

                   if (isnan(hr) || isnan(spo2) || isnan(temp)){
                     Serial.println("Failed to read from sensor!");
                     return;
                   }
                  
              Serial.println("Raw Values - ");
              display_max30100(hr, spo2, temp, gsr);   // display raw hr, spo2, temp, gsr data
              rem_dc_hr(hr, hr_prev, 0.95);            // remove dc component

              hr_f = meanfilter.AddValue(hr);

              Serial.println("Filtered Values - ");
              display_max30100(hr_f, spo2, temp,gsr);       // filtered values
              hr_prev = hr;
              tsLastReport = millis();
              
                         if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
                      {  
                            
                             String postStr = apiKey;
                             postStr +="&field1=";
                             postStr += String(hr_f);
                             postStr +="&field2=";
                             postStr += String(spo2);
                             postStr +="&field3=";
                             postStr += String(temp);
                             postStr +="&field4=";
                             postStr += String(gsr);           
                             postStr += "\r\n\r\n";
 
                             client.print("POST /update HTTP/1.1\n");
                             client.print("Host: api.thingspeak.com\n");
                             client.print("Connection: close\n");
                             client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
                             client.print("Content-Type: application/x-www-form-urlencoded\n");
                             client.print("Content-Length: ");
                             client.print(postStr.length());
                             client.print("\n\n");
                             client.print(postStr);
 
                             Serial.println("..... Send to Thingspeak.");
                        }
          client.stop();
 
          Serial.println("Waiting...");
  
  // thingspeak needs minimum 15 sec delay between updates, i've set it to 30 seconds
  delay(10000);
   }
}
