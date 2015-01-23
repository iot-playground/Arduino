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

Esp8266EasyIoT esp; 

SoftwareSerial serialEsp(10, 11);

#define LEAK_PIN  2  // Arduino Digital I/O pin number
#define CHILD_ID_LEAK 0

Esp8266EasyIoTMsg msgLeak(CHILD_ID_LEAK, V_DIGITAL_VALUE);
//Esp8266EasyIoTMsg msgHum(CHILD_ID_LEAK, V_LEAK); // supported in esp >= V1.1 lib

int lastLeakValue = -1;

void setup()
{
  serialEsp.begin(9600);
  Serial.begin(115200);  

  Serial.println("EasyIoTEsp init");
  esp.begin(NULL, 3, &serialEsp, &Serial);

  pinMode(LEAK_PIN, INPUT);

  esp.present(CHILD_ID_LEAK, S_LEAK);
}

void loop()
{
  esp.process();
  
  // Read digital pin value
  int leakValue = digitalRead(LEAK_PIN); 
  // send if changed
  if (leakValue != lastLeakValue) {    
    Serial.println(leakValue);
    esp.send(msgLeak.set(leakValue==0?0:1));
    lastLeakValue = leakValue;
  }  
}


