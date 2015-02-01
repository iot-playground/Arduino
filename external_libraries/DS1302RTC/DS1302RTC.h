/*
 * DS1302RTC.h - library for DS1302 RTC
 * This library is intended to be uses with Arduino Time.h library functions
 */

#ifndef DS1302RTC_h
#define DS1302RTC_h

#include <Time.h>
#include <Arduino.h>


// Register names.
// Since the highest bit is always '1',
// the registers start at 0x80
// If the register is read, the lowest bit should be '1'.
#define DS1302_SECONDS           0x80
#define DS1302_MINUTES           0x82
#define DS1302_HOURS             0x84
#define DS1302_DATE              0x86
#define DS1302_MONTH             0x88
#define DS1302_DAY               0x8A
#define DS1302_YEAR              0x8C
#define DS1302_ENABLE            0x8E
#define DS1302_TRICKLE           0x90
#define DS1302_CLOCK_BURST       0xBE
#define DS1302_CLOCK_BURST_WRITE 0xBE
#define DS1302_CLOCK_BURST_READ  0xBF
#define DS1302_RAM_START         0xC0
#define DS1302_RAM_END           0xFC
#define DS1302_RAM_BURST         0xFE
#define DS1302_RAM_BURST_WRITE   0xFE
#define DS1302_RAM_BURST_READ    0xFF



// Defines for the bits, to be able to change
// between bit number and binary definition.
// By using the bit number, using the DS1302
// is like programming an AVR microcontroller.
// But instead of using "(1<<X)", or "_BV(X)",
// the Arduino "bit(X)" is used.
#define DS1302_D0 0
#define DS1302_D1 1
#define DS1302_D2 2
#define DS1302_D3 3
#define DS1302_D4 4
#define DS1302_D5 5
#define DS1302_D6 6
#define DS1302_D7 7


// Bit for reading (bit in address)
#define DS1302_READ     DS1302_D0 // READ=1, WRITE=0

// Bit for clock (0) or ram (1) area,
// called R/C-bit (bit in address)
#define DS1302_RC       DS1302_D6

// In Seconds Register
#define DS1302_CH       DS1302_D7 // 1 = Clock Halt, 0 = start

// In Hours Register
#define DS1302_AM_PM    DS1302_D5 // 0 = AM, 1 = PM
#define DS1302_12_24    DS1302_D7 // 0 = 24 hour, 1 = 12 hour

// Enable Write Protect Register
#define DS1302_WP       DS1302_D7 // 1 = Write Protect, 0 = enabled

// Trickle Register
#define DS1302_ROUT0    DS1302_D0 // Resistor 2kR (1 for 8kR)
#define DS1302_ROUT1    DS1302_D1 // Resistor 4kR (1 for 8kR)
#define DS1302_DS0      DS1302_D2 // 1 Diode
#define DS1302_DS1      DS1302_D3 // 2 Diodes
#define DS1302_TCS0     DS1302_D4 // Trickle-charge select (0 for enable)
#define DS1302_TCS1     DS1302_D5 // Trickle-charge select (1 for enable)
#define DS1302_TCS2     DS1302_D6 // Trickle-charge select (0 for enable)
#define DS1302_TCS3     DS1302_D7 // Trickle-charge select (1 for enable)

#define DS1302_TCR_INI  B01011100 // Initial power-on state (disabled)
#define DS1302_TCR_2D8K B10101011 // 2 Diodes, 8kΩ
#define DS1302_TCR_2D4K B10101010 // 2 Diodes, 4kΩ
#define DS1302_TCR_2D2K B10101001 // 2 Diodes, 2kΩ
#define DS1302_TCR_1D8K B10100111 // 1 Diodes, 8kΩ
#define DS1302_TCR_1D4K B10100110 // 1 Diodes, 4kΩ
#define DS1302_TCR_1D2K B10100101 // 1 Diodes, 2kΩ


// library interface description
class DS1302RTC
{
  // user-accessible "public" interface
  public:
    DS1302RTC(uint8_t CE_pin, uint8_t IO_pin, uint8_t SCLK_pin);
    static  time_t  get();
    static  uint8_t set(time_t t);
    static  uint8_t read(tmElements_t &tm);
    static  uint8_t write(tmElements_t &tm);

            uint8_t haltRTC();
            void    haltRTC( uint8_t value);

    static  uint8_t writeEN();
    static  void    writeEN( uint8_t value);
    
    static  uint8_t readRTC( uint8_t address);
    static  void    readRTC( uint8_t *p);
            void    readRAM( uint8_t *p);

    static  void    writeRTC(uint8_t address, uint8_t value);
    static  void    writeRTC(uint8_t *p);
            void    writeRAM(uint8_t *p);


  private:
    static  uint8_t DS1302_CE_PIN;
    static  uint8_t DS1302_IO_PIN;
    static  uint8_t DS1302_SCLK_PIN;

    static  void    togglestart(void);
    static  void    togglestop(void);
    static  uint8_t toggleread(void);
    static  void    togglewrite(uint8_t value);

    static  uint8_t dec2bcd(uint8_t num);
    static  uint8_t bcd2dec(uint8_t num);
};

#endif

// vim: nowrap expandtab ts=2 sw=2
