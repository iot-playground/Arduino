 /*
 V1.0 - first version
 V1.1 - adopt to new library 
 
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



#define RELAY_1  13  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay


void setup()
{
  serialEsp.begin(9600);
  Serial.begin(115200);  

  Serial.println("EasyIoTEsp init");

  esp.begin(incomingMessage, 3, &serialEsp, &Serial);
  //esp.begin(incomingMessage, 3, &serialEsp);

  pinMode(RELAY_1, OUTPUT);

  esp.present(1, S_DIGITAL_OUTPUT);
}

void loop()
{
  esp.process();
}

void incomingMessage(const Esp8266EasyIoTMsg &message) {
  // We only expect one type of message from controller. But we better check anyway.

  Serial.println("New message");
  if (message.type==V_DIGITAL_VALUE) {
    // Change relay state
    digitalWrite(message.sensor-1+RELAY_1, message.getBool()?RELAY_ON:RELAY_OFF);

    Serial.print("Incoming change for sensor:");
    Serial.print(message.sensor);
    Serial.print(", New status: ");
    Serial.println(message.getBool());
  } 
}