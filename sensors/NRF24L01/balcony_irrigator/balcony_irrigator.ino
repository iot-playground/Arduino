/*
 V1.0 - first version
 
 Created by Igor Jarc <admin@iot-playground.com>
 See http://iot-playground.com for details
 Do not contact avtor dirrectly, use community forum on website.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <Irrigation.h>
#include <SPI.h>
#include <MySensor.h>  


#define CHILD_ID_SWITCH_IRRIGATE        0
#define CHILD_ID_AUTO_MODE              1
#define CHILD_ID_SOIL_HUMIDITY          2
#define CHILD_ID_SOIL_HUMIDITY_AO       3

#define SENORS_PER_IRRIGATOR            4
#define START_DO_PIN                    2

MySensor mys;
Irrigation irr;


// the setup function runs once when you press reset or power the board
void setup() {
  
  for (int i=0; i < irr.getIrrigatorsCount(); i++) {
    pinMode(START_DO_PIN + i, OUTPUT);
    digitalWrite(START_DO_PIN + i, HIGH);
  }
  
  
  mys.begin(incomingMessage, AUTO, true);
  
  for (int i=0; i < irr.getIrrigatorsCount(); i++) {      
    mys.present(CHILD_ID_SWITCH_IRRIGATE + SENORS_PER_IRRIGATOR * i, S_LIGHT);   // irrigation mottor switch    
    delay500();
    mys.present(CHILD_ID_AUTO_MODE + SENORS_PER_IRRIGATOR * i, S_LIGHT);         // irrigator mode - 1 auto, 0 - manual
    delay500();
    mys.present(CHILD_ID_SOIL_HUMIDITY + SENORS_PER_IRRIGATOR * i, S_HUM);       // soil humidity
    delay500();
    mys.present(CHILD_ID_SOIL_HUMIDITY_AO + SENORS_PER_IRRIGATOR * i, S_HUM);    // soil humidity treshold    - change later in EasyIoT server UI to humidity output
    delay500();      
  }


  irr.begin(&readAi, &sendHumidity, &setMotor);

  // request treshold humidity
  for (int i=0; i < irr.getIrrigatorsCount(); i++) {      
    mys.request(CHILD_ID_SOIL_HUMIDITY_AO + SENORS_PER_IRRIGATOR * i, V_HUM);
    delay500();
  }

  // request auto mode
  for (int i=0; i < irr.getIrrigatorsCount(); i++) {      
    mys.request(CHILD_ID_AUTO_MODE + SENORS_PER_IRRIGATOR * i, V_LIGHT);
    delay500();
  }  
}

// the loop function runs over and over again forever
void loop() {
  mys.process();
  irr.process();
}


void delay500()
{
  for (int i=0; i < 50; i++) { 
    delay(10);
    mys.process();
  }
}


int  readAi(int i)
{
  if (i < irr.getIrrigatorsCount())
  {
    if (i==0)
      return analogRead(A0);
    else if (i==1)
      return analogRead(A1);
    else if (i==2)
      return analogRead(A2);
    else if (i==3)
      return analogRead(A3);
    else if (i==4)
      return analogRead(A4);    
    else if (i==5)
      return analogRead(A5);    
    else if (i==6)
      return analogRead(A6);    
  }
  
  return 0;
}


void sendHumidity(int i, int humidity)
{
  if (i < irr.getIrrigatorsCount())
  {
      MyMessage msgHum(CHILD_ID_SOIL_HUMIDITY + SENORS_PER_IRRIGATOR * i, V_HUM);
      mys.send(msgHum.set(humidity, 1));
  }
}


void setMotor(int i, bool state)
{
  if (i < irr.getIrrigatorsCount())
  {
    digitalWrite(START_DO_PIN + i, state?LOW:HIGH);
    
    MyMessage msgState(CHILD_ID_SWITCH_IRRIGATE + SENORS_PER_IRRIGATOR * i, V_LIGHT);
    mys.send(msgState.set(state));
  }
}


void incomingMessage(const MyMessage &message) {
  Serial.println("");
  Serial.print("Incoming change for sensor:");
  Serial.println(message.sensor);

  int irrigator = message.sensor / SENORS_PER_IRRIGATOR; 
  int sensor = message.sensor % SENORS_PER_IRRIGATOR; 

  if (message.type == V_LIGHT) {    
    // process mode commnd
    if (sensor == CHILD_ID_AUTO_MODE)
    {
      irr.setAutoMode(irrigator,  message.getBool());
    }
    
    // process motor switch command  
    else if (sensor == CHILD_ID_SWITCH_IRRIGATE)        
    {
      irr.setIrrigate(irrigator, message.getBool()); 
    }
  }else if (message.type == V_HUM && sensor == CHILD_ID_SOIL_HUMIDITY_AO) {    
    // set humidity treshold for automatic irrigation
    irr.setSoilHumidityTreshold(irrigator, message.getInt());
  }  
}
