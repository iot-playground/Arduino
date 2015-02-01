ReadMe file for Arduino DS1302RTC Library

Timur Maksimov 2014
Based on ReadMe by Jack Christensen Mar 2013
Based on page by arduino.cc user "Krodal" March 2013

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0
Unported License. To view a copy of this license, visit
http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative
Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.

================================================================================
Documentation: datasheet (http://datasheets.maximintegrated.com/en/ds/DS1302.pdf)

Arduino library to support the Maxim Integrated DS1302
Real-Time Clocks. This library is intended for use with the Arduino Time.h
library, http://www.arduino.cc/playground/Code/Time.

The DS1302RTC library is a drop-in replacement for the DS1307RTC.h library by
Michael Margolis that is supplied with the Arduino Time library above. To change
from using a DS1307 RTC to a DS1302 RTC, it is only necessary to change the
#include statement to include DS1302RTC.h instead of DS1307RTC.h.

This library also implements functions to support the additional
features of the DS1302:
1. Real Time Clock read/write (8 bytes)
2. Battery backed RAM read/write (31 bytes)
3. Power save mode manipulation (start/stop clock)
4. Trickle charger setup
5. Burst mode read/write
6. 24 hour format only (12 hour format is function Time library)

Whether used with the DS1302, the user is responsible for ensuring reads
and writes do not exceed the device's address space
(0x80-0x90 for DS1302 clock data and 0xC0-0xFC RAM data);
no bounds checking is done by this library.

The DS1302 uses a 3-wire interface:
   - bidirectional data.
   - clock
   - chip select
It is not I2C, not OneWire, and not SPI. So the standard libraries can not be used.

The "Chip Enable" pin was called "/Reset" before.

The chip has internal pull-down registers. This keeps the chip disabled, even
if the pins of the Arduino are floating.

In burst mode, all the clock data is read at once. This is to prevent a rollover
of a digit during reading. The read data is from an internal buffer.

The DS1302 has 31 of ram, which can be used to store data. The contents will be
lost if the Arduino is off, and the backup battery gets empty.
It is better to store data in the EEPROM of the Arduino.

The DS1302 has a build-in trickle charger. That can be used for example with a
rechargeble battery or a supercap.

--------------------------------------------------------------------------------
Unlike the DS1307RTC library, the DS1302RTC library instantiates an RTC
object by user. This is required to define control pins.

To use the DS1302RTC library, the Time library must also be included.
For brevity, these includes are not repeated in the examples below:
#include <DS1302RTC.h>
#include <Time.h>                 // http://www.arduino.cc/playground/Code/Time

// Set pins:   CE, IO, CLK
DS1302RTC RTC( 27, 29,  31 );

================================================================================
SETTING AND READING THE TIME

Examples:
1. SetSerial     - Set date and time via serial interface
2. DS1302_Serial - Print time and date to serial interface
3. DS1302_LCD    - Simple clock with 16x2 LCD

--------------------------------------------------------------------------------
The get() method reads the current time from the RTC and returns it as a time_t
value (also known as UNIX time).

    time_t myTime;
    myTime = RTC.get();

Function return zero if any errors occuried (eg. RTC power down).

--------------------------------------------------------------------------------
The set(time_t t) method sets the RTC to the given time_t value.
The example below first sets the system time (maintained by the Time library) to
a hard-coded date and time, then sets the RTC from the system time. The
setTime(hr, min, sec, day, month, year) function comes from the Time library.

    setTime(23, 31, 30, 13, 2, 2009);   //set the system time to
                                        //23h31m30s on 3Feb2009
    RTC.set(now());                     //set the RTC from the system time

Function return zero if operation successful.

--------------------------------------------------------------------------------
The read(tmElements_t &tm) method reads the current time from the RTC and
returns it as a tmElements_t structure.
(See the Arduino Time library for details on the tmElements_t structure:
http://www.arduino.cc/playground/Code/Time.)

    tmElements_t tm;
    RTC.read(tm);
    Serial.print(tm.Hour, DEC);
    Serial.print(':');
    Serial.print(tm.Minute,DEC);
    Serial.print(':');
    Serial.println(tm.Second,DEC);

Function return zero if operation successful.

--------------------------------------------------------------------------------
The write(tmElements_t &tm) method sets the RTC to the date and time
as represented in a tmElements_t structure.

    tmElements_t tm;
    tm.Hour = 23;             // set the tm structure to 23h31m30s on 13Feb2009
    tm.Minute = 31;
    tm.Minute = 30;
    tm.Day = 13;
    tm.Month = 2;
    tm.Year = 2009 - 1970;    // tmElements_t.Year is the offset from 1970.
or  tm.Year = CalendarYrToTm(2009); // macros Time library
    RTC.write(tm);            // set the RTC from the tm structure

Function return zero if operation successful.

--------------------------------------------------------------------------------
The haltRTC() method get or set RTC clock halt flag in DS1302 seconds register.
When set to logic 1, the clock oscillator is stopped and the DS1302 is placed
into a low-power standby mode with a current drain of less than 100nA.
When set to logic 0, the clock will start.

    RTC.haltRTC(true);
    if(RTC.haltRTC())
      Serial.println("Clock stopped.");

When clock setup (set() or write() methods) clock automaticaly started.

================================================================================
READING AND WRITING REGISTERS (OR RAM)

Examples:
1. DS1302_RAM_Demo - access to RAM

--------------------------------------------------------------------------------
The writeEN() method set or clear write protect feature.
Before any write operation to the clock or RAM, must be 0. When 1, the
write-protect prevents a write operation to any other register.

    if( ! RTC.writeEN() ) {
      Serial.println("Write protected! Allow...");
      RTC.writeEN(true);
    }

When clock setup (set() or write() methods) not need clear write-protect flag,
after finish operation write-protect auto enabled.

--------------------------------------------------------------------------------
The writeRAM(uint8_t *p) method write 31 bytes to the DS1302 internal memory in
burst mode.

    uint8_t buff[31];

    buff[0] = 123;
    ...
    buff[30] = ...;

    RTC.writeEN(true);  // Disable write-protect
    RTC.writeRAM(buff);
    RTC.writeEN(false); // Enable write-protect

--------------------------------------------------------------------------------
The readRAM(uint8_t *p) method reading 31 bytes from the DS1302 internal memory
in burst mode.

    uint8_t buff[31];

    RTC.readRAM(buff);

    for(byte i = 0; i < 31; i++)
      Serial.println(buff[i], HEX);

--------------------------------------------------------------------------------
The writeRTC(uint8_t address, uint8_t value) method write 1 byte to any address
in the DS1302 (incude commands space).

     // Write 0xA0 value to RAM, offset 0x0F
     RTC.writeRTC(DS1302_RAM_START+(0x0F << 1),0xA0);

Address space have 5 bit from bit 1 to bit 5, bit 0 read/write flag (0 = read,
1 = write: set by function), bit 6 RAM or CLOCK access, bit 7 always =1.
ADDR: b7 b6  b5 b4 b3 b2 b1 b0
      --------------------------
       1 RAM A4 A3 A2 A1 A0 READ
         CLK                WR

--------------------------------------------------------------------------------
The readRTC(uint8_t address) method read and return 1 byte from any address
in the DS1302.

     // Read byte from RAM, offset 0x1E (last byte)
     Serial.print( RTC.readRTC(DS1302_RAM_START+(0x1E << 1)) , HEX);

--------------------------------------------------------------------------------
The writeRTC(uint8_t *p) method write 8 bytes clock data to the DS1302 internal
buffer in burst mode.

    uint8_t buff[8];

    buff[0] = B10000000; // Clock stopped, seconds is zeroed.
    ...
    buff[7] = B10000000; // Enable write protect after finish

    writeEN(true);
    writeRTC(buff);

This function not needed for normal operations, but may used eg. while
chip clear or reset.

--------------------------------------------------------------------------------
The readRTC(uint8_t *p) method read 8 bytes clock data from the DS1302 internal
buffer in burst mode.

    uint8_t buff[8];

    readRTC(buff);

This function not needed in normal operations.

================================================================================
TRICKLE CHARGER

WARNING: DO NOT ENABLE CHARGER UNLESS USE RECHARGEBLE BACKED POWER SOURCE.
(Lithium battery may explode on charge!)

Tricke charger configuration register not accessable in burst mode.

    // Set 2 diodes and 8k resistor as current limiter
    RTC.writeRTC(DS1302_TRICKLE, DS1302_TCR_2D8K);

    // Disable charger
    RTC.writeRTC(DS1302_TRICKLE, DS1302_TCR_INI);

    // Check
    if( RTC.readRTC(DS1302_TRICKLE) == DS1302_TCR_INI )
      Serial.println("Charger have power-on initial state! (DISABLED)");
    else
      Serial.println("Warning! Charger may be enabled!");
      
Only six register values indicated enable charger, other values indicated
disable charger.

For other defined constants please read DS1302RTC.h and do not fogot read
datasheet for understanding functional schematic.

--------------------------------------------------------------------------------
