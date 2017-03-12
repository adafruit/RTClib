// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#ifndef _RTCLIB_H_
#define _RTCLIB_H_

#include <Arduino.h>
class TimeSpan;


#define PCF8523_ADDRESS       0x68
#define PCF8523_CLKOUTCONTROL 0x0F
#define PCF8523_CONTROL_3     0x02

#define DS1307_ADDRESS  0x68
#define DS1307_CONTROL  0x07
#define DS1307_NVRAM    0x08

#define DS3231_ADDRESS  0x68
#define DS3231_CONTROL  0x0E
#define DS3231_STATUSREG 0x0F

#define SECONDS_PER_DAY 86400L

#define SECONDS_FROM_1970_TO_2000 946684800

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const DateTime& copy);
    DateTime (const char* date, const char* time);
    DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time);
    uint16_t year() const       { return 2000 + yOff; }
    uint8_t month() const       { return m; }
    uint8_t day() const         { return d; }
    uint8_t hour() const        { return hh; }
    uint8_t minute() const      { return mm; }
    uint8_t second() const      { return ss; }
    uint8_t dayOfTheWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;   
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

    DateTime operator+(const TimeSpan& span);
    DateTime operator-(const TimeSpan& span);
    TimeSpan operator-(const DateTime& right);

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};

// Timespan which can represent changes in time with seconds accuracy.
class TimeSpan {
public:
    TimeSpan (int32_t seconds = 0);
    TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
    TimeSpan (const TimeSpan& copy);
    int16_t days() const         { return _seconds / 86400L; }
    int8_t  hours() const        { return _seconds / 3600 % 24; }
    int8_t  minutes() const      { return _seconds / 60 % 60; }
    int8_t  seconds() const      { return _seconds % 60; }
    int32_t totalseconds() const { return _seconds; }

    TimeSpan operator+(const TimeSpan& right);
    TimeSpan operator-(const TimeSpan& right);

protected:
    int32_t _seconds;
};

// RTC based on the DS1307 chip connected via I2C and the Wire library
enum Ds1307SqwPinMode { OFF = 0x00, ON = 0x80, SquareWave1HZ = 0x10, SquareWave4kHz = 0x11, SquareWave8kHz = 0x12, SquareWave32kHz = 0x13 };

class RTC_DS1307 {
public:
    boolean begin(void);
    static void adjust(const DateTime& dt);
    uint8_t isrunning(void);
    static DateTime now();
    static Ds1307SqwPinMode readSqwPinMode();
    static void writeSqwPinMode(Ds1307SqwPinMode mode);
    uint8_t readnvram(uint8_t address);
    void readnvram(uint8_t* buf, uint8_t size, uint8_t address);
    void writenvram(uint8_t address, uint8_t data);
    void writenvram(uint8_t address, uint8_t* buf, uint8_t size);
};

// RTC based on the DS3231 chip connected via I2C and the Wire library
enum Ds3231SqwPinMode { DS3231_OFF = 0x01, DS3231_SquareWave1Hz = 0x00, DS3231_SquareWave1kHz = 0x08, DS3231_SquareWave4kHz = 0x10, DS3231_SquareWave8kHz = 0x18 };

class RTC_DS3231 {
public:
    boolean begin(void);
    static void adjust(const DateTime& dt);
    bool lostPower(void);
    static DateTime now();
    static Ds3231SqwPinMode readSqwPinMode();
    static void writeSqwPinMode(Ds3231SqwPinMode mode);
};


// RTC based on the PCF8523 chip connected via I2C and the Wire library
enum Pcf8523SqwPinMode { PCF8523_OFF = 7, PCF8523_SquareWave1HZ = 6, PCF8523_SquareWave32HZ = 5, PCF8523_SquareWave1kHz = 4, PCF8523_SquareWave4kHz = 3, PCF8523_SquareWave8kHz = 2, PCF8523_SquareWave16kHz = 1, PCF8523_SquareWave32kHz = 0 };

class RTC_PCF8523 {
public:
    boolean begin(void);
    void adjust(const DateTime& dt);
    boolean initialized(void);
    static DateTime now();

    Pcf8523SqwPinMode readSqwPinMode();
    void writeSqwPinMode(Pcf8523SqwPinMode mode);
};

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (>49d?)
class RTC_Millis {
public:
    static void begin(const DateTime& dt) { adjust(dt); }
    static void adjust(const DateTime& dt);
    static DateTime now();

protected:
    static long offset;
};

////////////////////////////////////////////////////////////////////////////////

//// Support for the PCF85263 chip from NXP added by Robert Fraczkiewicz, 03/12/2017

#define PCF85263_SECONDS       0x01
#define PCF85263_ALARM1_SECONDS 0x08
#define PCF85263_ALARM2_MINUTES 0x0D
#define PCF85263_ALARM_ENABLES 0x10
#define PCF85263_OSCILLATOR    0x25
#define PCF85263_BATTSWITCH    0x26
#define PCF85263_PIN_IO        0x27
#define PCF85263_FUNCTION      0x28
#define PCF85263_INTA_ENABLES  0x29
#define PCF85263_FLAGS         0x2B
#define PCF85263_STOP          0x2E
#define PCF85263_RESET         0x2F

#define PCF85263_OS_BIT        7
#define PCF85263_EMON_BIT      7
#define PCF85263_STOP_BIT      0
#define PCF85263_CLKPM_BIT     7
#define PCF85263_INTA_OUT_BIT  1
#define PCF85263_ALARM1_SECONDS_BIT  0
#define PCF85263_ALARM1_MINUTES_BIT  1
#define PCF85263_ALARM1_HOURS_BIT    2
#define PCF85263_ALARM1_DAYS_BIT     3
#define PCF85263_ALARM1_MONTHS_BIT   4
#define PCF85263_ALARM2_MINUTES_BIT  5
#define PCF85263_ALARM2_HOURS_BIT    6
#define PCF85263_ALARM2_DAY_OTW_BIT  7
#define PCF85263_INTA_A1IEA_BIT      4
#define PCF85263_INTA_A2IEA_BIT      3
#define PCF85263_INTA_PIEA_BIT       6
#define PCF85263_INTA_PI_SECOND_BIT  5
#define PCF85263_INTA_PI_MINUTE_BIT  6
#define PCF85263_OSC_HIGH_CAPACITANCE_BIT 1
#define PCF85263_OSC_LOW_CAPACITANCE_BIT  0

#define PCF85263_PRESCALER_BITS 0xA4

// RTC based on the PCF85263 chip connected via I2C and the Wire library
enum Pcf85263SqwPinMode { PCF85263_OFF = 7, PCF85263_SquareWave1HZ = 6, PCF85263_SquareWave1kHZ = 5, PCF85263_SquareWave2kHz = 4, PCF85263_SquareWave4kHz = 3, PCF85263_SquareWave8kHz = 2, PCF85263_SquareWave16kHz = 1, PCF85263_SquareWave32kHz = 0 };

enum PCF85263PeriodicIntMode { PCF85263_PI_None = 0x0, PCF85263_PI_Second = 0x20, PCF85263_PI_Minute = 0x40,PCF85263_PI_Hour = 0x60 };

class RTC_PCF85263 {
private:
    const uint8_t my_I2C_address=0x51; // Default slave address from PCF85263 datasheet
public:
    bool begin(void);
    bool initialized(void);
    DateTime now();
    void adjust(const DateTime& dt);
    uint8_t myAddress() {return my_I2C_address;}
    
    Pcf85263SqwPinMode readSqwPinMode();
    void writeSqwPinMode(Pcf85263SqwPinMode mode);
    
    void defaultBatteryMode();
    void disableCLKOUTenableINTA();
    void setAlarm(uint8_t alarm,const DateTime& dt);
    void setAlarm(uint8_t alarm,const uint8_t hour, const uint8_t minute);
    void enableAlarm(uint8_t mode);
    void disableAlarm(uint8_t alarm);
    void setPeriodicInterrupt(PCF85263PeriodicIntMode mode);
    void enablePeriodicInterrupt();
    void disablePeriodicInterrupt();
    void setOscillatorCapacitance(uint8_t mode);
    void clearAllFlags();
};

#endif // _RTCLIB_H_
