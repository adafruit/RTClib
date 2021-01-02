/**************************************************************************/
/*!
  @file     RTC_Super.h

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

#ifndef _RTC_SUPER_H_
#define _RTC_SUPER_H_

#include "DateTime.h"
#include <Arduino.h>

/**************************************************************************/
/*!
    @brief  RTC base class for all RTC chips
*/
/**************************************************************************/
class RTC_Base {
public:
  /*!
    @brief Start the RTC while preserving the RTC's date/time
    @return True if successful, false otherwise
  */
  virtual boolean begin(void) = 0;

  /*!
    @brief Start the RTC and adjust the RTC's date/time
    @param dt DateTime object containing desired date/time
    @return True if successful, false otherwise
  */
  virtual boolean begin(const DateTime &dt);

  /*!
    @brief Adjust the RTC to the specified date/time
    @param dt DateTime object containing desired date/time
  */
  virtual void adjust(const DateTime &dt) = 0;

  /*!
    @brief  Adjust the RTC clock speed to compensate for system clock
   drift
    @param ppm Parts per million to adjust clock speed by
    @note Positive values make the clock faster and vice-versa
  */
  virtual void adjustDrift(const int ppm);

  /*!
    @brief Check if the RTC is running or not
    @return True if it is running, false otherwise
  */
  virtual boolean isrunning(void);

  /*!
    @brief Check if the RTC has lost power since last adjust()
    @return True if the RTC has lost power, false otherwise
  */
  boolean lostPower(void);

  /*!
    @brief Get the current date/time from the RTC
    @return The date/time
  */
  virtual DateTime now() = 0;
};

#endif