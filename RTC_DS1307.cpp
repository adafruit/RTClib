// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <avr/pgmspace.h>
#include <WProgram.h>
#include <Wire.h>
#include "RTClib.h"
#include "RTC_DS1307.h"

#define DS1307_ADDRESS 0x68

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

uint8_t RTC_DS1307::begin(void)
{
    return 1;
}

uint8_t RTC_DS1307::isrunning(void)
{
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.send(0);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_ADDRESS, 1);
    uint8_t ss = Wire.receive();
    return !(ss>>7);
}

void RTC_DS1307::adjust(const DateTime& dt)
{
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.send(0);
    Wire.send(bin2bcd(dt.second()));
    Wire.send(bin2bcd(dt.minute()));
    Wire.send(bin2bcd(dt.hour()));
    Wire.send(bin2bcd(0));
    Wire.send(bin2bcd(dt.day()));
    Wire.send(bin2bcd(dt.month()));
    Wire.send(bin2bcd(dt.year() - 2000));
    Wire.send(0);
    Wire.endTransmission();
}

DateTime RTC_DS1307::now()
{
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.send(0);
    Wire.endTransmission();

    Wire.requestFrom(DS1307_ADDRESS, 7);
    uint8_t ss = bcd2bin(Wire.receive() & 0x7F);
    uint8_t mm = bcd2bin(Wire.receive());
    uint8_t hh = bcd2bin(Wire.receive());
    Wire.receive();
    uint8_t d = bcd2bin(Wire.receive());
    uint8_t m = bcd2bin(Wire.receive());
    uint16_t y = bcd2bin(Wire.receive()) + 2000;

    return DateTime (y, m, d, hh, mm, ss);
}

// vim:ci:sw=4 sts=4 ft=cpp
