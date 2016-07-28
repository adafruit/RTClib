#ifndef DateTime_hpp
#define DateTime_hpp

#include "Arduino.h"
class TimeSpan;

#define SECONDS_FROM_1970_TO_2000 946684800

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

class DateTime {
public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
                uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const DateTime& copy);
    DateTime (const char* date, const char* time);
    DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time);
    uint16_t year() const       { return 2000 + yOff; };
    uint8_t month() const       { return m; };
    uint8_t day() const         { return d; };
    uint8_t hour() const        { return hh; };
    uint8_t minute() const      { return mm; };
    uint8_t second() const      { return ss; };
    uint8_t dayOfTheWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;

    enum timestampOpt {
        TIMESTAMP_FULL, TIMESTAMP_TIME, TIMESTAMP_DATE
    };

    String timestamp(timestampOpt opt = TIMESTAMP_FULL) const;

    DateTime operator+(const TimeSpan& span);
    DateTime operator-(const TimeSpan& span);
    TimeSpan operator-(const DateTime& right);

    bool operator<(const DateTime& right) const;
    bool operator>(const DateTime& right) const  { return right < *this; };
    bool operator<=(const DateTime& right) const { return !(*this > right); };
    bool operator>=(const DateTime& right) const { return !(*this < right); };
    bool operator==(const DateTime& right) const;
    bool operator!=(const DateTime& right) const { return !(*this == right); };

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

#endif
