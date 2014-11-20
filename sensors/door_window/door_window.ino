#include <avr/sleep.h>    // Sleep Modes
#include <MySensor.h>
#include <SPI.h>


#define CHILD_ID 3
#define BUTTON_PIN 3 // Arduino Digital I/O pin for button/reed switch
MySensor gw;

#define INTERRUPT_SENSOR  1 // Interrupt 1 is on DIGITAL PIN 3!
#define BATTERY_SENSE_PIN  
#define SLEEP_IN_MS 86400000 // 1 day

byte val;
int oldBatLevel;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(CHILD_ID,V_TRIPPED);

void setup()
{
  gw.begin();

  pinMode(BUTTON_PIN,INPUT);
  digitalWrite (BUTTON_PIN, LOW);   

  pinMode(2,INPUT);
  digitalWrite (2, LOW); 

  pinMode(4,INPUT);
  digitalWrite (4, LOW); 

  pinMode(5,INPUT);
  digitalWrite (5, LOW); 

  pinMode(6,INPUT);
  digitalWrite (6, LOW); 

  pinMode(7,INPUT);
  digitalWrite (7, LOW); 

  pinMode(8,INPUT);
  digitalWrite (8, LOW); 

  gw.present(CHILD_ID, S_DOOR);

  val = digitalRead(BUTTON_PIN);
  oldBatLevel = -1;  
  
  sendValue();
}

void loop ()
{   
  gw.sleep(INTERRUPT_SENSOR, CHANGE, SLEEP_IN_MS); 
  val = digitalRead(BUTTON_PIN);

  sendValue(); 

  gw.sleep(200); 
}  // end of loop

void sendValue()
{
  gw.powerUp();  
  gw.send(msg.set(val==HIGH ? 1 : 0));

  gw.powerDown();  

  int batLevel = getBatteryLevel();
  if (oldBatLevel != batLevel)
  {
    gw.powerUp();  
    gw.sendBatteryLevel(batLevel);    
    oldBatLevel = batLevel;
  }
}

// Battery measure
int getBatteryLevel () 
{
  int results = (readVcc() - 2000)  / 10;   

  if (results > 100)
    results = 100;
  if (results < 0)
    results = 0;
  return results;
} // end of getBandgap

// when ADC completed, take an interrupt 
EMPTY_INTERRUPT (ADC_vect);

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  noInterrupts ();
  // start the conversion
  ADCSRA |= _BV (ADSC) | _BV (ADIE);
  set_sleep_mode (SLEEP_MODE_ADC);    // sleep during sample
  interrupts ();
  sleep_mode (); 
  // reading should be done, but better make sure
  // maybe the timer interrupt fired 
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV

  return result;
}
