/*
 V1.0 - first version
 
 Created by Igor Jarc <admin@iot-playground.com>
 See http://iot-playground.com for details
 Do not contact avtor dirrectly, use community forum on website.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "Irrigation.h"
#include "Arduino.h"


Irrigation::Irrigation(){
	for (int i=0; i<MAX_IRRIGATORS; i++) {  
		state[i] = s_idle;
		pumpMotor[i] = false;
		startTime[i] = millis();
		soilHum[i] = 0; 
		autoMode[i] = false;
		soilHumidityTreshold[i] = 0;
		irrigatorCounter[i] = IRRIGATION_PAUSE_TIME;
		humReportCnt[i] = HUM_REPORT_DELAY;
  }
}


void Irrigation::begin(int (* aiReadCallback)(int), void (* humiditySetCallback)(int, int), void (* motorSetCallback)(int, bool))
{
	_aiReadCallback = aiReadCallback;
	_motorSetCallback = motorSetCallback;
	_humiditySetCallback = humiditySetCallback;

	for (int i=0; i<MAX_IRRIGATORS; i++) {  
		_motorSetCallback(i, false);
	}

	for (int i=0; i<MAX_IRRIGATORS; i++) { 
		if (_aiReadCallback != NULL)
			lastAnalogReading[i] = _aiReadCallback(i);
		soilHum[i] = mapAiToPercent(lastAnalogReading[i]); 
	}
}


int Irrigation::mapAiToPercent(int in)
{
  int out = map(in, MIN_ANALOG_VAL, MAX_ANALOG_VAL, 0, 100);            
   // limit to 0-100%
   out = max(0, out);
   out = min(100, out);
   return out;
}

void Irrigation::process(){
	for (int i=0; i<MAX_IRRIGATORS; i++) {  
		processsingle(i);
  }
}


void Irrigation::processsingle(int i){
  if (IsTimeout(i))
  {
    startTime[i] = millis();

	if (humReportCnt[i]++ > HUM_REPORT_DELAY)
	{
		humReportCnt[i] = 0;

		int aireading = 0;
		if (_aiReadCallback != NULL)
			aireading = _aiReadCallback(i);

		// filter 
		lastAnalogReading[i] += (aireading - lastAnalogReading[i]) / 10;  
   
	   // calculate soil humidity in % 
	   int newSoilHum = mapAiToPercent(lastAnalogReading[i]);  
      
	   // report soil humidity if changed
	   if (soilHum[i] != newSoilHum)
	   {
		 soilHum[i] = newSoilHum;
		 if (_humiditySetCallback != NULL)
			_humiditySetCallback(i, soilHum[i]);
	   }
	}


   // irrigator state machine
   switch(state[i])
   {
     case s_idle:     
       if (irrigatorCounter[i] <= IRRIGATION_PAUSE_TIME)
         irrigatorCounter[i]++;

       if ((irrigatorCounter[i] >= IRRIGATION_PAUSE_TIME) && autoMode[i])
       {
         if (soilHum[i] < soilHumidityTreshold[i])
           state[i] = s_irrigation_start;       
       }         
       break;
     case s_irrigation_start:
       irrigatorCounter[i] = 0;
	   if (_motorSetCallback != NULL)
		_motorSetCallback(i, true);
       state[i] = s_irrigation;
       break;
     case s_irrigation:
       if (irrigatorCounter[i]++ > IRRIGATION_TIME)
         state[i] = s_irrigation_stop;
       break;
     case s_irrigation_stop:
       irrigatorCounter[i] = 0;
       state[i] = s_idle;
	   if (_motorSetCallback != NULL)
	   _motorSetCallback(i, false);
       break;
   }
  }
}


void Irrigation::setAutoMode(int i, bool enable){
  if ((i>=0) && (i<MAX_IRRIGATORS))
  {
    autoMode[i] = enable;
  }
}

void Irrigation::setIrrigate(int i, bool enable){
  if ((i>=0) && (i<MAX_IRRIGATORS))
  {
	if (enable==true)
	    state[i] = s_irrigation_start;
    else
		state[i] = s_irrigation_stop;
  }
}


void Irrigation::setSoilHumidityTreshold(int i, int treshold){
  if ((i>=0) && (i<MAX_IRRIGATORS))
  {
    soilHumidityTreshold[i] = treshold;
  }
}


int Irrigation::getIrrigatorsCount()
{
	return MAX_IRRIGATORS;
}

bool Irrigation::IsTimeout(int i)
{
	if ((i>=0) && (i<MAX_IRRIGATORS))
	{
		unsigned long now = millis();
		if (startTime[i] <= now)
		{
			if ( (unsigned long)(now - startTime[i] )  < MS_IN_SEC ) 
				return false;
		}
		else
		{
			if ( (unsigned long)(startTime[i] - now) < MS_IN_SEC ) 
				return false;
		}

		return true;
	}
	return false;
}