/**************************************************************************/
/*!
  @file     RTC.h

  Original library by JeeLabs http://news.jeelabs.org/code/, released to the
  public domain

  License: MIT (see LICENSE)

  This is a fork of JeeLab's fantastic real time clock library for Arduino.

  For details on using this library with an RTC module like the DS1307, PCF8523,
  or DS3231, see the guide at:
  https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/overview

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
*/
/**************************************************************************/

#ifndef _RTC_H_
#define _RTC_H_

#include <Arduino.h>

/**************************************************************************/
/*!
    @brief  RTC superclass for all RTC chips
*/
/**************************************************************************/
class RTC {
public:
  virtual boolean begin(void) = 0;
  virtual static void adjust(const DateTime &dt) = 0;
  virtual bool isrunning(void) = 0;
  virtual bool lostPower(void) = 0;
  virtual static DateTime now() = 0;
  virtual static int readSqwPinMode() = 0;
  virtual static void writeSqwPinMode(int mode) = 0;
}

#endif