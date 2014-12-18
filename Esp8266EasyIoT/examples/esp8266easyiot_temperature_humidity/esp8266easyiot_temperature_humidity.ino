 /*
 V1.0 - first version
 
 Created by Igor Jarc <igor.jarc1@gmail.com>
 See http://iot-playground.com for details
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <Esp8266EasyIoT.h>
#include <SoftwareSerial.h> 
#include <DHT.h>  

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define HUMIDITY_SENSOR_DIGITAL_PIN 2


Esp8266EasyIoT esp; 

SoftwareSerial serialEsp(10, 11);


DHT dht;
float lastTemp;
float lastHum;

Esp8266EasyIoTMsg msgHum(CHILD_ID_HUM, V_HUM);
Esp8266EasyIoTMsg msgTemp(CHILD_ID_TEMP, V_TEMP);


void setup()
{
  serialEsp.begin(9600);
  Serial.begin(115200);  

  Serial.println("EasyIoTEsp init");


  esp.begin(NULL, 3, &serialEsp, &Serial);
  //esp.begin(NULL, &serialEsp);
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  pinMode(13, OUTPUT);

//  Serial.println("present S_HUM");
  esp.present(CHILD_ID_HUM, S_HUM);

//  Serial.println("present S_TEMP");
  esp.present(CHILD_ID_TEMP, S_TEMP);

}

void loop()
{  
  while(!esp.process());

  delay(dht.getMinimumSamplingPeriod());

  while(!esp.process());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT");
  } 
  else if (temperature != lastTemp) 
  {
    lastTemp = temperature;
    esp.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }

  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } 
  else if (humidity != lastHum) 
  {
    lastHum = humidity;
    esp.send(msgHum.set(humidity, 1));
    Serial.print("H: ");
    Serial.println(humidity);
  }
}




