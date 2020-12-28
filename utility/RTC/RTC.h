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
#include "../DateTime/DateTime.h"

/**************************************************************************/
/*!
    @brief  RTC superclass for all RTC chips
*/
/**************************************************************************/
class RTC {
public:
  /*!
    @brief Start the RTC
    @param dt DateTime object containing desired date/time
    @return True if successful, false otherwise
  */
  virtual boolean begin(DateTime &dt) = 0;

  /*!
    @brief Adjust the RTC to the specified date/time
    @param dt DateTime object containing desired date/time
  */
  virtual void adjust(const DateTime &dt) = 0;

  /*!
    @brief Adjust the RTC's date/time to account for RTC drift
    @param drift The drift to adjust the date/time by
    @note Positive values makes the clock go ahead in time and vice-versa
  */
  virtual void adjustDrift(const int drift);

  /*!
    @brief Check if the RTC is running or not
    @return True if it is running, false otherwise
  */
  virtual boolean isrunning(void) = 0;

  /*!
    @brief Check if the RTC has lost power
    @return True if the RTC has lost power, false otherwise
    @note Equivalent to `!rtc.isrunning()`
  */
  virtual boolean lostPower(void);

  /*!
    @brief Get the current date/time from the RTC
    @return The date/time
  */
  virtual DateTime now() = 0;
};

#endif