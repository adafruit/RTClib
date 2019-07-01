// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#ifndef _RTCLIB_H_
#define _RTCLIB_H_

#include <Arduino.h>
class TimeSpan;


#define PCF8523_ADDRESS       0x68
#define PCF8523_CLKOUTCONTROL 0x0F
#define PCF8523_CONTROL_3     0x02
#define PCF8523_OFFSET        0x0E

#define DS1307_ADDRESS  0x68
#define DS1307_CONTROL  0x07
#define DS1307_NVRAM    0x08

#define DS3231_ADDRESS  0x68
#define DS3231_CONTROL  0x0E
#define DS3231_STATUSREG 0x0F
#define DS3231_TEMPERATUREREG	0x11

#define SECONDS_PER_DAY 86400L

#define SECONDS_FROM_1970_TO_2000 946684800



// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {
public:
    DateTime (uint32_t t = SECONDS_FROM_1970_TO_2000);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
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
    
    //ISO 8601 Timestamp function
    enum timestampOpt{
        TIMESTAMP_FULL, TIMESTAMP_TIME, TIMESTAMP_DATE
    };
    String timestamp(timestampOpt opt = TIMESTAMP_FULL);

    DateTime operator+(const TimeSpan& span);
    DateTime operator-(const TimeSpan& span);
    TimeSpan operator-(const DateTime& right);
    bool operator<(const DateTime& right) const;
    bool operator>(const DateTime& right) const  { return right < *this; }
    bool operator<=(const DateTime& right) const { return !(*this > right); }
    bool operator>=(const DateTime& right) const { return !(*this < right); }
    bool operator==(const DateTime& right) const;
    bool operator!=(const DateTime& right) const { return !(*this == right); }

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
	static float getTemperature();		// in Celcius degree
};


// RTC based on the PCF8523 chip connected via I2C and the Wire library
enum Pcf8523SqwPinMode { PCF8523_OFF = 7, PCF8523_SquareWave1HZ = 6, PCF8523_SquareWave32HZ = 5, PCF8523_SquareWave1kHz = 4, PCF8523_SquareWave4kHz = 3, PCF8523_SquareWave8kHz = 2, PCF8523_SquareWave16kHz = 1, PCF8523_SquareWave32kHz = 0 };

enum Pcf8523OffsetMode { PCF8523_TwoHours = 0x00, PCF8523_OneMinute = 0x80 };

class RTC_PCF8523 {
public:
    boolean begin(void);
    void adjust(const DateTime& dt);
    boolean initialized(void);
    static DateTime now();

    Pcf8523SqwPinMode readSqwPinMode();
    void writeSqwPinMode(Pcf8523SqwPinMode mode);
    void calibrate(Pcf8523OffsetMode mode, int8_t offset);
};

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this is immune to millis() rollover events
class RTC_Millis {
public:
    static void begin(const DateTime& dt) { adjust(dt); }
    static void adjust(const DateTime& dt);
    static DateTime now();

protected:
    static uint32_t lastUnix;
    static uint32_t lastMillis;
};

// RTC using the internal micros() clock, has to be initialized before
// use. Unlike RTC_Millis, this can be tuned in order to compensate for
// the natural drift of the system clock. Note that now() has to be
// called more frequently than the micros() rollover period, which is
// approximately 71.6 minutes.
class RTC_Micros {
public:
    static void begin(const DateTime& dt) { adjust(dt); }
    static void adjust(const DateTime& dt);
    static void adjustDrift(int ppm);
    static DateTime now();

protected:
    static uint32_t microsPerSecond;
    static uint32_t lastUnix;
    static uint32_t lastMicros;
};

#endif // _RTCLIB_H_
