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



#define PWM_OUT  13  // 
#define FADE_DELAY 10 


static int currLevel = 0; 
static int newLevel = 0;
Esp8266EasyIoTMsg dimmerMsg(0, V_DIMMER);
Esp8266EasyIoTMsg lightMsg(0, V_DIGITAL_VALUE);


void setup()
{
  serialEsp.begin(9600);
  
  //Serial1.begin(9600); // ESP
  Serial.begin(115200);  

  Serial.println("EasyIoTEsp LED dimmer example init");

  esp.begin(incomingMessage, 3, &serialEsp, &Serial);
  //esp.begin(incomingMessage, 12, &Serial1, &Serial);
  //esp.begin(incomingMessage, 3, &serialEsp);
  
  esp.present(0, S_DIMMER );
  // request last dimmer value
  esp.request( 0, V_DIMMER );
}

void loop()
{
  if (esp.process())
  {
    if (newLevel != currLevel)
    {
      int dir = ( newLevel - currLevel ) < 0 ? -1 : 1;
      currLevel += dir;
      analogWrite( PWM_OUT, (int)(currLevel / 100. * 255) );
      
      if (newLevel == currLevel)
      {
        esp.send(lightMsg.set(newLevel > 0 ? 1 : 0));
        esp.send( dimmerMsg.set(newLevel) );
      }
      delay( FADE_DELAY );
    }
  }
}

void incomingMessage(const Esp8266EasyIoTMsg &message) {
  Serial.println("New message");
  if (message.type==V_DIGITAL_VALUE || message.type == V_DIMMER) {
    newLevel = message.getInt();
    
    newLevel *= ( message.type == V_DIGITAL_VALUE ? 100 : 1 );
    
    newLevel = min(100, newLevel);
    newLevel = max(0, newLevel);
    
    
    Serial.print("Old level:");
    Serial.print(currLevel);
    Serial.print(", New level: ");
    Serial.println(newLevel);
  } 
}

void fadeToLevel(int newLevel)
{
  int dir = ( newLevel - currLevel ) < 0 ? -1 : 1;
  
  while ( currLevel != newLevel ) {
    currLevel += dir;
    analogWrite( PWM_OUT, (int)(currLevel / 100. * 255) );
    delay( FADE_DELAY );
  }
}
