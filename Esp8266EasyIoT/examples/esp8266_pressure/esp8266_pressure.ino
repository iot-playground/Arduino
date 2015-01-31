 /*
 V1.0 - first version
 
 Created by Igor Jarc <igor.jarc1@gmail.com>
 See http://iot-playground.com for details
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <Esp8266EasyIoT.h>
#include <SFE_BMP180.h>
#include <Wire.h>


#define ALTITUDE 301.0 // Altitude of my home
#define ESP_RESET_PIN     12

#define MILS_IN_MIN  60000

#define CHILD_ID_TEMP        0
#define CHILD_ID_BARO        1


int minuteCount = 0;
double pressureSamples[9][6];
double pressureAvg[9];
double dP_dt;

const char *weather[] = {
  "stable","sunny","cloudy","unstable","thunderstorm","unknown"};

int forecast = 5;

unsigned long startTime;

SFE_BMP180 bmp180;
Esp8266EasyIoT esp; 


Esp8266EasyIoTMsg msgTemp(CHILD_ID_TEMP, V_TEMP);
Esp8266EasyIoTMsg msgPress(CHILD_ID_BARO, V_PRESSURE);
Esp8266EasyIoTMsg msgForec(CHILD_ID_BARO, V_FORECAST);

void setup()
{  
  Serial1.begin(9600); // ESP
  Serial.begin(115200); // debug

  if (bmp180.begin())
    Serial.println("BMP180 init success");
   else
     {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
   
   startTime =  -1;
    
  esp.begin(NULL, ESP_RESET_PIN, &Serial1, &Serial);

  esp.present(CHILD_ID_TEMP, S_TEMP);
  esp.present(CHILD_ID_BARO, S_BARO);
}


void loop()
{
  
  for(int i =0; i<10;i++)
  {
    if (esp.process())
      break;
  }
  
  
  if (IsTimeout())
  {
  char status;
  double T,P,p0,a;

  // Loop here getting pressure readings every 60 seconds.

  // If you want sea-level-compensated pressure, as used in weather reports,
  // you will need to know the altitude at which your measurements are taken.
  // We're using a constant called ALTITUDE in this sketch:
  
  // If you want to measure altitude, and not pressure, you will instead need
  // to provide a known baseline pressure. This is shown at the end of the sketch.

  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = bmp180.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = bmp180.getTemperature(T);
    if (status != 0)
    {
      // Print out the measurement:
      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.print(" deg C, ");
      Serial.print((9.0/5.0)*T+32.0,2);
      Serial.println(" deg F");
      
           
      static int lastSendTempInt;
      int temp = round(T *10);
          
      if (temp != lastSendTempInt)
      {
        lastSendTempInt = temp;      
        esp.send(msgTemp.set((float)T, 1));
      }
            
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = bmp180.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = bmp180.getPressure(P,T);
        if (status != 0)
        {
          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          p0 = bmp180.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.print(" mb, ");
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");
          
          
          static int lastSendPresInt;
          int pres = round(p0 *10);
          
          if (pres != lastSendPresInt)
          {
            lastSendPresInt = pres;      
            esp.send(msgPress.set((float)p0, 1));
          }
          
          forecast = calculateForecast(p0);
          static int lastSendForeInt = -1;
          
      
          if (forecast != lastSendForeInt)
          {
            lastSendForeInt = forecast;      
            esp.send(msgForec.set(weather[forecast]));
          }
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");

  startTime = millis();  
}

  //delay(5000);  // Pause for 5 seconds.
}

boolean IsTimeout()
{
  unsigned long now = millis();
  if (startTime <= now)
  {
    if ( (unsigned long)(now - startTime )  < MILS_IN_MIN ) 
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime - now) < MILS_IN_MIN ) 
      return false;
  }

  return true;
}


int calculateForecast(double pressure) {
  //From 0 to 5 min.
  if (minuteCount <= 5){
    pressureSamples[0][minuteCount] = pressure;
  }
  //From 30 to 35 min.
  else if ((minuteCount >= 30) && (minuteCount <= 35)){
    pressureSamples[1][minuteCount - 30] = pressure;  
  }
  //From 60 to 65 min.
  else if ((minuteCount >= 60) && (minuteCount <= 65)){
    pressureSamples[2][minuteCount - 60] = pressure;  
  }  
  //From 90 to 95 min.
  else if ((minuteCount >= 90) && (minuteCount <= 95)){
    pressureSamples[3][minuteCount - 90] = pressure;  
  }
  //From 120 to 125 min.
  else if ((minuteCount >= 120) && (minuteCount <= 125)){
    pressureSamples[4][minuteCount - 120] = pressure;  
  }
  //From 150 to 155 min.
  else if ((minuteCount >= 150) && (minuteCount <= 155)){
    pressureSamples[5][minuteCount - 150] = pressure;  
  }
  //From 180 to 185 min.
  else if ((minuteCount >= 180) && (minuteCount <= 185)){
    pressureSamples[6][minuteCount - 180] = pressure;  
  }
  //From 210 to 215 min.
  else if ((minuteCount >= 210) && (minuteCount <= 215)){
    pressureSamples[7][minuteCount - 210] = pressure;  
  }
  //From 240 to 245 min.
  else if ((minuteCount >= 240) && (minuteCount <= 245)){
    pressureSamples[8][minuteCount - 240] = pressure;  
  }


  if (minuteCount == 5) {
    // Avg pressure in first 5 min, value averaged from 0 to 5 min.
    pressureAvg[0] = ((pressureSamples[0][0] + pressureSamples[0][1] 
      + pressureSamples[0][2] + pressureSamples[0][3]
      + pressureSamples[0][4] + pressureSamples[0][5]) / 6);
  } 
  else if (minuteCount == 35) {
    // Avg pressure in 30 min, value averaged from 0 to 5 min.
    pressureAvg[1] = ((pressureSamples[1][0] + pressureSamples[1][1] 
      + pressureSamples[1][2] + pressureSamples[1][3]
      + pressureSamples[1][4] + pressureSamples[1][5]) / 6);
    float change = (pressureAvg[1] - pressureAvg[0]);
      dP_dt = change / 5; 
  } 
  else if (minuteCount == 65) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[2] = ((pressureSamples[2][0] + pressureSamples[2][1] 
      + pressureSamples[2][2] + pressureSamples[2][3]
      + pressureSamples[2][4] + pressureSamples[2][5]) / 6);
    float change = (pressureAvg[2] - pressureAvg[0]);
      dP_dt = change / 10; 
  } 
  else if (minuteCount == 95) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[3] = ((pressureSamples[3][0] + pressureSamples[3][1] 
      + pressureSamples[3][2] + pressureSamples[3][3]
      + pressureSamples[3][4] + pressureSamples[3][5]) / 6);
    float change = (pressureAvg[3] - pressureAvg[0]);
    dP_dt = change / 15; 
  } 
  else if (minuteCount == 125) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[4] = ((pressureSamples[4][0] + pressureSamples[4][1] 
      + pressureSamples[4][2] + pressureSamples[4][3]
      + pressureSamples[4][4] + pressureSamples[4][5]) / 6);
    float change = (pressureAvg[4] - pressureAvg[0]);
    dP_dt = change / 20; 
  } 
  else if (minuteCount == 155) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[5] = ((pressureSamples[5][0] + pressureSamples[5][1] 
      + pressureSamples[5][2] + pressureSamples[5][3]
      + pressureSamples[5][4] + pressureSamples[5][5]) / 6);
    float change = (pressureAvg[5] - pressureAvg[0]);
    dP_dt = change / 25; 
  } 
  else if (minuteCount == 185) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[6] = ((pressureSamples[6][0] + pressureSamples[6][1] 
      + pressureSamples[6][2] + pressureSamples[6][3]
      + pressureSamples[6][4] + pressureSamples[6][5]) / 6);
    float change = (pressureAvg[6] - pressureAvg[0]);
      dP_dt = change / 30; 
  }
  else if (minuteCount == 215) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[7] = ((pressureSamples[7][0] + pressureSamples[7][1] 
      + pressureSamples[7][2] + pressureSamples[7][3]
      + pressureSamples[7][4] + pressureSamples[7][5]) / 6);
    float change = (pressureAvg[7] - pressureAvg[0]);
      dP_dt = change / 35; 
  } 
  else if (minuteCount == 245) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    pressureAvg[8] = ((pressureSamples[8][0] + pressureSamples[8][1] 
      + pressureSamples[8][2] + pressureSamples[8][3]
      + pressureSamples[8][4] + pressureSamples[8][5]) / 6);
    float change = (pressureAvg[8] - pressureAvg[0]);
      dP_dt = change / 40; // note this is for t = 4 hour
        
    minuteCount -= 30;
    pressureAvg[0] = pressureAvg[1];
    pressureAvg[1] = pressureAvg[2];
    pressureAvg[2] = pressureAvg[3];
    pressureAvg[3] = pressureAvg[4];
    pressureAvg[4] = pressureAvg[5];
    pressureAvg[5] = pressureAvg[6];
    pressureAvg[6] = pressureAvg[7];
    pressureAvg[7] = pressureAvg[8];
  } 

  minuteCount++;

  if (minuteCount < 36) //if time is less than 35 min 
    return 5; // Unknown, more time needed
  else if (dP_dt < (-0.25))
    return 4; // Quickly falling LP, Thunderstorm, not stable
  else if (dP_dt > 0.25)
    return 3; // Quickly rising HP, not stable weather
  else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
    return 2; // Slowly falling Low Pressure System, stable rainy weather
  else if ((dP_dt > 0.05) && (dP_dt < 0.25))
    return 1; // Slowly rising HP stable good weather
  else if ((dP_dt > (-0.05)) && (dP_dt < 0.05))
    return 0; // Stable weather
  else
    return 5; // Unknown
}

