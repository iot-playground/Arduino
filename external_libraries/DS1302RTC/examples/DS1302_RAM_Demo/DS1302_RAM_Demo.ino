// DS1302_RAM_Demo (C)2010 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// Adopted for DS1302RTC library by Timur Maksimov 2014
//
// A quick demo of how to use my DS1302-library to 
// read and write to the internal RAM of the DS1302.
// Both burst (all 31 bytes at once) and single byte
// reads and write is demonstrated.
// All output is sent to the serial-port at 9600 baud.
//
// I assume you know how to connect the DS1302.
// DS1302:  CE pin    -> Arduino Digital 27
//          I/O pin   -> Arduino Digital 29
//          SCLK pin  -> Arduino Digital 31
//          VCC pin   -> Arduino Digital 33
//          GND pin   -> Arduino Digital 35

#include <Time.h>
#include <DS1302RTC.h>

// Set pins:  CE, IO,CLK
DS1302RTC RTC(27, 29, 31);

// Optional connection for RTC module
#define DS1302_GND_PIN 33
#define DS1302_VCC_PIN 35

// Global
uint8_t ramBuffer[31];

void setup()
{
  Serial.begin(9600);
  
  // Activate RTC module
  digitalWrite(DS1302_GND_PIN, LOW);
  pinMode(DS1302_GND_PIN, OUTPUT);

  digitalWrite(DS1302_VCC_PIN, HIGH);
  pinMode(DS1302_VCC_PIN, OUTPUT);
  
  Serial.println("RTC module activated");
  
  delay(500);
  
  if (RTC.haltRTC()) {
    Serial.println("The DS1302 is stopped.  Please run the SetTime");
    Serial.println("example to initialize the time and begin running.");
    Serial.println();
  }
  if (!RTC.writeEN()) {
    Serial.println("The DS1302 is write protected. This normal.");
    Serial.println();
  }

  delay(2000);
}

void loop()
{
  Serial.println();
  bufferDump("Initial buffer");

  comment("Filling buffer with data...");
  for (int i=0; i<31; i++)
    ramBuffer[i]=31-i;
    
  comment("Enable write access to RAM...");
  RTC.writeEN(true);
  
  if(RTC.writeEN())
    Serial.println("Write access granted!");
  else
    Serial.println("No write enabled. FATAL!");

  comment("Writing buffer to RAM...");
  RTC.writeRAM(ramBuffer);
  bufferDump("Buffer written to RAM...");
  
  comment("Clearing buffer...");
  for (int i=0; i<31; i++)
    ramBuffer[i]=0;
  bufferDump("Cleared buffer...");
  
  comment("Setting byte 15 (0x0F) to value 160 (0xA0)...");
  RTC.writeRTC(DS1302_RAM_START+(0x0F << 1),0xA0);
  
  comment("Reading buffer from RAM...");
  RTC.readRAM(ramBuffer);
  bufferDump("Buffer read from RAM...");

  int tmp = 0xFF;
  comment("Reading address:");
  
  Serial.print("Address: 0x12  ");
  tmp = RTC.readRTC(DS1302_RAM_START+(0x12 << 1));
  Serial.print("Return value: ");
  Serial.print(tmp, DEC);
  Serial.print(", 0x");
  Serial.println(tmp, HEX);

  Serial.print("Address: 0x01  ");
  tmp = RTC.readRTC(DS1302_RAM_START+(0x01 << 1));
  Serial.print("Return value: ");
  Serial.print(tmp, DEC);
  Serial.print(", 0x");
  Serial.println(tmp, HEX);

  Serial.print("Address: 0x1E  ");
  tmp = RTC.readRTC(DS1302_RAM_START+(0x1E << 1));
  Serial.print("Return value: ");
  Serial.print(tmp, DEC);
  Serial.print(", 0x");
  Serial.println(tmp, HEX);

  comment("Disable write access to RAM...");
  RTC.writeEN(false);

  Serial.println();
  Serial.println();
  Serial.println("***** End of demo *****");
 
  while (true);
}

void bufferDump(const char *msg)
{
  Serial.println(msg);
  for (int i=0; i<31; i++)
  {
    Serial.print("0x");
    if(ramBuffer[i] <= 0xF) Serial.print("0");
    Serial.print(ramBuffer[i], HEX);
    Serial.print(" ");
    if(!((i+1) % 8)) Serial.println();
  }
  Serial.println();
  Serial.println("--------------------------------------------------------");
}

void comment(const char *msg)
{
  Serial.println();
  Serial.print("---> ");
  Serial.write(msg);
  Serial.println();
  Serial.println();
}

