/*
 * DS1302RTC.h - library for DS1302 RTC
  
  2014 Timur Maksimov
  Based on DS1307RTC library Michael Margolis 2009
  Based on DS1302 code by arduino.cc user "Krodal" 2013

  This library is intended to be uses with Arduino Time.h library functions

  The library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  18 Jul 2014 - Initial release
 */

#include "DS1302RTC.h"


// Private globals
uint8_t DS1302RTC::DS1302_CE_PIN;
uint8_t DS1302RTC::DS1302_IO_PIN;
uint8_t DS1302RTC::DS1302_SCLK_PIN;


// --------------------------------------------------------
// Constructor.
DS1302RTC::DS1302RTC(uint8_t CE_pin, uint8_t IO_pin, uint8_t SCLK_pin)
{
  DS1302_CE_PIN = CE_pin;
  DS1302_IO_PIN = IO_pin;
  DS1302_SCLK_PIN = SCLK_pin;
}
  
// PUBLIC FUNCTIONS

// --------------------------------------------------------
// Read the current time from the RTC and return it as a 
// time_t value. Returns a zero value if an bus error occurred
// (e.g. RTC not present).
time_t DS1302RTC::get()
{
    tmElements_t tm;
    
    if ( read(tm) ) return 0;
    return( makeTime(tm) );
}

// --------------------------------------------------------
// Set the RTC to the given time_t value.
// Returns the bus status (zero if successful).
uint8_t DS1302RTC::set(time_t t)
{
  tmElements_t tm;

  breakTime(t, tm);
  return ( write(tm) );
}

// --------------------------------------------------------
// Read the current time from the RTC and return it in a tmElements_t
// structure. Returns the bus status (zero if successful).
uint8_t DS1302RTC::read(tmElements_t &tm)
{
  uint8_t buff[8];

  readRTC(buff);

  tm.Second = bcd2dec(buff[0] & B01111111); // 7 bit (4L - sec, 3H - 10 sec), ignore CH bit
  tm.Minute = bcd2dec(buff[1] & B01111111); // 7 bit (4L - min, 3H - 10 min), ignore NULL bit
  tm.Hour   = bcd2dec(buff[2] & B00111111); // 6 bit (4L - hrs, 2H - 10 hrs), ignore NULL & 12/24 bits
  tm.Day    = bcd2dec(buff[3] & B00111111); // 6 bit (4L - dat, 2H - 10 dat), ignore 2 NULLs
  tm.Month  = bcd2dec(buff[4] & B00011111); // 5 bit (4L - mth, 1H - 10 mth), ignore 3 NULLs
  tm.Wday   =         buff[5] & B00000111 ; // 3 bit, ignore 5 NULLs
  tm.Year   = y2kYearToTm(
              bcd2dec(buff[6]));            // 8 bit
  
  // Validation
  if(tm.Second >= 0 && tm.Second <= 59)
    if(tm.Minute >= 0 && tm.Minute <= 59)
      if(tm.Hour >= 0 && tm.Hour <= 23)
        if(tm.Day >= 1 && tm.Day <= 31)
          if(tm.Month >= 1 && tm.Month <= 12)
            if(tm.Wday >= 1 && tm.Wday <= 7)
              if(tm.Year >= 0 && tm.Year <= 99)
                return 0;                   // Success

  return 1; // Error
}

// --------------------------------------------------------
// Set the RTC's time from a tmElements_t structure.
// Returns the bus status (zero if successful).
uint8_t DS1302RTC::write(tmElements_t &tm)
{
  uint8_t buff[8];

  writeEN(true);
  
  if(!writeEN()) return 255;                // Error! Write-protect not disabled

  buff[0] = dec2bcd(tm.Second);             // Disable Clock halt flag
  buff[1] = dec2bcd(tm.Minute);
  buff[2] = dec2bcd(tm.Hour);               // 24-hour mode
  buff[3] = dec2bcd(tm.Day);
  buff[4] = dec2bcd(tm.Month);
  buff[5] = tm.Wday;
  buff[6] = dec2bcd(tmYearToY2k(tm.Year));
  buff[7] = B10000000;                      // Write protect enable

  writeRTC(buff);

  return writeEN();
}

// --------------------------------------------------------
// Set or clear Clock halt flag bit
// When this bit is set to logic 1, the clock oscillator
// is stopped and the DS1302 is placed into a low-power 
// standby mode with a current drain of less than 100nA. When
// this bit is written to logic 0, the clock will start.
void DS1302RTC::haltRTC(uint8_t value)
{
  uint8_t seconds = readRTC(DS1302_SECONDS);
  bitWrite(seconds, DS1302_CH, value);
  writeRTC(DS1302_SECONDS, seconds);
}

// --------------------------------------------------------
// Check Clock halt flag bit
uint8_t DS1302RTC::haltRTC()
{
  return bitRead(readRTC(DS1302_SECONDS), DS1302_CH);
}

// --------------------------------------------------------
// Set or clear Write-protect bit
// Before any write operation to the clock or RAM, must be 0.
// When 1, the write-protect bit prevents a write operation
// to any other register.
void DS1302RTC::writeEN(uint8_t value)
{
  uint8_t wp = readRTC(DS1302_ENABLE);
  bitWrite(wp, DS1302_WP, !value);
  writeRTC(DS1302_ENABLE, wp);
}

// --------------------------------------------------------
// Check Write-protect bit
uint8_t DS1302RTC::writeEN()
{
  return !bitRead(readRTC(DS1302_ENABLE), DS1302_WP);
}

// --------------------------------------------------------
// readRTC burst mode
//
// This function reads 8 bytes clock data in burst mode
// from the DS1302.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void DS1302RTC::readRTC( uint8_t *p)
{
  togglestart();

  // Instead of the address,
  // the CLOCK_BURST_READ command is issued
  // the I/O-line is released for the data
  togglewrite( DS1302_CLOCK_BURST_READ);

  for(uint8_t i = 0; i < 8; i++)
    *p++ = toggleread();
  
  togglestop();
}


// --------------------------------------------------------
// writeRTC burst mode
//
// This function writes 8 bytes clock data in burst mode
// to the DS1302.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void DS1302RTC::writeRTC(uint8_t *p)
{
  togglestart();

  // Instead of the address,
  // the CLOCK_BURST_WRITE command is issued.
  // the I/O-line is not released
  togglewrite(DS1302_CLOCK_BURST_WRITE);

  for(uint8_t i = 0; i < 8; i++)
    togglewrite(*p++);  
  
  togglestop();
}

// --------------------------------------------------------
// readRAM burst mode
//
// This function reads 31 bytes RAM data in burst mode
// from the DS1302.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void DS1302RTC::readRAM(uint8_t *p)
{
  togglestart();

  // Instead of the address,
  // the RAM_BURST_READ command is issued
  // the I/O-line is released for the data
  togglewrite(DS1302_RAM_BURST_READ);

  for(uint8_t i = 0; i < 31; i++)
    *p++ = toggleread();

  togglestop();
}


// --------------------------------------------------------
// writeRAM burst mode
//
// This function writes 31 bytes RAM data in burst mode
// to the DS1302.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void DS1302RTC::writeRAM(uint8_t *p)
{
  togglestart();

  // Instead of the address,
  // the RAM_BURST_WRITE command is issued.
  // the I/O-line is not released
  togglewrite(DS1302_RAM_BURST_WRITE);

  for(uint8_t i = 0; i < 31; i++)
    togglewrite(*p++);  

  togglestop();
}

// --------------------------------------------------------
// readRTC
//
// This function reads a byte from the DS1302
// (clock or ram).
//
// The address could be like "0x80" or "0x81",
// the lowest bit is set anyway.
//
// This function may be called as the first function,
// also the pinMode is set.
//
uint8_t DS1302RTC::readRTC(uint8_t address)
{
  uint8_t value;

  // set lowest bit (read bit) in address
  bitSet( address, DS1302_READ);  

  togglestart();
  // the I/O-line is released for the value
  togglewrite( address);  
  value = toggleread();
  togglestop();

  return (value);
}


// --------------------------------------------------------
// writeRTC
//
// This function writes a byte to the DS1302 (clock or ram).
//
// The address could be like "0x80" or "0x81",
// the lowest bit is cleared anyway.
//
// This function may be called as the first function,
// also the pinMode is set.
//
void DS1302RTC::writeRTC(uint8_t address, uint8_t value)
{
  // clear lowest bit (read bit) in address
  bitClear(address, DS1302_READ);  

  togglestart();

  togglewrite(address);
  togglewrite(value);

  togglestop();  
}

// PRIVATE FUNCTIONS

// --------------------------------------------------------
// togglestart
//
// A helper function to setup the start condition.
//
// An 'init' function is not used.
// But now the pinMode is set every time.
// That's not a big deal, and it's valid.
// At startup, the pins of the Arduino are high impedance.
// Since the DS1302 has pull-down resistors,
// the signals are low (inactive) until the DS1302 is used.
void DS1302RTC::togglestart(void)
{
  digitalWrite( DS1302_CE_PIN, LOW); // default, not enabled
  pinMode( DS1302_CE_PIN, OUTPUT);  

  digitalWrite( DS1302_SCLK_PIN, LOW); // default, clock low
  pinMode( DS1302_SCLK_PIN, OUTPUT);

  pinMode( DS1302_IO_PIN, OUTPUT);

  digitalWrite( DS1302_CE_PIN, HIGH); // start the session
  delayMicroseconds( 4);           // tCC = 4us
}


// --------------------------------------------------------
// togglestop
//
// A helper function to finish the communication.
//
void DS1302RTC::togglestop(void)
{
  // Set CE low
  digitalWrite( DS1302_CE_PIN, LOW);

  delayMicroseconds( 4);           // tCWH = 4us
}


// --------------------------------------------------------
// toggleread
//
// A helper function for reading a byte with bit toggle
//
// This function assumes that the SCLK is still low.
//
uint8_t DS1302RTC::toggleread( void)
{
  uint8_t value = 0;

  // Set IO line for input
  pinMode( DS1302_IO_PIN, INPUT);

  for(uint8_t i = 0; i <= 7; i++)
  {
    // While insert function SCLK low and delay executed
    // data ready for reading

    // read bit, and set it in place in 'value' variable
    bitWrite(value, i, digitalRead( DS1302_IO_PIN));

    // This routine is called immediately after a togglewrite.
    // The first bit is present on IO_PIN at call, so only 7
    // additional clock pulses are required.
    // But in burst read all pules needed, and only last
    // byte should not have tail pulse.
    // So, while DS1302 ignore additional pulse,
    // we do nothing...

    // Clock up, prepare for next
    digitalWrite( DS1302_SCLK_PIN, HIGH);
    delayMicroseconds( 1);

    // Clock down, value is ready after some time.
    digitalWrite( DS1302_SCLK_PIN, LOW);
    delayMicroseconds( 1);        // tCL=1000ns, tCDD=800ns
  }
  return(value);
}


// --------------------------------------------------------
// togglewrite
//
// A helper function for writing a byte with bit toggle
//
void DS1302RTC::togglewrite(uint8_t value)
{
  for(uint8_t i = 0; i <= 7; i++)
  {
    // set a bit of the data on the I/O-line
    digitalWrite( DS1302_IO_PIN, bitRead(value, i));  
    delayMicroseconds( 1);     // tDC = 200ns

    // clock up, data is read by DS1302
    digitalWrite( DS1302_SCLK_PIN, HIGH);    
    delayMicroseconds( 1);     // tCH = 1000ns, tCDH = 800ns

    digitalWrite( DS1302_SCLK_PIN, LOW);
    delayMicroseconds( 1);     // tCL=1000ns, tCDD=800ns
  }
}

// Convert Decimal to Binary Coded Decimal (BCD)
uint8_t DS1302RTC::dec2bcd(uint8_t num)
{
  return ((num/10 * 16) + (num % 10));
}

// Convert Binary Coded Decimal (BCD) to Decimal
uint8_t DS1302RTC::bcd2dec(uint8_t num)
{
  return ((num/16 * 10) + (num % 16));
}

// vim: nowrap expandtab ts=2 sw=2
