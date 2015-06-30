/*
 V1.0 - first version
 
 Created by Igor Jarc <admin@iot-playground.com>
 See http://iot-playground.com for details
 Do not contact avtor dirrectly, use community forum on website.
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#ifndef Irrigation_h
#define Irrigation_h


#define MAX_IRRIGATORS 5


#define MS_IN_SEC  1000 // 1S 


#define HUM_REPORT_DELAY  60 // S 

#define CHILD_ID_SWITCH_IRRIGATE        0
#define CHILD_ID_AUTO_MODE              1
#define CHILD_ID_SOIL_HUMIDITY          2
#define CHILD_ID_SOIL_HUMIDITY_AO       3


#define MAX_ANALOG_VAL         1023
#define MIN_ANALOG_VAL         0
#define IRRIGATION_TIME        15 // irrigation time in sec
#define IRRIGATION_PAUSE_TIME  3600 // irrigation pause time in sec - only for auto irrigator




// irrigator state
typedef enum {
  s_idle             = 0,  // irrigation idle
  s_irrigation_start = 1,  // start irrigation
  s_irrigation       = 2,  // irrigate
  s_irrigation_stop  = 3,  // irrigation stop
} e_state;

#ifdef __cplusplus
class Irrigation
{
  public:
	Irrigation(void);
	void begin(int (* aiReadCallback)(int), void (* humiditySetCallback)(int, int), void (* motorSetCallback)(int, bool));
    void process();
	void setAutoMode(int i, bool enable);
    void setIrrigate(int i, bool enable);
	void setSoilHumidityTreshold(int i, int treshold);
	int getIrrigatorsCount();

  private:
    void processsingle(int i);
	bool IsTimeout(int i);
	int mapAiToPercent(int in);
	int (*_aiReadCallback)(int); 
	void (* _motorSetCallback)(int, bool);
	void (* _humiditySetCallback)(int, int);
	

    e_state state[MAX_IRRIGATORS];      // state
    bool pumpMotor[MAX_IRRIGATORS];         // motor state
    unsigned long startTime[MAX_IRRIGATORS];   //
    int soilHum[MAX_IRRIGATORS]; // 
	bool autoMode[MAX_IRRIGATORS];
	int irrigatorCounter[MAX_IRRIGATORS];
	int soilHumidityTreshold[MAX_IRRIGATORS];
	int lastAnalogReading[MAX_IRRIGATORS];
	int humReportCnt[MAX_IRRIGATORS];

};

#endif
#endif