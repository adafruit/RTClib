// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#ifndef __RTCLIB_H__
#define __RTCLIB_H__

// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime
{
public:
    DateTime (uint32_t t =0);
    DateTime (uint16_t year, uint8_t month, uint8_t day,
              uint8_t hour =0, uint8_t min =0, uint8_t sec =0);
    DateTime (const char* date, const char* time);
    uint16_t year() const
    {
        return 2000 + yOff;
    }
    uint8_t month() const
    {
        return m;
    }
    uint8_t day() const
    {
        return d;
    }
    uint8_t hour() const
    {
        return hh;
    }
    uint8_t minute() const
    {
        return mm;
    }
    uint8_t second() const
    {
        return ss;
    }
    uint8_t dayOfWeek() const;

    // 32-bit times as seconds since 1/1/2000
    long secondstime() const;
    // 32-bit times as seconds since 1/1/1970
    uint32_t unixtime(void) const;
    // as a string
    char* toString(char* buf, int maxlen) const;
    // add additional time
    void operator+=(uint32_t);

protected:
    uint8_t yOff, m, d, hh, mm, ss;
};


extern uint8_t bcd2bin (uint8_t val);
extern uint8_t bin2bcd (uint8_t val);

// RTC using the internal millis() clock, has to be initialized before use
// NOTE: this clock won't be correct once the millis() timer rolls over (>49d?)
class RTC_Millis
{
public:
    void begin(const DateTime& dt)
    {
        adjust(dt);
    }
    void adjust(const DateTime& dt);
    DateTime now();
    RTC_Millis(void)
    {
	adjust(DateTime(2000,1,1,0,0,0));
    }

protected:
    long offset;
};

#endif // __RTCLIB_H__

// vim:ci:sw=4 sts=4 ft=cpp
