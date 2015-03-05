 /*
 V1.0 - first version
 
 Created by Igor Jarc <admin@iot-playground.com>
 See http://iot-playground.com for details
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */
#include <UTFT.h>
#include <UTouch.h>
#include <avr/eeprom.h>
#include <Time.h>
#include "DHT.h"
#include <DS1302RTC.h>
#include <Esp8266EasyIoT.h>
#include <Timezone.h>
#include <SFE_BMP180.h>
#include <Wire.h>

#include <SevenSegNumFontPlus.h>
#include <SevenSeg_XXXL_Num.h>
#include <arial_normal.h>



// Declare which fonts we will be using
extern uint8_t SevenSegNumFontPlus[];
extern uint8_t ArialNumFontPlus[];
extern uint8_t SevenSeg_XXXL_Num[];
extern uint8_t arial_normal[];

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

// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
UTouch  myTouch( 6, 5, 4, 3, 2);


// Set pins:  CE, IO,CLK
DS1302RTC RTC(11, 10, 9);


#define DHT22_PIN         8
#define ESP_RESET_PIN     12
#define RELAY_PIN         13

#define RELAY_ON 0  // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay


#define HEATER_MODE_OFF   0 
#define HEATER_MODE_AUTO  1
#define HEATER_MODE_LOLO  2
#define HEATER_MODE_LO    3
#define HEATER_MODE_HI    4
#define HEATER_MODE_HIHI  5

//int heaterMode = HEATER_MODE_OFF;
#define TEMPERATURE_READ_INTERVAL  3  // in s
#define MILS_IN_SEC  1000  



#define SCREEN_MAIN           0 
#define SCREEN_SETTINGS       1 
#define SCREEN_SET_DATETIME   2 
#define SCREEN_TEMPWERATURE   3 
#define SCREEN_SCHEDULE       4 

int screen = SCREEN_MAIN;

#define TEMP_STEP       0.5 
#define TEPM_HISTERESIS 0.4
#define MAX_SCHEDULES   10

int _rec = 0;


#define DAYS_ALL        0
#define DAYS_WEEK       1
#define DAYS_WEEKEND    2
#define DAYS_MO         3
#define DAYS_TU         4
#define DAYS_WE         5
#define DAYS_TH         6
#define DAYS_FR         7
#define DAYS_SA         8
#define DAYS_SU         9

#define CONFIG_REVISION 12353L
#define EEPROM_CONFIG_OFFSET 2


#define CHILD_ID_HUM         0 // room humidity
#define CHILD_ID_TEMP        1 // room temperature
#define CHILD_ID_BARO        2 // room pressure
#define CHILD_ID_TEMP1       3 // temperature 1
#define CHILD_ID_TEMP2       4 // temperature 2
#define CHILD_ID_HUM1        5 // humidity 1
#define CHILD_ID_MODE_OFF    6 
#define CHILD_ID_MODE_AUTO   7
#define CHILD_ID_MODE_LOLO   8
#define CHILD_ID_MODE_LO     9
#define CHILD_ID_MODE_HI     10
#define CHILD_ID_MODE_HIHI   11
#define CHILD_ID_TEMP_LOLO   12 // set temperature LOLO
#define CHILD_ID_TEMP_LO     13 // set temperature LO
#define CHILD_ID_TEMP_HI     14 // set temperature HI
#define CHILD_ID_TEMP_HIHI   15 // set temperature HIHI
#define CHILD_ID_HEATING     16 // DI heating 

char *days[DAYS_SU+1] = {
  "All      ", "Weekdays ", "Weekend  ", "Monday   ", "Tuesday  ", "Wednesday", "Thursday ", "Friday   ", "Saturday ", "Sunday   "};


char Dbuf1[60];	// all the rest for building messages with sprintf
char Dbuf[60];

typedef struct s_shedule{
  boolean enabled;
  unsigned char days;
  unsigned char hourStart;
  unsigned char minuteStart;
  unsigned char hourStop;
  unsigned char minuteStop;
  unsigned char temp;  
};

typedef struct s_config{
  long  config_revision;
  int heaterMode;
  float temp_lolo;
  float temp_lo;
  float temp_hi;
  float temp_hihi;
  s_shedule schedules[MAX_SCHEDULES];
}
config;


config configuration;

// change if you are not in CET
//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {
  "CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {
  "CET ", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone tzc(CEST, CET);


unsigned char tmpHour;
unsigned char tmpMinute;
unsigned char tmpDay;
unsigned char tmpMonth;
unsigned int tmpYear;

DHT dht;
Esp8266EasyIoT esp; 

int cnt;

boolean clock;
float temperature = 22;
float humidity = 50;


float temperature1 = 22;
float humidity1 = 50;

float temperature2 = 22;


// forecast
#define FORECAST_CALC_INTERVAL     60 // 60s 
int forecastInterval;
int minuteCount = 0;
double pressureSamples[9][6];
double pressureAvg[9];
//bool firstRound = true;
double dP_dt;
const char *weather[] = {
  "stable      ","sunny       ","cloudy      ","unstable    ","thunderstorm","unknown     "};
double pressureF;
SFE_BMP180 bmp180;
#define ALTITUDE 301.0 // Altitude of my home
int forecast = 5;
boolean heaterStatus;


#define TMPERATURE_NO_TARGET     -9999
float temperatureTarget = TMPERATURE_NO_TARGET;
boolean heating;

unsigned long startTime;

boolean timeReceived = false;


Esp8266EasyIoTMsg msgHum(CHILD_ID_HUM, V_HUM);
Esp8266EasyIoTMsg msgTemp(CHILD_ID_TEMP, V_TEMP);
Esp8266EasyIoTMsg msgPress(CHILD_ID_BARO, V_PRESSURE);
Esp8266EasyIoTMsg msgForec(CHILD_ID_BARO, V_FORECAST);

// mode
Esp8266EasyIoTMsg msgModeOff(CHILD_ID_MODE_OFF, V_DIGITAL_VALUE);
Esp8266EasyIoTMsg msgModeAuto(CHILD_ID_MODE_AUTO, V_DIGITAL_VALUE);
Esp8266EasyIoTMsg msgModeLoLo(CHILD_ID_MODE_LOLO, V_DIGITAL_VALUE);
Esp8266EasyIoTMsg msgModeLo(CHILD_ID_MODE_LO, V_DIGITAL_VALUE);
Esp8266EasyIoTMsg msgModeHi(CHILD_ID_MODE_HI, V_DIGITAL_VALUE);
Esp8266EasyIoTMsg msgModeHiHi(CHILD_ID_MODE_HIHI, V_DIGITAL_VALUE);

//set temparature
Esp8266EasyIoTMsg msgTempLoLo(CHILD_ID_TEMP_LOLO, V_TEMP);
Esp8266EasyIoTMsg msgTempLo(CHILD_ID_TEMP_LO, V_TEMP);
Esp8266EasyIoTMsg msgTempHi(CHILD_ID_TEMP_HI, V_TEMP);
Esp8266EasyIoTMsg msgTempHiHi(CHILD_ID_TEMP_HIHI, V_TEMP);

Esp8266EasyIoTMsg msgHeaterStatus(CHILD_ID_HEATING, V_DIGITAL_VALUE);

void setup()
{  
  pinMode(RELAY_PIN, OUTPUT);
  
  Serial1.begin(9600);

  bmp180.begin();

  Serial.begin(115200);

  // DHT22
  dht.setup(DHT22_PIN); // data pin 8

  // init ESP
  esp.begin(incomingMessage, ESP_RESET_PIN, &Serial1, &Serial);
 
  eeprom_read_block((void*)&configuration, (void*)EEPROM_CONFIG_OFFSET, sizeof(configuration));

  if (configuration.config_revision != CONFIG_REVISION)
  {  // default values
    configuration.config_revision = CONFIG_REVISION;
    configuration.heaterMode = HEATER_MODE_OFF;
    configuration.temp_lolo = 19;
    configuration.temp_lo = 20;
    configuration.temp_hi = 21;
    configuration.temp_hihi = 22;

    for(int i = 0; i < MAX_SCHEDULES;i++)
    {
      configuration.schedules[i].enabled = false;        
      configuration.schedules[i].days = DAYS_ALL;
      configuration.schedules[i].hourStart = 0;
      configuration.schedules[i].minuteStart = 0;
      configuration.schedules[i].hourStop = 0;
      configuration.schedules[i].minuteStop = 0;     
      configuration.schedules[i].temp = HEATER_MODE_HI;   
    }

    SaveConfiguration();
  }

  myGLCD.InitLCD();
  myGLCD.clrScr();

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);

  setSyncProvider(RTC.get);
  
  esp.requestTime(receiveTime);  

  DrawScreenMain();
  startTime = millis();
  cnt = TEMPERATURE_READ_INTERVAL+1;    


  forecastInterval = FORECAST_CALC_INTERVAL + 1;

  // present to server
  esp.present(CHILD_ID_HUM, S_HUM);
  esp.present(CHILD_ID_TEMP, S_TEMP);
  esp.present(CHILD_ID_BARO, S_BARO);
  
  esp.present(CHILD_ID_TEMP1, S_TEMP_AO);
  esp.present(CHILD_ID_TEMP2, S_TEMP_AO);
  esp.present(CHILD_ID_HUM1, S_HUM_AO);
  
  esp.present(CHILD_ID_MODE_OFF, S_DIGITAL_OUTPUT);
  esp.present(CHILD_ID_MODE_AUTO, S_DIGITAL_OUTPUT);
  esp.present(CHILD_ID_MODE_LOLO, S_DIGITAL_OUTPUT);
  esp.present(CHILD_ID_MODE_LO, S_DIGITAL_OUTPUT);
  esp.present(CHILD_ID_MODE_HI, S_DIGITAL_OUTPUT);
  esp.present(CHILD_ID_MODE_HIHI, S_DIGITAL_OUTPUT);
  
  esp.present(CHILD_ID_TEMP_LOLO, S_TEMP_AO);
  esp.present(CHILD_ID_TEMP_LO, S_TEMP_AO);
  esp.present(CHILD_ID_TEMP_HI, S_TEMP_AO);
  esp.present(CHILD_ID_TEMP_HIHI, S_TEMP_AO);
  
  
  esp.present(CHILD_ID_HEATING, S_DIGITAL_INPUT);
  
  
  SendModeStatus();  
  
   
  // send temperatures
  esp.send(msgTempLoLo.set(configuration.temp_lolo, 1));
  esp.send(msgTempLo.set(configuration.temp_lo, 1));
  esp.send(msgTempHi.set(configuration.temp_hi, 1));
  esp.send(msgTempHiHi.set(configuration.temp_hihi, 1));
    
  // send relay status
  
  // request temperatures
  wait();
  esp.request(CHILD_ID_TEMP1, V_TEMP);
  esp.request(CHILD_ID_TEMP2, V_TEMP);
  wait();
  esp.request(CHILD_ID_HUM1, V_HUM);
  wait();
  esp.send(msgHeaterStatus.set((uint8_t)heaterStatus));
 }

void wait()
{
  for(int i =0; i<50;i++)
  {
    esp.process();
    delay(10);
  }
}

void loop()
{
  for(int i =0; i<10;i++)
  {
    if (esp.process())
      break;
  }
  
  unsigned long now = millis();
  static unsigned long lastRequest;
   // If no time has been received yet, request it every 10 second from controller
  // When time has been received, request update every hour
  if ((!timeReceived && now-lastRequest > 10*1000)
    || (timeReceived && now-lastRequest > 60*1000*60)) {
    // Request time from controller. 
    esp.requestTime(receiveTime);  
    lastRequest = now;
  }
  
  
  // handle buttons
  if (screen == SCREEN_MAIN)
  {
    ScreenMainButtonHandler();    
  }
  else if (screen == SCREEN_SETTINGS)
  {
    ScreenSettingsButtonHandler();
  }
  else if (screen == SCREEN_TEMPWERATURE)
  {
    ScreenTemperatureButtonHandler();
  }
  else if (screen == SCREEN_SCHEDULE)
  {
    ScreenScheduleButtonHandler();
  }
  else if (screen == SCREEN_SET_DATETIME)
  {
    ScreenDatetimeeButtonHandler();
  }

  if (IsTimeout())
  {
    cnt++ ;
    forecastInterval++;  
    startTime = millis();
    DrawTime(10,110, false);    
  }

  if (forecastInterval > FORECAST_CALC_INTERVAL)
  {
    forecastInterval = 0;  

    ReadPressure();
    forecast = calculateForecast(pressureF);
    
    static int lastSendPresInt;
    int pres = round(pressureF *10);
      
    if (pres != lastSendPresInt)
    {
      lastSendPresInt = pres;      
      esp.send(msgPress.set((float)pressureF, 1));
    }
    
    static int lastSendForeInt = -1;
      
    if (forecast != lastSendForeInt)
    {
      lastSendForeInt = forecast;      
      esp.send(msgForec.set(weather[forecast]));
    }
    
    DrawPressure(false);
    DrawForecast(false);
  }

  if (cnt > TEMPERATURE_READ_INTERVAL)
  {
    cnt = 0;

    float humidityReading = dht.getHumidity();
    float tempReading = dht.getTemperature();

    if (!isnan(humidityReading)) 
    {
      if (humidityReading <= 100)
        humidity = humidityReading;
    } 
    
    static int lastSendHumInt;
    int hum = round(humidity);
      
    if (hum != lastSendHumInt)
    {
      lastSendHumInt = hum;      
      esp.send(msgHum.set(humidity, 0));
    }  
      
    if (isnan(tempReading)) 
    {
      Serial.println("Failed reading temperatur from DHT");
    }
    else
    {
      if (abs(temperature - tempReading) > 10)
        temperature += tempReading/16.0 - temperature/16.0;
      else    
        temperature += tempReading/4.0 - temperature/4.0;
        
      
      static int lastSendTempInt;
      int temp = round(temperature *10);
      
      if (temp != lastSendTempInt)
      {
        lastSendTempInt = temp;      
        esp.send(msgTemp.set(temperature, 1));
      }

      //temperature = 23.4;
      DrawCurrentTemp(10, 30, temperature, humidity);

      UpdateSetTemp();
      DrawModeButton(220, 10, false);
      UpdateHeaterStatus();
    }
  }
}


// This is called when a new time value was received
void receiveTime(unsigned long controllerTime) {
  // Ok, set incoming time 
  Serial.print("Time value received: ");
  Serial.println(controllerTime);
  RTC.set(controllerTime); // this sets the RTC to the time from controller - which we do want periodically
  timeReceived = true;
}

void ReadPressure()
{
  //pressureF++;
  char status;
  double T,P,p0,a;

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
          pressureF = bmp180.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
        }
      }
    }
  }
}


void incomingMessage(const Esp8266EasyIoTMsg &message) {
   Serial.println("New message");
        
   if (message.type==V_TEMP) { 
     if (message.sensor==CHILD_ID_TEMP_LOLO){
       configuration.temp_lolo = message.getFloat();
       configuration.temp_lolo = (configuration.temp_lolo > configuration.temp_lo)?configuration.temp_lo:configuration.temp_lolo;
       UpdateSetTemp();
       UpdateHeaterStatus();
       // custom ack
         
       Esp8266EasyIoTMsg msgnew;
       msgnew = message;
       esp.setNewMsg(msgnew.set(configuration.temp_lolo, 1));
       //esp.send(msgTempLoLo.set(configuration.temp_lolo, 1));
       SaveConfiguration();
     }
     else if (message.sensor==CHILD_ID_TEMP_LO){
       configuration.temp_lo = message.getFloat();
       configuration.temp_lo = (configuration.temp_lo > configuration.temp_hi)?configuration.temp_hi:configuration.temp_lo;
       configuration.temp_lo = (configuration.temp_lo < configuration.temp_lolo)?configuration.temp_lolo:configuration.temp_lo;
       UpdateSetTemp();
       UpdateHeaterStatus();
       
       Esp8266EasyIoTMsg msgnew;
       msgnew = message;
       esp.setNewMsg(msgnew.set(configuration.temp_lo, 1));
       //esp.send(msgTempLo.set(configuration.temp_lo, 1));
       SaveConfiguration();
     }
     else if (message.sensor==CHILD_ID_TEMP_HI){
       configuration.temp_hi = message.getFloat();
       configuration.temp_hi = (configuration.temp_hi > configuration.temp_hihi)?configuration.temp_hihi:configuration.temp_hi;
       configuration.temp_hi = (configuration.temp_hi < configuration.temp_lo)?configuration.temp_lo:configuration.temp_hi;
       UpdateSetTemp();
       UpdateHeaterStatus();
       
       Esp8266EasyIoTMsg msgnew;
       msgnew = message;
       esp.setNewMsg(msgnew.set(configuration.temp_hi, 1));
       //esp.send(msgTempHi.set(configuration.temp_hi, 1));
       SaveConfiguration();
     }
     else if (message.sensor==CHILD_ID_TEMP_HIHI){
       configuration.temp_hihi = message.getFloat();
       configuration.temp_hihi = (configuration.temp_hihi < configuration.temp_hi)?configuration.temp_hi:configuration.temp_hihi;
       UpdateSetTemp();
       UpdateHeaterStatus();
       
       Esp8266EasyIoTMsg msgnew;
       msgnew = message;
       esp.setNewMsg(msgnew.set(configuration.temp_hihi, 1));
       //esp.send(msgTempHiHi.set(configuration.temp_hihi, 1));
       SaveConfiguration();
     }

     else if (message.sensor==CHILD_ID_TEMP1){
       temperature1 = message.getFloat();
       DrawTemp1(false);
     }
     else if (message.sensor==CHILD_ID_TEMP2){
       temperature2 = message.getFloat();
       DrawTemp2(false);
     }
   }
   // heater mode
   else if ((message.sensor==CHILD_ID_MODE_OFF) && (message.type==V_DIGITAL_VALUE))
   {
    configuration.heaterMode = HEATER_MODE_OFF;
    UpdateSetTemp();
    DrawModeButton(true);
    UpdateHeaterStatus();
    SendModeStatus();
    SaveConfiguration();
   }
   else if ((message.sensor==CHILD_ID_MODE_AUTO) && (message.type==V_DIGITAL_VALUE))
   {
     configuration.heaterMode = HEATER_MODE_AUTO;
     UpdateSetTemp();
     DrawModeButton(true);
     UpdateHeaterStatus();
     SendModeStatus();
     SaveConfiguration();
   }
   else if ((message.sensor==CHILD_ID_MODE_LOLO) && (message.type==V_DIGITAL_VALUE))
   {
     configuration.heaterMode = HEATER_MODE_LOLO;
     UpdateSetTemp();
     DrawModeButton(true);
     UpdateHeaterStatus();
     SendModeStatus();
     SaveConfiguration();
   }
   else if ((message.sensor==CHILD_ID_MODE_LO) && (message.type==V_DIGITAL_VALUE))
   {
     configuration.heaterMode = HEATER_MODE_LO;
     UpdateSetTemp();
     DrawModeButton(true);
     UpdateHeaterStatus();
     SendModeStatus();
     SaveConfiguration();
   }
   else if ((message.sensor==CHILD_ID_MODE_HI) && (message.type==V_DIGITAL_VALUE))
   {
     configuration.heaterMode = HEATER_MODE_HI;
     UpdateSetTemp();
     DrawModeButton(true);
     UpdateHeaterStatus();
     SendModeStatus();
     SaveConfiguration();
   }
   else if ((message.sensor==CHILD_ID_MODE_HIHI) && (message.type==V_DIGITAL_VALUE)) 
   {
     configuration.heaterMode = HEATER_MODE_HIHI;
     UpdateSetTemp();
     DrawModeButton(true);
     UpdateHeaterStatus();
     SendModeStatus();
     SaveConfiguration();
   }
   // measuured temperatures and humidity
   else if (message.sensor==CHILD_ID_HUM1 && message.type==V_HUM) {
       humidity1 = message.getFloat();
       DrawTemp1(false);
   }
}


void SendModeStatus()
{
  if (configuration.heaterMode == HEATER_MODE_OFF)
  {
    esp.send(msgModeOff.set((uint8_t)1));
    esp.send(msgModeAuto.set((uint8_t)0));
    esp.send(msgModeLoLo.set((uint8_t)0));
    esp.send(msgModeLo.set((uint8_t)0));
    esp.send(msgModeHi.set((uint8_t)0));
    esp.send(msgModeHiHi.set((uint8_t)0));
  }
  else if (configuration.heaterMode == HEATER_MODE_AUTO)
  {
    esp.send(msgModeOff.set((uint8_t)0));
    esp.send(msgModeAuto.set((uint8_t)1));
    esp.send(msgModeLoLo.set((uint8_t)0));
    esp.send(msgModeLo.set((uint8_t)0));
    esp.send(msgModeHi.set((uint8_t)0));
    esp.send(msgModeHiHi.set((uint8_t)0));
  }
  else if (configuration.heaterMode == HEATER_MODE_LOLO)
  {
    esp.send(msgModeOff.set((uint8_t)0));
    esp.send(msgModeAuto.set((uint8_t)0));
    esp.send(msgModeLoLo.set((uint8_t)1));
    esp.send(msgModeLo.set((uint8_t)0));
    esp.send(msgModeHi.set((uint8_t)0));
    esp.send(msgModeHiHi.set((uint8_t)0));
  }
  else if (configuration.heaterMode == HEATER_MODE_LO)
  {
    esp.send(msgModeOff.set((uint8_t)0));
    esp.send(msgModeAuto.set((uint8_t)0));
    esp.send(msgModeLoLo.set((uint8_t)0));
    esp.send(msgModeLo.set((uint8_t)1));
    esp.send(msgModeHi.set((uint8_t)0));
    esp.send(msgModeHiHi.set((uint8_t)0));
  }
  else if (configuration.heaterMode == HEATER_MODE_HI)
  {
    esp.send(msgModeOff.set((uint8_t)0));
    esp.send(msgModeAuto.set((uint8_t)0));
    esp.send(msgModeLoLo.set((uint8_t)0));
    esp.send(msgModeLo.set((uint8_t)0));
    esp.send(msgModeHi.set((uint8_t)1));
    esp.send(msgModeHiHi.set((uint8_t)0));
  }
  else if (configuration.heaterMode == HEATER_MODE_HIHI)
  {
    esp.send(msgModeOff.set((uint8_t)0));
    esp.send(msgModeAuto.set((uint8_t)0));
    esp.send(msgModeLoLo.set((uint8_t)0));
    esp.send(msgModeLo.set((uint8_t)0));
    esp.send(msgModeHi.set((uint8_t)0));
    esp.send(msgModeHiHi.set((uint8_t)1));
  }
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

  // http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf table 4

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




boolean IsTimeout()
{
  unsigned long now = millis();
  if (startTime <= now)
  {
    if ( (unsigned long)(now - startTime )  < MILS_IN_SEC ) 
      return false;
  }
  else
  {
    if ( (unsigned long)(startTime - now) < MILS_IN_SEC ) 
      return false;
  }

  return true;
}

void SaveConfiguration()
{
  config tmpconfig;
  eeprom_read_block((void*)&tmpconfig, (void*)EEPROM_CONFIG_OFFSET, sizeof(tmpconfig));

  byte *b1;
  byte *b2;

  b1 = (byte *)&tmpconfig;
  b2 = (byte *)&configuration;

  // store in EEPROM only if changed
  for(int i = 0; i < sizeof(configuration);i++)
  {
    if (*b1 != *b2)
    {
      eeprom_write_block((const void*)&configuration, (void*)EEPROM_CONFIG_OFFSET, sizeof(configuration));
      //save = true;
      break;
    }
    b1++;
    b2++;
  }
}

void WaitTouch()
{
  delay(300);
  while(myTouch.dataAvailable())
    myTouch.read();
}


// Draw a red frame while a button is touched
void DrawButtonPressed(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
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


void UpdateHeaterStatus()
{
  static boolean oldHeaterStatus;
  
  if (temperature < (temperatureTarget - TEPM_HISTERESIS / 2))
    heaterStatus = true;
  else if (temperature > (temperatureTarget + TEPM_HISTERESIS / 2))
    heaterStatus = false;
  else
    heaterStatus = oldHeaterStatus;


  if (screen == SCREEN_MAIN)
  {
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.setFont(arial_normal);

    if (heaterStatus)
      myGLCD.print("ON ", 160, 10);
    else
      myGLCD.print("OFF", 160, 10);
  }
  
  // set heater status
  digitalWrite(RELAY_PIN, heaterStatus?RELAY_ON:RELAY_OFF);
  
  if (oldHeaterStatus != heaterStatus)
  {
    oldHeaterStatus = heaterStatus;    
    esp.send(msgHeaterStatus.set((uint8_t)heaterStatus));
  }
}


void UpdateSetTemp()
{
  if (configuration.heaterMode == HEATER_MODE_OFF)
  {
    temperatureTarget = TMPERATURE_NO_TARGET;
  }
  else if (configuration.heaterMode == HEATER_MODE_AUTO)
  {
    temperatureTarget = TMPERATURE_NO_TARGET;

    time_t t = timelocal(now());

    for(int i = 0; i < MAX_SCHEDULES;i++)
    {
      if (configuration.schedules[i].enabled)
      {
        int wd = weekday(t);      
        boolean found = false;

        if ((configuration.schedules[i].days == DAYS_ALL) ||
          (configuration.schedules[i].days == DAYS_WEEK) && (wd >=2 && wd <=6) ||
          (configuration.schedules[i].days == DAYS_WEEKEND) && (wd ==1 || wd == 7) ||
          (configuration.schedules[i].days == DAYS_MO) && (wd == 2) ||
          (configuration.schedules[i].days == DAYS_TU) && (wd == 3) ||
          (configuration.schedules[i].days == DAYS_WE) && (wd == 4) ||
          (configuration.schedules[i].days == DAYS_TH) && (wd == 5) ||
          (configuration.schedules[i].days == DAYS_FR) && (wd == 6) ||
          (configuration.schedules[i].days == DAYS_SA) && (wd == 7) ||
          (configuration.schedules[i].days == DAYS_SU) && (wd == 1))
        {         
          if ((configuration.schedules[i].hourStart == configuration.schedules[i].hourStop) && (configuration.schedules[i].minuteStart == configuration.schedules[i].minuteStop))
            continue;
          else if ((configuration.schedules[i].hourStart < configuration.schedules[i].hourStop) || (configuration.schedules[i].hourStart == configuration.schedules[i].hourStop) && (configuration.schedules[i].minuteStart < configuration.schedules[i].minuteStop))
          {
            if ((configuration.schedules[i].hourStart < hour(t) || ((configuration.schedules[i].hourStart == hour(t)) && (configuration.schedules[i].minuteStart <= minute(t)))) &&
              (((hour(t) < configuration.schedules[i].hourStop) || ((hour(t) == configuration.schedules[i].hourStop) && (minute(t) <= configuration.schedules[i].minuteStop)))))
            {
              found = true;
            }
          }
          else
          {
            if ((hour(t) > configuration.schedules[i].hourStart || hour(t) == configuration.schedules[i].hourStart && minute(t) >= configuration.schedules[i].minuteStart) ||
              ( hour(t) < configuration.schedules[i].hourStop || hour(t) == configuration.schedules[i].hourStop && minute(t) <= configuration.schedules[i].minuteStop))
            {
              found = true;
            }
          }

          if (found)
          {
            float settemp = configuration.temp_lolo;

            if (configuration.schedules[i].temp == HEATER_MODE_LOLO)
              settemp = configuration.temp_lolo;
            else if (configuration.schedules[i].temp == HEATER_MODE_LO)
              settemp = configuration.temp_lo;
            else if (configuration.schedules[i].temp == HEATER_MODE_HI)
              settemp = configuration.temp_hi;
            else if (configuration.schedules[i].temp == HEATER_MODE_HIHI)
              settemp = configuration.temp_hihi;              

            if (settemp > temperatureTarget)
              temperatureTarget = settemp;
          }          
        }
      }
    }
  }
  else if (configuration.heaterMode == HEATER_MODE_LOLO)
  {
    temperatureTarget = configuration.temp_lolo;
  }
  else if (configuration.heaterMode == HEATER_MODE_LO)
  {
    temperatureTarget = configuration.temp_lo;
  }
  else if (configuration.heaterMode == HEATER_MODE_HI)
  {
    temperatureTarget = configuration.temp_hi;
  }
  else if (configuration.heaterMode == HEATER_MODE_HIHI)
  {
    temperatureTarget = configuration.temp_hihi;
  }
}

/////////////////////////////////////
// begin Main screen
/////////////////////////////////////
void DrawScreenMain()
{
  screen = SCREEN_MAIN;

  myGLCD.clrScr();

  DrawHeaterStatus(10, 10);
  DrawCurrentTemp(10, 30, temperature, humidity);

  UpdateSetTemp();
  DrawModeButton(220, 10, true);  
  UpdateHeaterStatus();
  DrawSettingsButton(220, 70);
  DrawTime(10,110, true);


  DrawTemp1(true);
  DrawTemp2(true);
  DrawPressure(true);
  DrawForecast(true);
}

void DrawTemp1(boolean forceRedraw)
{
  static int lastHumidity1;
  static float lastTemperature1;
  int humidityI = round(humidity1);
  if ((humidityI != lastHumidity1) || (temperature1 != lastTemperature1) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastHumidity1 = humidityI;
      lastTemperature1 = temperature1;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      char str_temp[6];
      dtostrf(temperature1, 5, 1, str_temp);
      strcpy_P(Dbuf1,PSTR("Bedroom:%s'C %d%%"));
      sprintf(Dbuf, Dbuf1, str_temp, humidityI); 

      myGLCD.print(Dbuf, 10, 180);
    }
  }
}



void DrawTemp2(boolean forceRedraw)
{
  static float lastTemperature2;
  if ((temperature2 != lastTemperature2) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastTemperature2 = temperature2;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      char str_temp[6];
      dtostrf(temperature2, 5, 1, str_temp);
      strcpy_P(Dbuf1,PSTR("Outdoor:%s'C"));
      sprintf(Dbuf, Dbuf1, str_temp); 

      myGLCD.print(Dbuf, 10, 200);
    }
  }
}

void DrawPressure(boolean forceRedraw)
{
  static int lastPressure;
  int pressuseI = round(pressureF);
  if ((pressuseI != lastPressure) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastPressure = pressuseI;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      strcpy_P(Dbuf1,PSTR("%dmb"));
      sprintf(Dbuf, Dbuf1, pressuseI); 

      myGLCD.print(Dbuf, 10, 220);
    }
  }
}


void DrawForecast(boolean forceRedraw)
{
  static int lastForecast;
  if ((lastForecast != forecast) || forceRedraw)
  {
    if (screen == SCREEN_MAIN)
    {
      lastForecast = forecast;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);
      myGLCD.setFont(arial_normal);

      myGLCD.print(weather[forecast], 122, 220);
    }
  }
}

void DrawModeButton(boolean forceDraw)
{
  DrawModeButton(220, 10, forceDraw);
}
void DrawModeButton(int x, int y, boolean forceDraw)
{
  if (screen == SCREEN_MAIN)
  {
    static float oldtemperatureTarget;

    if ((oldtemperatureTarget != temperatureTarget) || forceDraw)
    {
      oldtemperatureTarget = temperatureTarget;

      myGLCD.setBackColor(0, 255, 0);
      myGLCD.setColor(0, 255, 0);
      myGLCD.fillRoundRect (x, y, x+90, y+45);

      myGLCD.setColor(255, 255, 255);
      myGLCD.drawRoundRect  (x, y, x+90, y+45);
      myGLCD.setColor(0, 0, 0);
      myGLCD.setFont(arial_normal);
      if (configuration.heaterMode == HEATER_MODE_OFF)
      {
        myGLCD.print("OFF", x+5, y+5);
        myGLCD.setColor(0, 255, 0);
        myGLCD.fillRect  (x+1, y+25, x+80, y+20);
      }
      else if (configuration.heaterMode == HEATER_MODE_AUTO)
      {
        myGLCD.print("AUTO", x+5, y+5);
        if (temperatureTarget != TMPERATURE_NO_TARGET)
          myGLCD.printNumF(temperatureTarget,1,x+1, y+25,46,5); 
      }
      else if (configuration.heaterMode == HEATER_MODE_LOLO)
      {
        myGLCD.print("LOLO", x+5, y+5);
        myGLCD.printNumF(temperatureTarget,1,x+1, y+25,46,5); 
      }
      else if (configuration.heaterMode == HEATER_MODE_LO)
      {
        myGLCD.print("LOW", x+5, y+5);
        myGLCD.printNumF(temperatureTarget,1,x+1, y+25,46,5); 
      }
      else if (configuration.heaterMode == HEATER_MODE_HI)
      {
        myGLCD.print("HIGH", x+5, y+5);
        myGLCD.printNumF(temperatureTarget,1,x+1, y+25,46,5); 
      }
      else if (configuration.heaterMode == HEATER_MODE_HIHI)
      {
        myGLCD.print("HIHI", x+5, y+5);  
        myGLCD.printNumF(temperatureTarget,1,x+1, y+25,46,5); 
      }
    }
  }
}

void DrawSettingsButton(int x, int y)
{
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (x, y, x+90, y+45);

  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect  (x, y, x+90, y+45);
  myGLCD.setColor(0, 0, 0);
  myGLCD.setFont(arial_normal);
  myGLCD.print("Sett", x+5, y+15);
}


void DrawHeaterStatus(int x, int y)
{
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 0);

  myGLCD.setFont(arial_normal);
  myGLCD.print("Indoor", x, y);

  //myGLCD.print("OFF", x+150, y);
}
void DrawCurrentTemp(int x, int y, float temp, float hum)
{
  if (screen == SCREEN_MAIN)
  {
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);

    myGLCD.setFont(arial_normal);

    int tmp1 = trunc(temp);
    int tmp2 = trunc(temp * 10 - tmp1 * 10); 
    float tmp3 = temp *100 - tmp1 *100  - tmp2*10;

    if (tmp3 > 5)
    {
      tmp2++;

      if (tmp2>9)
      {
        tmp1++;
        tmp2 = 0;
      }
    }

    myGLCD.setFont(ArialNumFontPlus);
    myGLCD.printNumI(tmp1, x+20, y);
    myGLCD.printNumI(tmp2, x+ 100, y);
    //myGLCD.setFont(arial_normal);
    //myGLCD.print("o", x+130, y);
    myGLCD.fillCircle(x+137, y+4, 4);
    y += 36;
    //myGLCD.print("o", x+85, y);
    myGLCD.fillCircle(x+92, y+7, 4);

    myGLCD.setFont(arial_normal);
    y += 24;
    myGLCD.print("Hum.:", x, y);
    tmp1 = round(hum);
    myGLCD.printNumI(tmp1,x+100, y);
    myGLCD.print("%", x+140, y);
  }
}

void DrawTime(int x, int y, boolean force)
{
  static time_t tLast;
  //time_t utc = now();
  //time_t t = tzc.toLocal(t);
  time_t t = timelocal(now());




  if (minute(t) != minute(tLast))
  {
    UpdateSetTemp();
    DrawModeButton(220, 10, false);
    UpdateHeaterStatus();
  }
  if (screen == SCREEN_MAIN)
  {
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 0);

    myGLCD.setFont(SevenSegNumFontPlus);  

    clock = !clock;
    if (clock)
      myGLCD.print(":", x+2*32, y);
    else
    {
      myGLCD.setColor(0, 0, 0);
      myGLCD.fillRect (x+2*32, y, x+3*32-1, y+50);
    }

    if ((minute(t) != minute(tLast)) || force)
    {
      tLast = t;
      myGLCD.setColor(0, 255, 0);
      myGLCD.setBackColor(0, 0, 0);

      strcpy_P(Dbuf1,PSTR("%02d"));
      sprintf(Dbuf, Dbuf1, hour(t)); 

      myGLCD.print(Dbuf, x, y);
      sprintf(Dbuf, Dbuf1, minute(t)); 
      myGLCD.print(Dbuf, x+3*32, y);

      y +=50; 

      myGLCD.setFont(arial_normal);

      strcpy_P(Dbuf1,PSTR("%s %02d.%02d.%04d"));
      sprintf(Dbuf, Dbuf1, dayShortStr(weekday(t)), day(t), month(t), year(t)); 

      myGLCD.print(Dbuf, x, y);
      //myGLCD.print("MO 18.2.2015", x, y);
    }
  }
}


void ScreenMainButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x=myTouch.getX();
    int y=myTouch.getY();

    int xpos = 220;
    int ypos = 10;

    if ((x > xpos) && (x < (xpos + 90)) && (y > 10) && (y < (ypos + 45)))
    {
      configuration.heaterMode += 1;
      configuration.heaterMode = (configuration.heaterMode > HEATER_MODE_HIHI)? HEATER_MODE_OFF: configuration.heaterMode;  
      SaveConfiguration();

      DrawButtonPressed(xpos, ypos, xpos+90, ypos+45);      
      UpdateSetTemp();
      DrawModeButton(xpos, ypos, true);         
      UpdateHeaterStatus();
      SendModeStatus();
      WaitTouch();
    }
    else
    {
      xpos = 220;
      ypos = 70;

      if ((x > xpos) && (x < (xpos + 90)) && (y > 10) && (y < (ypos + 45)))
      {
        DrawButtonPressed(xpos, ypos, xpos+90, ypos+45);
        WaitTouch();
        DrawScreenSettings();
      }
    }
  }
}

/////////////////////////////////////
// end Main screen
/////////////////////////////////////


/////////////////////////////////////
// begin temperature screen
/////////////////////////////////////
void DrawScreenTemperature()
{
  screen = SCREEN_TEMPWERATURE;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);

  int x = 20;
  int y = 10;

  int s = 50;

  myGLCD.print("LOLO:", x, y);

  myGLCD.print("LOW:",  x, y + s);

  myGLCD.print("HIGH:", x, y  + s * 2);

  myGLCD.print("HIHI:", x, y  + s * 3);

  DrawTeemperatureTemp();

  int t = 45;
  int u = 250;
  int k;

  for (k=0; k<4; k++)
  {
    u = 250;
    myGLCD.setBackColor(0, 255, 0);
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect (u, y+k*t, u+t, (k+1)*t);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (u, y+k*t, u+t, (k+1)*t);
    myGLCD.setColor(0, 0, 0);
    myGLCD.print("+", u+15, y+k*t+10);

    u = 200;
    myGLCD.setColor(0, 255, 0);
    myGLCD.fillRoundRect (u, y+k*t, u+t, (k+1)*t);
    myGLCD.setColor(255, 255, 255);
    myGLCD.drawRoundRect (u, y+k*t, u+t, (k+1)*t);
    myGLCD.setColor(0, 0, 0);
    myGLCD.print("-", u+15, y+k*t+10);
  }

  u = 200;
  k =4;
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRoundRect (u, y+k*t, u+t*2, (k+1)*t);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (u, y+k*t, u+t*2, (k+1)*t);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", u+15, y+k*t+10);
}

void DrawTeemperatureTemp()
{
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);
  int x = 20;
  int y = 10;
  int s = 50;

  DrawScreenTemperatureTemp(x+75, y, configuration.temp_lolo);
  DrawScreenTemperatureTemp(x+75, y + s, configuration.temp_lo);
  DrawScreenTemperatureTemp(x+75, y  + s * 2, configuration.temp_hi);
  DrawScreenTemperatureTemp(x+75, y  + s * 3, configuration.temp_hihi);
}

void DrawScreenTemperatureTemp(int x, int y, float temp)
{
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.setFont(arial_normal);

  myGLCD.printNumF(temp,1,x,y,46,5);    // Print Floating Number   1 decimal places, x=20, y=170, 46 = ".", 8 characters, 48= "0" (filler)
}

// sceen temperature button handler
void ScreenTemperatureButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x=myTouch.getX();
    int y=myTouch.getY();

    // exit button

    int xpos = 20;
    int ypos = 10;

    int s = 50;  
    int t = 45;
    int u = 250;
    int k;

    u = 200;
    k =4;

    // back button              
    if ((x > u) && (x < (u+t*2)) && (y > (ypos+k*t)) && (y < ((k+1)*t)))
    {
      DrawButtonPressed(u, ypos+k*t, u+t*2, (k+1)*t);
      WaitTouch();
      SaveConfiguration();
      DrawScreenSettings();
    }

    // increase decrease temp button handler  
    for (k=0; k<4; k++)
    {
      // increase       
      u = 250;      
      if ((x > u) && (x < (u+t)) && (y > (ypos+k*t)) && (y < ((k+1)*t)))
      {
        if (k == 0)
        {  
          configuration.temp_lolo += TEMP_STEP; 
          configuration.temp_lolo = (configuration.temp_lolo > configuration.temp_lo)?configuration.temp_lo:configuration.temp_lolo;
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempLoLo.set(configuration.temp_lolo, 1));
        }
        else if (k == 1)
        {
          configuration.temp_lo += TEMP_STEP;
          configuration.temp_lo = (configuration.temp_lo > configuration.temp_hi)?configuration.temp_hi:configuration.temp_lo;
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempLo.set(configuration.temp_lo, 1));
        }
        else if (k == 2)
        {
          configuration.temp_hi += TEMP_STEP;
          configuration.temp_hi = (configuration.temp_hi > configuration.temp_hihi)?configuration.temp_hihi:configuration.temp_hi;
          UpdateSetTemp();
          UpdateHeaterStatus();          
          esp.send(msgTempHi.set(configuration.temp_hi, 1));
        }
        else if (k == 3)
        {
          configuration.temp_hihi += TEMP_STEP;
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempHiHi.set(configuration.temp_hihi, 1));
        }

        DrawTeemperatureTemp();

        DrawButtonPressed(u, ypos+k*t, u+t, (k+1)*t);
        WaitTouch();
      }

      // decrease
      u = 200; 
      if ((x > u) && (x < (u+t)) && (y > (ypos+k*t)) && (y < ((k+1)*t)))
      {
        if (k == 0)
        {  
          configuration.temp_lolo -= TEMP_STEP; 
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempLoLo.set(configuration.temp_lolo, 1));
        }
        else if (k == 1)
        {
          configuration.temp_lo -= TEMP_STEP;
          configuration.temp_lo = (configuration.temp_lo < configuration.temp_lolo)?configuration.temp_lolo:configuration.temp_lo;
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempLo.set(configuration.temp_lo, 1));
        }
        else if (k == 2)
        {
          configuration.temp_hi -= TEMP_STEP;
          configuration.temp_hi = (configuration.temp_hi < configuration.temp_lo)?configuration.temp_lo:configuration.temp_hi;
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempHi.set(configuration.temp_hi, 1));
        }
        else if (k == 3)
        {
          configuration.temp_hihi -= TEMP_STEP;
          configuration.temp_hihi = (configuration.temp_hihi < configuration.temp_hi)?configuration.temp_hi:configuration.temp_hihi;
          UpdateSetTemp();
          UpdateHeaterStatus();
          esp.send(msgTempHiHi.set(configuration.temp_hihi, 1));
        }

        DrawTeemperatureTemp();

        DrawButtonPressed(u, ypos+k*t, u+t, (k+1)*t);
        WaitTouch();
      }
    }

  }
}
/////////////////////////////////////
// end temperature screen
/////////////////////////////////////


/////////////////////////////////////
// begin settings screen
/////////////////////////////////////

void DrawScreenSettings()
{
  screen = SCREEN_SETTINGS;
  _rec = 0;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);

  int xpos = 20;
  int ypos = 15;
  int s=45;
  int xsize = 280;


  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Date/Time", xpos+20, ypos+15);

  ypos += s + 10; 

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Temperature", xpos+20, ypos+15);

  ypos += s + 10; 

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Schedule", xpos+20, ypos+15);

  ypos += s + 10; 

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + s);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", xpos+20, ypos+15);
}


void ScreenSettingsButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x=myTouch.getX();
    int y=myTouch.getY();

    int xpos = 20;
    int ypos = 15;
    int ysize=45;
    int xsize = 280;

    // date/time
    if ((x > xpos) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);      
      WaitTouch();
      DrawScreenDatetime();
    }

    ypos += ysize + 10; 
    // temperature
    if ((x > xpos) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);            
      WaitTouch();
      DrawScreenTemperature();
    }

    ypos += ysize + 10; 
    // schedule
    if ((x > xpos) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);      
      WaitTouch();
      DrawScreenSchedule();
    }

    ypos += ysize + 10; 
    // back
    if ((x > xpos) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);   
      WaitTouch();
      DrawScreenMain();  
    }
  }
}

/////////////////////////////////////
// end settings screen
/////////////////////////////////////



/////////////////////////////////////
// begin schedule screen
/////////////////////////////////////

void DrawScreenSchedule()
{
  drawScreenSchedule();
  drawScreenSchedule(_rec);
}

void drawScreenSchedule(int rec)
{
  myGLCD.setFont(arial_normal);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  int xpos = 0;
  int ypos = 15;
  int ysize = 30;
  int xsize = 280;

  if (configuration.schedules[_rec].enabled == true)
    myGLCD.print("On ", xpos+120, ypos+5);
  else
    myGLCD.print("Off", xpos+120, ypos+5);

  ypos += ysize + 5;

  myGLCD.print(days[configuration.schedules[_rec].days], xpos+120, ypos+5);

  ypos += ysize + 5;
  strcpy_P(Dbuf1,PSTR("%02d:%02d"));
  sprintf(Dbuf, Dbuf1, configuration.schedules[_rec].hourStart, configuration.schedules[_rec].minuteStart); 
  myGLCD.print(Dbuf, xpos+120, ypos+5);

  ypos += ysize + 5;
  //strcpy_P(Dbuf1,PSTR("%02d:%02d"));
  sprintf(Dbuf, Dbuf1, configuration.schedules[_rec].hourStop, configuration.schedules[_rec].minuteStop); 
  myGLCD.print(Dbuf, xpos+120, ypos+5);


  ypos += ysize + 5;
  if (configuration.schedules[_rec].temp == HEATER_MODE_LOLO)
    myGLCD.print("LOLO", xpos+120, ypos+5);
  else if (configuration.schedules[_rec].temp == HEATER_MODE_LO)
    myGLCD.print("LOW ", xpos+120, ypos+5);
  else if (configuration.schedules[_rec].temp == HEATER_MODE_HI)
    myGLCD.print("HIGH ", xpos+120, ypos+5);
  else if (configuration.schedules[_rec].temp == HEATER_MODE_HIHI)
    myGLCD.print("HIHI ", xpos+120, ypos+5);


  ypos += ysize + 5;
  myGLCD.printNumI((_rec + 1), xpos+120, ypos+5);
}

void drawScreenSchedule()
{
  screen = SCREEN_SCHEDULE;
  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);


  int xpos = 20;
  int ypos = 15;
  int ysize = 30;
  int xsize = 280;


  // enabled
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);  
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Active:", xpos-10, ypos+5);


  ypos += ysize + 5;

  // days
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Days:", xpos-10,ypos+5);

  ypos += ysize + 5;

  // satrt
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("M", xpos+xsize-ysize+7, ypos+7);
  myGLCD.print("H", xpos+xsize-ysize*2-5+7, ypos+7);

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Start:", xpos-10, ypos+5);

  ypos += ysize + 5;

  // stop
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("M", xpos+xsize-ysize+7, ypos+7);
  myGLCD.print("H", xpos+xsize-ysize*2-5+7, ypos+7);

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Stop:", xpos-10, ypos+5);

  ypos += ysize + 5;

  // temp
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);  
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Temp:", xpos-10, ypos+5);


  ypos += ysize + 5;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print(">", xpos+xsize-ysize+7, ypos+7);
  myGLCD.print("<", xpos+xsize-ysize*2-5+7, ypos+7);

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);
  myGLCD.print("Rec:", xpos-10, ypos+5);
}


void ScreenScheduleButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x=myTouch.getX();
    int y=myTouch.getY();


    int xpos = 20;
    int ypos = 15;
    int ysize = 30;
    int xsize = 280;

    if ((x > (xpos+xsize-ysize)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);      
      WaitTouch();
      configuration.schedules[_rec].enabled = !configuration.schedules[_rec].enabled;
      drawScreenSchedule(_rec);     
      UpdateSetTemp();
      UpdateHeaterStatus(); 
    }


    ypos += ysize + 5;

    if ((x > (xpos+xsize-ysize)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].days = (++configuration.schedules[_rec].days > DAYS_SU)? DAYS_ALL:configuration.schedules[_rec].days;
      drawScreenSchedule(_rec);      

      UpdateSetTemp();
      UpdateHeaterStatus(); 
    }


    ypos += ysize + 5;

    if ((x > (xpos+xsize-ysize)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].minuteStart = (++configuration.schedules[_rec].minuteStart > 59)? 0:configuration.schedules[_rec].minuteStart;
      drawScreenSchedule(_rec);      

      UpdateSetTemp();
      UpdateHeaterStatus(); 
    }

    if ((x > (xpos+xsize-ysize*2-5)) && (x < (xpos+xsize-ysize-5)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].hourStart = (++configuration.schedules[_rec].hourStart > 23)? 0:configuration.schedules[_rec].hourStart;
      drawScreenSchedule(_rec);

      UpdateSetTemp();
      UpdateHeaterStatus();       
    }

    ypos += ysize + 5;

    if ((x > (xpos+xsize-ysize)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].minuteStop = (++configuration.schedules[_rec].minuteStop > 59)? 0:configuration.schedules[_rec].minuteStop;
      drawScreenSchedule(_rec);      

      UpdateSetTemp();
      UpdateHeaterStatus();       
    }

    if ((x > (xpos+xsize-ysize*2-5)) && (x < (xpos+xsize-ysize-5)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].hourStop = (++configuration.schedules[_rec].hourStop > 23)? 0:configuration.schedules[_rec].hourStop;
      drawScreenSchedule(_rec);      

      UpdateSetTemp();
      UpdateHeaterStatus();       
    }


    ypos += ysize + 5;

    if ((x > (xpos+xsize-ysize)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
      WaitTouch();
      configuration.schedules[_rec].temp = (++configuration.schedules[_rec].temp > HEATER_MODE_HIHI)? HEATER_MODE_LOLO:configuration.schedules[_rec].temp;
      drawScreenSchedule(_rec);    

      UpdateSetTemp();
      UpdateHeaterStatus();       
    }


    ypos += ysize + 5;

    if ((x > (xpos+xsize-ysize)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize, ypos, xpos+xsize, ypos + ysize);
      WaitTouch();

      if (++_rec >= MAX_SCHEDULES)
      {
        SaveConfiguration();
        DrawScreenSettings();
      }
      else
        drawScreenSchedule(_rec);      
    }

    if ((x > (xpos+xsize-ysize*2-5)) && (x < (xpos+xsize-ysize-5)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize-ysize*2-5, ypos, xpos+xsize-ysize-5, ypos + ysize);
      WaitTouch();
      if (--_rec < 0)
      {
        SaveConfiguration();
        DrawScreenSettings();
      }
      else
        drawScreenSchedule(_rec);      
    }


  }
}

/////////////////////////////////////
// end schedule screen
/////////////////////////////////////


/////////////////////////////////////
// begin datetime screen
/////////////////////////////////////


void DrawScreenDatetime()
{
  time_t t = timelocal(now());

  screen = SCREEN_SET_DATETIME;

  tmpHour = hour(t);
  tmpMinute = minute(t);

  tmpMonth = month(t);
  tmpDay = day(t);
  tmpYear = year(t);


  myGLCD.clrScr();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);

  myGLCD.setFont(arial_normal);


  int xpos = 20;
  int ypos = 15;
  int ysize = 30;
  int xsize = 30;


  // time  
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);  
  myGLCD.print("Time:", xpos, ypos+5);

  ypos += ysize + 5;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize+10, ypos, xpos+xsize*2+10, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize*2+40, ypos, xpos+xsize*3+40, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize*3+50, ypos, xpos+xsize*4+50, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + ysize);  
  myGLCD.drawRoundRect (xpos+xsize+10, ypos, xpos+xsize*2+10, ypos + ysize);  
  myGLCD.drawRoundRect (xpos+xsize*2+40, ypos, xpos+xsize*3+40, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize*3+50, ypos, xpos+xsize*4+50, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos+7, ypos+7);
  myGLCD.print("+", xpos+xsize+10+7, ypos+7);
  myGLCD.print("-", xpos+xsize*2+40+7, ypos+7);
  myGLCD.print("+", xpos+xsize*3+50+7, ypos+7);

  ypos += ysize + 5;
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0);  
  myGLCD.print("Date:", xpos, ypos+5);

  ypos += ysize + 5;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize+10, ypos, xpos+xsize*2+10, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize*2+40, ypos, xpos+xsize*3+40, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize*3+50, ypos, xpos+xsize*4+50, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize*4+80, ypos, xpos+xsize*5+80, ypos + ysize);
  myGLCD.fillRoundRect (xpos+xsize*5+90, ypos, xpos+xsize*6+90, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + ysize);  
  myGLCD.drawRoundRect (xpos+xsize+10, ypos, xpos+xsize*2+10, ypos + ysize);  
  myGLCD.drawRoundRect (xpos+xsize*2+40, ypos, xpos+xsize*3+40, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize*3+50, ypos, xpos+xsize*4+50, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize*4+80, ypos, xpos+xsize*5+80, ypos + ysize);
  myGLCD.drawRoundRect (xpos+xsize*5+90, ypos, xpos+xsize*6+90, ypos + ysize);

  myGLCD.setColor(0, 0, 0);
  myGLCD.print("-", xpos+7, ypos+7);
  myGLCD.print("+", xpos+xsize+10+7, ypos+7);
  myGLCD.print("-", xpos+xsize*2+40+7, ypos+7);
  myGLCD.print("+", xpos+xsize*3+50+7, ypos+7);
  myGLCD.print("-", xpos+xsize*4+80+7, ypos+7);
  myGLCD.print("+", xpos+xsize*5+90+7, ypos+7);


  ypos += ysize + 40;
  xsize = 280;

  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setColor(0, 255, 0);   
  myGLCD.fillRoundRect (xpos, ypos, xpos+xsize, ypos + ysize);
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (xpos, ypos, xpos+xsize, ypos + ysize);
  myGLCD.setColor(0, 0, 0);
  myGLCD.print("Back", xpos+120, ypos+7);

  DisplayDatetime();
}

void SetTime()
{
  time_t t1;
  time_t utc;
  tmElements_t tm;

  tm.Year = CalendarYrToTm(tmpYear);

  tm.Month = tmpMonth;
  tm.Day = tmpDay;
  tm.Hour = tmpHour;
  tm.Minute = tmpMinute;
  tm.Second = 0;
  t1 = makeTime(tm);

  utc = tzc.toUTC(t1);


  //use the time_t value to ensure correct weekday is set
  if(RTC.set(utc) == 0) { // Success
    setTime(utc);
  }

  time_t t = timelocal(now());

  tmpHour = hour(t);
  tmpMinute = minute(t);

  tmpMonth = month(t);
  tmpDay = day(t);
  tmpYear = year(t);
}

void ScreenDatetimeeButtonHandler()
{
  if (myTouch.dataAvailable())
  {
    myTouch.read();
    int x=myTouch.getX();
    int y=myTouch.getY();


    int xpos = 20;
    int ypos = 15;
    int ysize = 30;
    int xsize = 30;

    ypos += ysize + 5;

    // hour -
    if ((x > (xpos)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);      
      WaitTouch();      
      tmpHour = (--tmpHour == 0xFF)?23:tmpHour;
      DisplayDatetime();      
    }

    // hour +
    if ((x > (xpos+xsize+10)) && (x < (xpos+xsize*2+10)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize+10, ypos, xpos+xsize*2+10, ypos + ysize);      
      WaitTouch();      
      tmpHour = (++tmpHour > 23)?0:tmpHour;
      DisplayDatetime();      
    }

    // minute -
    if ((x > (xpos+xsize*2+40)) && (x < (xpos+xsize*3+40)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize*2+40, ypos, xpos+xsize*3+40, ypos + ysize);      
      WaitTouch();      
      tmpMinute = (--tmpMinute == 0xFF)?59:tmpMinute;
      DisplayDatetime();      
    }

    // minute +
    if ((x > (xpos+xsize*3+50)) && (x < (xpos+xsize*4+50)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize*3+50, ypos, xpos+xsize*4+50, ypos + ysize);      
      WaitTouch();      
      tmpMinute = (++tmpMinute > 59)?0:tmpMinute;
      DisplayDatetime();      
    }

    ypos += ysize + 5;
    ypos += ysize + 5;


    // day -
    if ((x > (xpos)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);      
      WaitTouch();      
      tmpDay = (--tmpDay < 1)?31:tmpDay;
      DisplayDatetime();      
    }

    // day +
    if ((x > (xpos+xsize+10)) && (x < (xpos+xsize*2+10)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize+10, ypos, xpos+xsize*2+10, ypos + ysize);      
      WaitTouch();      
      tmpDay = (++tmpDay > 31)?1:tmpDay;
      DisplayDatetime();      
    }


    // month -
    if ((x > (xpos+xsize*2+40)) && (x < (xpos+xsize*3+40)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize*2+40, ypos, xpos+xsize*3+40, ypos + ysize);      
      WaitTouch();      
      tmpMonth = (--tmpMonth < 1)?12:tmpMonth;
      DisplayDatetime();      
    }

    // month +
    if ((x > (xpos+xsize*3+50)) && (x < (xpos+xsize*4+50)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize*3+50, ypos, xpos+xsize*4+50, ypos + ysize);      
      WaitTouch();      
      tmpMonth = (++tmpMonth > 12)?1:tmpMonth;
      DisplayDatetime();      
    }

    // year -
    if ((x > (xpos+xsize*4+80)) && (x < (xpos+xsize*5+80)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize*4+80, ypos, xpos+xsize*5+80, ypos + ysize);      
      WaitTouch();      
      tmpYear = (--tmpYear < 2000)?2014:tmpYear;
      DisplayDatetime();      
    }

    // year +
    if ((x > (xpos+xsize*5+90)) && (x < (xpos+xsize*6+90)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos+xsize*5+90, ypos, xpos+xsize*6+90, ypos + ysize);      
      WaitTouch(); 
      ++tmpYear;     
      //tmpYear = (++tmpYear > 12)?1:tmpYear;
      DisplayDatetime();      
    }

    ypos = 190;
    xsize = 280;

    if ((x > (xpos)) && (x < (xpos+xsize)) && (y > ypos) && (y < (ypos + ysize)))
    {
      DrawButtonPressed(xpos, ypos, xpos+xsize, ypos + ysize);      
      WaitTouch();
      DrawScreenSettings();
    }
  }
}

void DisplayDatetime()
{
  SetTime();

  myGLCD.setBackColor(0, 0, 0);
  myGLCD.setColor(0, 255, 0); 

  strcpy_P(Dbuf1,PSTR("%02d:%02d"));
  sprintf(Dbuf, Dbuf1, tmpHour, tmpMinute); 
  myGLCD.print(Dbuf, 110, 20);

  strcpy_P(Dbuf1,PSTR("%02d.%02d.%04d"));
  sprintf(Dbuf, Dbuf1, tmpDay, tmpMonth, tmpYear); 
  myGLCD.print(Dbuf, 110, 90);
}

/////////////////////////////////////
// end datetime screen
/////////////////////////////////////

