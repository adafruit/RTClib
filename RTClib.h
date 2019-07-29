/**************************************************************************/
/*!
  @file     RTClib.h

  Original library by JeeLabs http://news.jeelabs.org/code/, released to the public domain

  License: MIT (see LICENSE)

  This is a fork of JeeLab's fantastic real time clock library for Arduino.

  For details on using this library with an RTC module like the DS1307, PCF8523, or DS3231,
  see the guide at: https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/overview

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
*/
/**************************************************************************/

#ifndef _RTCLIB_H_
#define _RTCLIB_H_

#include <Arduino.h>
class TimeSpan;

/** Registers */
#define PCF8523_ADDRESS       0x68  ///< I2C address for PCF8523
#define PCF8523_CLKOUTCONTROL 0x0F  ///< Timer and CLKOUT control register
#define PCF8523_CONTROL_3     0x02  ///< Control and status register 3
#define PCF8523_OFFSET        0x0E  ///< Offset register

#define DS1307_ADDRESS        0x68  ///< I2C address for DS1307
#define DS1307_CONTROL        0x07  ///< Control register
#define DS1307_NVRAM          0x08  ///< Start of RAM registers - 56 bytes, 0x08 to 0x3f

#define DS3231_ADDRESS        0x68  ///< I2C address for DS3231
#define DS3231_CONTROL        0x0E  ///< Control register
#define DS3231_STATUSREG      0x0F  ///< Status register
#define DS3231_TEMPERATUREREG	0x11  ///< Temperature register (high byte - low byte is at 0x12), 10-bit temperature value

/** Constants */
#define SECONDS_PER_DAY       86400L  ///< 60 * 60 * 24
#define SECONDS_FROM_1970_TO_2000 946684800  ///< Unixtime for 2000-01-01 00:00:00, useful for initialization


/**************************************************************************/
/*!
    @brief  Simple general-purpose date/time class (no TZ / DST / leap second handling!).
            See http://en.wikipedia.org/wiki/Leap_second
*/
/**************************************************************************/
class DateTime {
public:
  DateTime (uint32_t t = SECONDS_FROM_1970_TO_2000);
  DateTime (uint16_t year, uint8_t month, uint8_t day,
              uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
  DateTime (const DateTime& copy);
  DateTime (const char* date, const char* time);
  DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time);
  char* toString(char* buffer);

  /*!
      @brief  Return the year, stored as an offset from 2000
      @return uint16_t year
  */
  uint16_t year() const       { return 2000 + yOff; }
  /*!
      @brief  Return month
      @return uint8_t month
  */
  uint8_t month() const       { return m; }
  /*!
      @brief  Return day
      @return uint8_t day
  */
  uint8_t day() const         { return d; }
  /*!
      @brief  Return hours
      @return uint8_t hours
  */
  uint8_t hour() const        { return hh; }
  /*!
      @brief  Return minutes
      @return uint8_t minutes
  */
  uint8_t minute() const      { return mm; }
  /*!
      @brief  Return seconds
      @return uint8_t seconds
  */
  uint8_t second() const      { return ss; }

  uint8_t dayOfTheWeek() const;

  /** 32-bit times as seconds since 1/1/2000 */
  long secondstime() const;

  /** 32-bit times as seconds since 1/1/1970 */
  uint32_t unixtime(void) const;

  /** ISO 8601 Timestamp function */
  enum timestampOpt{
    TIMESTAMP_FULL, // YYYY-MM-DDTHH:MM:SS
    TIMESTAMP_TIME, // HH:MM:SS
    TIMESTAMP_DATE  // YYYY-MM-DD
  };
  String timestamp(timestampOpt opt = TIMESTAMP_FULL);

  DateTime operator+(const TimeSpan& span);
  DateTime operator-(const TimeSpan& span);
  TimeSpan operator-(const DateTime& right);
  bool operator<(const DateTime& right) const;
  /*!
      @brief  Test if one DateTime is greater (later) than another
      @param right DateTime object to compare
      @return True if the left object is greater than the right object, false otherwise
  */
  bool operator>(const DateTime& right) const  { return right < *this; }
  /*!
      @brief  Test if one DateTime is less (earlier) than or equal to another
      @param right DateTime object to compare
      @return True if the left object is less than or equal to the right object, false otherwise
  */
  bool operator<=(const DateTime& right) const { return !(*this > right); }
  /*!
      @brief  Test if one DateTime is greater (later) than or equal to another
      @param right DateTime object to compare
      @return True if the left object is greater than or equal to the right object, false otherwise
  */
  bool operator>=(const DateTime& right) const { return !(*this < right); }
  bool operator==(const DateTime& right) const;
  /*!
      @brief  Test if two DateTime objects not equal
      @param right DateTime object to compare
      @return True if the two objects are not equal, false if they are
  */
  bool operator!=(const DateTime& right) const { return !(*this == right); }

protected:
  uint8_t yOff;   ///< Year offset from 2000
  uint8_t m;      ///< Month 1-12
  uint8_t d;      ///< Day 1-31
  uint8_t hh;     ///< Hours 0-23
  uint8_t mm;     ///< Minutes 0-59
  uint8_t ss;     ///< Seconds 0-59
};


/**************************************************************************/
/*!
    @brief  Timespan which can represent changes in time with seconds accuracy.
*/
/**************************************************************************/
class TimeSpan {
public:
  TimeSpan (int32_t seconds = 0);
  TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
  TimeSpan (const TimeSpan& copy);

  /*!
      @brief  Number of days in the TimeSpan
              e.g. 4
      @return int16_t days
  */
  int16_t days() const         { return _seconds / 86400L; }
  /*!
      @brief  Number of hours in the TimeSpan
              This is not the total hours, it includes the days
              e.g. 4 days, 3 hours - NOT 99 hours
      @return int8_t hours
  */
  int8_t  hours() const        { return _seconds / 3600 % 24; }
  /*!
      @brief  Number of minutes in the TimeSpan
              This is not the total minutes, it includes days/hours
              e.g. 4 days, 3 hours, 27 minutes
      @return int8_t minutes
  */
  int8_t  minutes() const      { return _seconds / 60 % 60; }
  /*!
      @brief  Number of seconds in the TimeSpan
              This is not the total seconds, it includes the days/hours/minutes
              e.g. 4 days, 3 hours, 27 minutes, 7 seconds
      @return int8_t seconds
  */
  int8_t  seconds() const      { return _seconds % 60; }
  /*!
      @brief  Total number of seconds in the TimeSpan, e.g. 358027
      @return int32_t seconds
  */
  int32_t totalseconds() const { return _seconds; }

  TimeSpan operator+(const TimeSpan& right);
  TimeSpan operator-(const TimeSpan& right);

protected:
  int32_t _seconds;   ///< Actual TimeSpan value is stored as seconds
};



/** DS1307 SQW pin mode settings */
enum Ds1307SqwPinMode {
  DS1307_OFF              = 0x00, // Low
  DS1307_ON               = 0x80, // High
  DS1307_SquareWave1HZ    = 0x10, // 1Hz square wave
  DS1307_SquareWave4kHz   = 0x11, // 4kHz square wave
  DS1307_SquareWave8kHz   = 0x12, // 8kHz square wave
  DS1307_SquareWave32kHz  = 0x13  // 32kHz square wave
};

/**************************************************************************/
/*!
    @brief  RTC based on the DS1307 chip connected via I2C and the Wire library
*/
/**************************************************************************/
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



/** DS3231 SQW pin mode settings */
enum Ds3231SqwPinMode {
  DS3231_OFF            = 0x01, // Off
  DS3231_SquareWave1Hz  = 0x00, // 1Hz square wave
  DS3231_SquareWave1kHz = 0x08, // 1kHz square wave
  DS3231_SquareWave4kHz = 0x10, // 4kHz square wave
  DS3231_SquareWave8kHz = 0x18  // 8kHz square wave
};

/**************************************************************************/
/*!
    @brief  RTC based on the DS3231 chip connected via I2C and the Wire library
*/
/**************************************************************************/
class RTC_DS3231 {
public:
  boolean begin(void);
  static void adjust(const DateTime& dt);
  bool lostPower(void);
  static DateTime now();
  static Ds3231SqwPinMode readSqwPinMode();
  static void writeSqwPinMode(Ds3231SqwPinMode mode);
  static float getTemperature();  // in Celcius degree
};



/** PCF8523 SQW pin mode settings */
enum Pcf8523SqwPinMode {
  PCF8523_OFF             = 7, // Off
  PCF8523_SquareWave1HZ   = 6, // 1Hz square wave
  PCF8523_SquareWave32HZ  = 5, // 32Hz square wave
  PCF8523_SquareWave1kHz  = 4, // 1kHz square wave
  PCF8523_SquareWave4kHz  = 3, // 4kHz square wave
  PCF8523_SquareWave8kHz  = 2, // 8kHz square wave
  PCF8523_SquareWave16kHz = 1, // 16kHz square wave
  PCF8523_SquareWave32kHz = 0  // 32kHz square wave
};

/** PCF8523 Offset modes for making temperature/aging/accuracy adjustments */
enum Pcf8523OffsetMode {
  PCF8523_TwoHours = 0x00,  // Offset made every two hours
  PCF8523_OneMinute = 0x80  // Offset made every minute
};

/**************************************************************************/
/*!
    @brief  RTC based on the PCF8523 chip connected via I2C and the Wire library
*/
/**************************************************************************/
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


/**************************************************************************/
/*!
    @brief  RTC using the internal millis() clock, has to be initialized before use.
            NOTE: this is immune to millis() rollover events.
*/
/**************************************************************************/
class RTC_Millis {
public:
  /*!
      @brief  Start the RTC
      @param dt DateTime object with the date/time to set
  */
  static void begin(const DateTime& dt) { adjust(dt); }
  static void adjust(const DateTime& dt);
  static DateTime now();

protected:
  static uint32_t lastUnix;   ///< Unix time from the previous call to now() - prevents rollover issues
  static uint32_t lastMillis; ///< the millis() value corresponding to the last **full second** of Unix time
};



/**************************************************************************/
/*!
    @brief  RTC using the internal micros() clock, has to be initialized before
            use. Unlike RTC_Millis, this can be tuned in order to compensate for
            the natural drift of the system clock. Note that now() has to be
            called more frequently than the micros() rollover period, which is
            approximately 71.6 minutes.
*/
/**************************************************************************/
class RTC_Micros {
public:
  /*!
      @brief  Start the RTC
      @param dt DateTime object with the date/time to set
  */
  static void begin(const DateTime& dt) { adjust(dt); }
  static void adjust(const DateTime& dt);
  static void adjustDrift(int ppm);
  static DateTime now();

protected:
  static uint32_t microsPerSecond;  ///< Number of microseconds reported by micros() per "true" (calibrated) second
  static uint32_t lastUnix;         ///< Unix time from the previous call to now() - prevents rollover issues
  static uint32_t lastMicros;       ///< micros() value corresponding to the last full second of Unix time
};

#endif // _RTCLIB_H_
