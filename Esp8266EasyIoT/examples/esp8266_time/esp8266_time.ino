 /*
 V1.0 - first version
 
 Created by Igor Jarc <igor.jarc1@gmail.com>
 See http://iot-playground.com for details
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <UTFT.h>
#include <UTouch.h>
#include <Time.h>
#include <Esp8266EasyIoT.h>
#include <Timezone.h>
#include <DS1302RTC.h>

#include <SevenSegNumFontPlus.h>
extern uint8_t SevenSegNumFontPlus[];


#define ESP_RESET_PIN     12

// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Arduino Uno / 2009:
// -------------------
// Standard Arduino Uno/2009 shield            : <display model>,A5,A4,A3,A2
// DisplayModule Arduino Uno TFT shield        : <display model>,A5,A4,A3,A2
//
// Arduino Mega:
// -------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Mega      : <display model>,38,39,40,41
//
// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ITDB32S,38,39,40,41);

// Set pins:  CE, IO,CLK
DS1302RTC RTC(11, 10, 9);


// change if you are not in CET
//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {
  "CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {
  "CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone tzc(CEST, CET);


Esp8266EasyIoT esp; 

boolean timeReceived = false;

void setup()
{  
  // init LSCD
  myGLCD.InitLCD();
  myGLCD.clrScr();

  
  // ESP serial
  Serial1.begin(9600);
  // debug serial
  Serial.begin(115200);

  // init ESP
  esp.begin(NULL, ESP_RESET_PIN, &Serial1, &Serial);
  esp.present(0, S_DIGITAL_INPUT);
  
  setSyncProvider(RTC.get);
  Serial.println("requesting time");
  esp.requestTime(receiveTime);    
  
  displayTime();
}


void loop()
{
  esp.process();

  unsigned long now = millis();
  static unsigned long lastRequest;
   // If no time has been received yet, request it every 10 second from controller
  // When time has been received, request update every hour
  if ((!timeReceived && now-lastRequest > 10*1000)
    || (timeReceived && now-lastRequest > 60*1000*60)) {
    // Request time from controller. 
    Serial.println("requesting time");
    esp.requestTime(receiveTime);  
    lastRequest = now;
  }

  displayTime();
}

// This is called when a new time value was received
void receiveTime(unsigned long controllerTime) {
  // Ok, set incoming time 
  Serial.print("Time value received: ");
  Serial.println(controllerTime);
  RTC.set(controllerTime); // this sets the RTC to the time from controller - which we do want periodically
  timeReceived = true;
}


void displayTime()
{
  char Dbuf1[60];	// all the rest for building messages with sprintf
  char Dbuf[60];

  time_t t = timelocal(now());
  static time_t tLast;
  
  if (tLast != t)
  {
    tLast = t;
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);

    myGLCD.setFont(SevenSegNumFontPlus);  

    strcpy_P(Dbuf1,PSTR("%02d:%02d"));
    sprintf(Dbuf, Dbuf1, hour(t), minute(t)); 

    myGLCD.print(Dbuf, 0, 0);
  }
}


time_t timelocal(time_t utctime) {
  time_t rt;
  rt = tzc.toLocal(utctime);
  //if(year(rt) < 2036) {
  return(rt);
  //    }
  //    else {
  //	return(RTC.get());
  //    }
}


