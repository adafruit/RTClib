// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#ifndef __RTC_DS1307_H__
#define __RTC_DS1307_H__

#include <RTClib.h>

// RTC based on the DS1307 chip connected via I2C and the Wire library
class RTC_DS1307
{
public:
    static uint8_t begin(void);
    static void adjust(const DateTime& dt);
    uint8_t isrunning(void);
    static DateTime now();
};

#endif // __RTC_DS1307_H__

// vim:ci:sw=4 sts=4 ft=cpp
