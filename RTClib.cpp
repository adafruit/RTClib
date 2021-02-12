/**************************************************************************/
/*!
  @file     RTClib.cpp

  @mainpage Adafruit RTClib

  @section intro Introduction

  This is a fork of JeeLab's fantastic real time clock library for Arduino.

  For details on using this library with an RTC module like the DS1307, PCF8523,
  or DS3231, see the guide at:
  https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/overview

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  @section classes Available classes

  This library provides the following classes:

  - Classes for manipulating dates, times and durations:
    - DateTime represents a specific point in time; this is the data
      type used for setting and reading the supported RTCs
    - TimeSpan represents the length of a time interval
  - Interfacing specific RTC chips:
    - RTC_DS1307
    - RTC_DS3231
    - RTC_PCF8523
  - RTC emulated in software; do not expect much accuracy out of these:
    - RTC_Millis is based on `millis()`
    - RTC_Micros is based on `micros()`; its drift rate can be tuned by
      the user

  @section license License

  Original library by JeeLabs https://jeelabs.org/pub/docs/rtclib/, released to
  the public domain.

  This version: MIT (see LICENSE)
*/
/**************************************************************************/

#include "RTClib.h"

#ifdef __AVR_ATtiny85__
#include <TinyWireM.h>
#define Wire TinyWireM
#else
#include <Wire.h>
#endif

#if (ARDUINO >= 100)
#include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
// Macro to deal with the difference in I2C write functions from old and new
// Arduino versions.
#define _I2C_WRITE write ///< Modern I2C write
#define _I2C_READ read   ///< Modern I2C read
#else
#include <WProgram.h>
#define _I2C_WRITE send   ///< Legacy I2C write
#define _I2C_READ receive ///< legacy I2C read
#endif

/**************************************************************************/
/*!
    @brief  Read a byte from an I2C register
    @param addr I2C address
    @param reg Register address
    @return Register value
*/
/**************************************************************************/
static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire._I2C_READ();
}

/**************************************************************************/
/*!
    @brief  Write a byte to an I2C register
    @param addr I2C address
    @param reg Register address
    @param val Value to write
*/
/**************************************************************************/
static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire._I2C_WRITE((byte)val);
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Convert a binary coded decimal value to binary. RTC stores time/date
   values as BCD.
    @param val BCD value
    @return Binary value
*/
/**************************************************************************/
static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }

/**************************************************************************/
/*!
    @brief  Convert a binary value to BCD format for the RTC registers
    @param val Binary value
    @return BCD value
*/
/**************************************************************************/
static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

/**************************************************************************/
/*!
    @brief  Start I2C for the DS1307 and test succesful connection
    @return True if Wire can find DS1307 or false otherwise.
    @note Preserves the date/time on the RTC
*/
/**************************************************************************/
boolean RTC_DS1307::begin(void) {
  Wire.begin();
  Wire.beginTransmission(DS1307_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;

  return false;
}

/**************************************************************************/
/*!
    @brief  Start I2C for the DS1307, test succesful connection, then initialise
   the date/time
    @param dt DateTime object containing desired date/time
    @return True if Wire can find DS1307 or false otherwise.
*/
/**************************************************************************/
boolean RTC_DS1307::begin(const DateTime &dt) {
  Wire.begin();
  Wire.beginTransmission(DS1307_ADDRESS);
  if (Wire.endTransmission() == 0) {
    adjust(dt);
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief  Is the DS1307 running? Check the Clock Halt bit in register 0
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
boolean RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return !(ss >> 7);
}

/**************************************************************************/
/*!
    @brief  Check the status register Oscillator Stop flag to see if the DS1307
   stopped due to power loss
    @return True if the bit is set (oscillator is or has stopped) and false only
   after the bit is cleared, for instance with adjust()
*/
/**************************************************************************/
boolean RTC_DS1307::lostPower(void) {
  return (read_i2c_register(DS1307_ADDRESS, 0) >> 7);
}

/**************************************************************************/
/*!
    @brief  Set the date and time in the DS1307
    @param dt DateTime object containing the desired date/time
*/
/**************************************************************************/
void RTC_DS1307::adjust(const DateTime &dt) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000U));
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Get the current date and time from the DS1307
    @return DateTime object containing the current date and time
*/
/**************************************************************************/
DateTime RTC_DS1307::now() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();
  uint8_t d = bcd2bin(Wire._I2C_READ());
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000U;

  return DateTime(y, m, d, hh, mm, ss);
}

/**************************************************************************/
/*!
    @brief  Read the current mode of the SQW pin
    @return Mode as Ds1307SqwPinMode enum
*/
/**************************************************************************/
Ds1307SqwPinMode RTC_DS1307::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds1307SqwPinMode>(mode);
}

/**************************************************************************/
/*!
    @brief  Change the SQW pin mode
    @param mode The mode to use
*/
/**************************************************************************/
void RTC_DS1307::writeSqwPinMode(Ds1307SqwPinMode mode) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire._I2C_WRITE(mode);
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Read data from the DS1307's NVRAM
    @param buf Pointer to a buffer to store the data - make sure it's large
   enough to hold size bytes
    @param size Number of bytes to read
    @param address Starting NVRAM address, from 0 to 55
*/
/**************************************************************************/
void RTC_DS1307::readnvram(uint8_t *buf, uint8_t size, uint8_t address) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)DS1307_ADDRESS, size);
  for (uint8_t pos = 0; pos < size; ++pos) {
    buf[pos] = Wire._I2C_READ();
  }
}

/**************************************************************************/
/*!
    @brief  Write data to the DS1307 NVRAM
    @param address Starting NVRAM address, from 0 to 55
    @param buf Pointer to buffer containing the data to write
    @param size Number of bytes in buf to write to NVRAM
*/
/**************************************************************************/
void RTC_DS1307::writenvram(uint8_t address, uint8_t *buf, uint8_t size) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  for (uint8_t pos = 0; pos < size; ++pos) {
    Wire._I2C_WRITE(buf[pos]);
  }
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Shortcut to read one byte from NVRAM
    @param address NVRAM address, 0 to 55
    @return The byte read from NVRAM
*/
/**************************************************************************/
uint8_t RTC_DS1307::readnvram(uint8_t address) {
  uint8_t data;
  readnvram(&data, 1, address);
  return data;
}

/**************************************************************************/
/*!
    @brief  Shortcut to write one byte to NVRAM
    @param address NVRAM address, 0 to 55
    @param data One byte to write
*/
/**************************************************************************/
void RTC_DS1307::writenvram(uint8_t address, uint8_t data) {
  writenvram(address, &data, 1);
}

/** Alignment between the milis() timescale and the Unix timescale. These
  two variables are updated on each call to now(), which prevents
  rollover issues. Note that lastMillis is **not** the millis() value
  of the last call to now(): it's the millis() value corresponding to
  the last **full second** of Unix time. */
uint32_t RTC_Millis::lastMillis;
uint32_t RTC_Millis::lastUnix;

/**************************************************************************/
/*!
    @brief  Start the RTC_Millis date/time
    @return true
    @note Equivalent to `begin(DateTime())`
*/
/**************************************************************************/
boolean RTC_Millis::begin(void) {
  adjust(DateTime());
  return true;
}
/**************************************************************************/
/*!
    @brief  Start the RTC_Millis date/time
    @param dt DateTime object containing desired date/time
    @return true
*/
/**************************************************************************/
boolean RTC_Millis::begin(const DateTime &dt) {
  adjust(dt);
  return true;
}

/**************************************************************************/
/*!
    @brief  Set the current date/time of the RTC_Millis clock.
    @param dt DateTime object with the desired date and time
*/
/**************************************************************************/
void RTC_Millis::adjust(const DateTime &dt) {
  lastMillis = millis();
  lastUnix = dt.unixtime();
}

/**************************************************************************/
/*!
    @brief  Return a DateTime object containing the current date/time.
            Note that computing (millis() - lastMillis) is rollover-safe as long
            as this method is called at least once every 49.7 days.
    @return DateTime object containing current time
*/
/**************************************************************************/
DateTime RTC_Millis::now() {
  uint32_t elapsedSeconds = (millis() - lastMillis) / 1000;
  lastMillis += elapsedSeconds * 1000;
  lastUnix += elapsedSeconds;
  return lastUnix;
}

/** Number of microseconds reported by micros() per "true" (calibrated) second.
 */
uint32_t RTC_Micros::microsPerSecond = 1000000L;

/** The timing logic is identical to RTC_Millis. */
uint32_t RTC_Micros::lastMicros;
uint32_t RTC_Micros::lastUnix;

/**************************************************************************/
/*!
    @brief  Start the RTC_Micros date/time
    @return true
    @note Equivalent to `begin(DateTime())`
*/
/**************************************************************************/
boolean RTC_Micros::begin(void) {
  adjust(DateTime());
  return true;
}

/**************************************************************************/
/*!
    @brief  Start the RTC_Micros date/time
    @param dt DateTime object containing desired date/time
    @return true
*/
/**************************************************************************/
boolean RTC_Micros::begin(const DateTime &dt) {
  adjust(dt);
  return true;
}

/**************************************************************************/
/*!
    @brief  Set the current date/time of the RTC_Micros clock.
    @param dt DateTime object with the desired date and time
*/
/**************************************************************************/
void RTC_Micros::adjust(const DateTime &dt) {
  lastMicros = micros();
  lastUnix = dt.unixtime();
}

/**************************************************************************/
/*!
    @brief  Adjust the RTC_Micros clock speed to compensate for system clock
   drift
    @param ppm Parts per million drift rate adjustment
    @note Positive values make the clock faster and vice-versa
*/
/**************************************************************************/
void RTC_Micros::adjustDrift(int ppm) { microsPerSecond = 1000000L - ppm; }

/**************************************************************************/
/*!
    @brief  Get the current date/time from the RTC_Micros clock.
    @return DateTime object containing the current date/time
*/
/**************************************************************************/
DateTime RTC_Micros::now() {
  uint32_t elapsedSeconds = (micros() - lastMicros) / microsPerSecond;
  lastMicros += elapsedSeconds * microsPerSecond;
  lastUnix += elapsedSeconds;
  return lastUnix;
}

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8523 and test succesful connection
    @return True if Wire can find PCF8523 or false otherwise.
    @note Preserves the date/time on the RTC
*/
/**************************************************************************/
boolean RTC_PCF8523::begin(void) {
  Wire.begin();
  Wire.beginTransmission(PCF8523_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;

  return false;
}

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8523 and test succesful connection
    @param dt DateTime object containing desired date/time
    @return True if Wire can find PCF8523 or false otherwise.
*/
/**************************************************************************/
boolean RTC_PCF8523::begin(const DateTime &dt) {
  Wire.begin();
  Wire.beginTransmission(PCF8523_ADDRESS);
  if (Wire.endTransmission() == 0) {
    adjust(dt);
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief  Check the status register Oscillator Stop flag to see if the PCF8523
   stopped due to power loss
    @details When battery or external power is first applied, the PCF8523's
   crystal oscillator takes up to 2s to stabilize. During this time adjust()
   cannot clear the 'OS' flag. See datasheet OS flag section for details.
    @return True if the bit is set (oscillator is or has stopped) and false only
   after the bit is cleared, for instance with adjust()
*/
/**************************************************************************/
boolean RTC_PCF8523::lostPower(void) {
  return (read_i2c_register(PCF8523_ADDRESS, PCF8523_STATUSREG) >> 7);
}

/**************************************************************************/
/*!
    @brief  Check control register 3 to see if we've run adjust() yet (setting
   the date/time and battery switchover mode)
    @return True if the PCF8523 has been set up, false if not
*/
/**************************************************************************/
boolean RTC_PCF8523::initialized(void) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return ((ss & 0xE0) != 0xE0); // 0xE0 = standby mode, set after power out
}

/**************************************************************************/
/*!
    @brief  Set the date and time, set battery switchover mode
    @param dt DateTime to set
*/
/**************************************************************************/
void RTC_PCF8523::adjust(const DateTime &dt) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3); // start at location 3
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(0)); // skip weekdays
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000U));
  Wire.endTransmission();

  // set to battery switchover mode
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire._I2C_WRITE((byte)0x00);
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object containing the current date/time
*/
/**************************************************************************/
DateTime RTC_PCF8523::now() {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3);
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  uint8_t d = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ(); // skip 'weekdays'
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000U;

  return DateTime(y, m, d, hh, mm, ss);
}

/**************************************************************************/
/*!
    @brief  resets the stop bit in register control_1
*/
/**************************************************************************/
void RTC_PCF8523::start(void) {
  uint8_t ctlreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1);
  if (ctlreg & (1 << 5)) {
    write_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1, ctlreg & ~(1 << 5));
  }
}

/**************************************************************************/
/*!
    @brief  Sets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8523::stop(void) {
  uint8_t ctlreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1);
  if (!(ctlreg & (1 << 5))) {
    write_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1, ctlreg | (1 << 5));
  }
}

/**************************************************************************/
/*!
    @brief  Is the PCF8523 running? Check the STOP bit in register Control_1
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
boolean RTC_PCF8523::isrunning() {
  uint8_t ctlreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1);
  return !((ctlreg >> 5) & 1);
}

/**************************************************************************/
/*!
    @brief  Read the mode of the INT/SQW pin on the PCF8523
    @return SQW pin mode as a #Pcf8523SqwPinMode enum
*/
/**************************************************************************/
Pcf8523SqwPinMode RTC_PCF8523::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)PCF8523_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode >>= 3;
  mode &= 0x7;
  return static_cast<Pcf8523SqwPinMode>(mode);
}

/**************************************************************************/
/*!
    @brief  Set the INT/SQW pin mode on the PCF8523
    @param mode The mode to set, see the #Pcf8523SqwPinMode enum for options
*/
/**************************************************************************/
void RTC_PCF8523::writeSqwPinMode(Pcf8523SqwPinMode mode) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire._I2C_WRITE(mode << 3); // disables other timers
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Enable the Second Timer (1Hz) Interrupt on the PCF8523.
    @details The INT/SQW pin will pull low for a brief pulse once per second.
*/
/**************************************************************************/
void RTC_PCF8523::enableSecondTimer() {
  // Leave compatible settings intact
  uint8_t ctlreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1);
  uint8_t clkreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL);

  // TAM pulse int. mode (shared with Timer A), CLKOUT (aka SQW) disabled
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL, clkreg | 0xB8);

  // SIE Second timer int. enable
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1, ctlreg | (1 << 2));
}

/**************************************************************************/
/*!
    @brief  Disable the Second Timer (1Hz) Interrupt on the PCF8523.
*/
/**************************************************************************/
void RTC_PCF8523::disableSecondTimer() {
  // Leave compatible settings intact
  uint8_t ctlreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1);

  // SIE Second timer int. disable
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_1, ctlreg & ~(1 << 2));
}

/**************************************************************************/
/*!
    @brief  Enable the Countdown Timer Interrupt on the PCF8523.
    @details The INT/SQW pin will be pulled low at the end of a specified
   countdown period ranging from 244 microseconds to 10.625 days.
    Uses PCF8523 Timer B. Any existing CLKOUT square wave, configured with
   writeSqwPinMode(), will halt. The interrupt low pulse width is adjustable
   from 3/64ths (default) to 14/64ths of a second.
    @param clkFreq One of the PCF8523's Timer Source Clock Frequencies.
   See the #PCF8523TimerClockFreq enum for options and associated time ranges.
    @param numPeriods The number of clkFreq periods (1-255) to count down.
    @param lowPulseWidth Optional: the length of time for the interrupt pin
   low pulse. See the #PCF8523TimerIntPulse enum for options.
*/
/**************************************************************************/
void RTC_PCF8523::enableCountdownTimer(PCF8523TimerClockFreq clkFreq,
                                       uint8_t numPeriods,
                                       uint8_t lowPulseWidth) {
  // Datasheet cautions against updating countdown value while it's running,
  // so disabling allows repeated calls with new values to set new countdowns
  disableCountdownTimer();

  // Leave compatible settings intact
  uint8_t ctlreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_2);
  uint8_t clkreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL);

  // CTBIE Countdown Timer B Interrupt Enabled
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_2, ctlreg |= 0x01);

  // Timer B source clock frequency, optionally int. low pulse width
  write_i2c_register(PCF8523_ADDRESS, PCF8523_TIMER_B_FRCTL,
                     lowPulseWidth << 4 | clkFreq);

  // Timer B value (number of source clock periods)
  write_i2c_register(PCF8523_ADDRESS, PCF8523_TIMER_B_VALUE, numPeriods);

  // TBM Timer B pulse int. mode, CLKOUT (aka SQW) disabled, TBC start Timer B
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL, clkreg | 0x79);
}

/**************************************************************************/
/*!
    @overload
    @brief  Enable Countdown Timer using default interrupt low pulse width.
    @param clkFreq One of the PCF8523's Timer Source Clock Frequencies.
   See the #PCF8523TimerClockFreq enum for options and associated time ranges.
    @param numPeriods The number of clkFreq periods (1-255) to count down.
*/
/**************************************************************************/
void RTC_PCF8523::enableCountdownTimer(PCF8523TimerClockFreq clkFreq,
                                       uint8_t numPeriods) {
  enableCountdownTimer(clkFreq, numPeriods, 0);
}

/**************************************************************************/
/*!
    @brief  Disable the Countdown Timer Interrupt on the PCF8523.
    @details For simplicity, this function strictly disables Timer B by setting
   TBC to 0. The datasheet describes TBC as the Timer B on/off switch.
   Timer B is the only countdown timer implemented at this time.
   The following flags have no effect while TBC is off, they are *not* cleared:
      - TBM: Timer B will still be set to pulsed mode.
      - CTBIE: Timer B interrupt would be triggered if TBC were on.
      - CTBF: Timer B flag indicates that interrupt was triggered. Though
        typically used for non-pulsed mode, user may wish to query this later.
*/
/**************************************************************************/
void RTC_PCF8523::disableCountdownTimer() {
  // Leave compatible settings intact
  uint8_t clkreg = read_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL);

  // TBC disable to stop Timer B clock
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL, ~1 & clkreg);
}

/**************************************************************************/
/*!
    @brief  Stop all timers, clear their flags and settings on the PCF8523.
    @details This includes the Countdown Timer, Second Timer, and any CLKOUT
   square wave configured with writeSqwPinMode().
*/
/**************************************************************************/
void RTC_PCF8523::deconfigureAllTimers() {
  disableSecondTimer(); // Surgically clears CONTROL_1
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CONTROL_2, 0);
  write_i2c_register(PCF8523_ADDRESS, PCF8523_CLKOUTCONTROL, 0);
  write_i2c_register(PCF8523_ADDRESS, PCF8523_TIMER_B_FRCTL, 0);
  write_i2c_register(PCF8523_ADDRESS, PCF8523_TIMER_B_VALUE, 0);
}

/**************************************************************************/
/*!
    @brief Compensate the drift of the RTC.
    @details This method sets the "offset" register of the PCF8523,
      which can be used to correct a previously measured drift rate.
      Two correction modes are available:

      - **PCF8523\_TwoHours**: Clock adjustments are performed on
        `offset` consecutive minutes every two hours. This is the most
        energy-efficient mode.

      - **PCF8523\_OneMinute**: Clock adjustments are performed on
        `offset` consecutive seconds every minute. Extra adjustments are
        performed on the last second of the minute is `abs(offset)>60`.

      The `offset` parameter sets the correction amount in units of
      roughly 4&nbsp;ppm. The exact unit depends on the selected mode:

      |  mode               | offset unit                            |
      |---------------------|----------------------------------------|
      | `PCF8523_TwoHours`  | 4.340 ppm = 0.375 s/day = 2.625 s/week |
      | `PCF8523_OneMinute` | 4.069 ppm = 0.352 s/day = 2.461 s/week |

      See the accompanying sketch pcf8523.ino for an example on how to
      use this method.

    @param mode Correction mode, either `PCF8523_TwoHours` or
      `PCF8523_OneMinute`.
    @param offset Correction amount, from -64 to +63. A positive offset
      makes the clock slower.
*/
/**************************************************************************/
void RTC_PCF8523::calibrate(Pcf8523OffsetMode mode, int8_t offset) {
  uint8_t reg = (uint8_t)offset & 0x7F;
  reg |= mode;

  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_OFFSET);
  Wire._I2C_WRITE(reg);
  Wire.endTransmission();
}

// START RTC_PCF8563 implementation

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8563 and test succesful connection
    @return True if Wire can find PCF8563 or false otherwise.
    @note Preserves the date/time on the RTC
*/
/**************************************************************************/
boolean RTC_PCF8563::begin(void) {
  Wire.begin();
  Wire.beginTransmission(PCF8563_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;

  return false;
}

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8563 and test succesful connection
    @param dt DateTime object containing desired date/time
    @return True if Wire can find PCF8563 or false otherwise.
*/
/**************************************************************************/
boolean RTC_PCF8563::begin(const DateTime &dt) {
  Wire.begin();
  Wire.beginTransmission(PCF8563_ADDRESS);
  if (Wire.endTransmission() == 0) {
    adjust(dt);
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief  Check the status of the VL bit in the VL_SECONDS register.
    @details The PCF8563 has an on-chip voltage-low detector. When VDD drops
     below Vlow, bit VL in the VL_seconds register is set to indicate that
     the integrity of the clock information is no longer guaranteed.
    @return True if the bit is set (VDD droped below Vlow) indicating that
    the clock integrity is not guaranteed and false only after the bit is
    cleared using adjust()
*/
/**************************************************************************/

boolean RTC_PCF8563::lostPower(void) {
  return (read_i2c_register(PCF8563_ADDRESS, PCF8563_VL_SECONDS) >> 7);
}

/**************************************************************************/
/*!
    @brief  Set the date and time
    @param dt DateTime to set
*/
/**************************************************************************/
void RTC_PCF8563::adjust(const DateTime &dt) {

  Wire.beginTransmission(PCF8563_ADDRESS);
  Wire._I2C_WRITE(PCF8563_VL_SECONDS); // start at location 2, VL_SECONDS
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(0)); // skip weekdays
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object containing the current date/time
*/
/**************************************************************************/

DateTime RTC_PCF8563::now() {
  Wire.beginTransmission(PCF8563_ADDRESS);
  Wire._I2C_WRITE((byte)2);
  Wire.endTransmission();

  Wire.requestFrom(PCF8563_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t hh = bcd2bin(Wire._I2C_READ() & 0x3F);
  uint8_t d = bcd2bin(Wire._I2C_READ() & 0x3F);
  Wire._I2C_READ(); // skip 'weekdays'
  uint8_t m = bcd2bin(Wire._I2C_READ() & 0x1F);
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;

  return DateTime(y, m, d, hh, mm, ss);
}

/**************************************************************************/
/*!
    @brief  Resets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8563::start(void) {
  uint8_t ctlreg = read_i2c_register(PCF8563_ADDRESS, PCF8563_CONTROL_1);
  if (ctlreg & (1 << 5)) {
    write_i2c_register(PCF8563_ADDRESS, PCF8563_CONTROL_1, ctlreg & ~(1 << 5));
  }
}

/**************************************************************************/
/*!
    @brief  Sets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8563::stop(void) {
  uint8_t ctlreg = read_i2c_register(PCF8563_ADDRESS, PCF8563_CONTROL_1);
  if (!(ctlreg & (1 << 5))) {
    write_i2c_register(PCF8523_ADDRESS, PCF8563_CONTROL_1, ctlreg | (1 << 5));
  }
}

/**************************************************************************/
/*!
    @brief  Is the PCF8563 running? Check the STOP bit in register Control_1
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
boolean RTC_PCF8563::isrunning() {
  uint8_t ctlreg = read_i2c_register(PCF8563_ADDRESS, PCF8563_CONTROL_1);
  return !((ctlreg >> 5) & 1);
}

/**************************************************************************/
/*!
    @brief  Read the mode of the CLKOUT pin on the PCF8563
    @return CLKOUT pin mode as a #Pcf8563SqwPinMode enum
*/
/**************************************************************************/
Pcf8563SqwPinMode RTC_PCF8563::readSqwPinMode() {

  int mode;

  Wire.beginTransmission(PCF8563_ADDRESS);
  Wire._I2C_WRITE(PCF8563_CLKOUTCONTROL);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)PCF8563_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  return static_cast<Pcf8563SqwPinMode>(mode & PCF8563_CLKOUT_MASK);
}

/**************************************************************************/
/*!
    @brief  Set the CLKOUT pin mode on the PCF8563
    @param mode The mode to set, see the #Pcf8563SqwPinMode enum for options
*/
/**************************************************************************/
void RTC_PCF8563::writeSqwPinMode(Pcf8563SqwPinMode mode) {

  Wire.beginTransmission(PCF8563_ADDRESS);
  Wire._I2C_WRITE(PCF8563_CLKOUTCONTROL);
  Wire._I2C_WRITE(mode);
  Wire.endTransmission();
}
// END RTC_PCF8563 implementation

/**************************************************************************/
/*!
    @brief  Convert the day of the week to a representation suitable for
            storing in the DS3231: from 1 (Monday) to 7 (Sunday).
    @param  d Day of the week as represented by the library:
            from 0 (Sunday) to 6 (Saturday).
*/
/**************************************************************************/
static uint8_t dowToDS3231(uint8_t d) { return d == 0 ? 7 : d; }

/**************************************************************************/
/*!
    @brief  Start I2C for the DS3231 and test succesful connection
    @return True if Wire can find DS3231 or false otherwise.
    @note Preserves the date/time on the RTC
*/
/**************************************************************************/
boolean RTC_DS3231::begin(void) {
  Wire.begin();
  Wire.beginTransmission(DS3231_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;

  return false;
}

/**************************************************************************/
/*!
    @brief  Start I2C for the DS3231 and test succesful connection
    @param dt DateTime object containing desired date/time
    @return True if Wire can find DS3231 or false otherwise.
*/
/**************************************************************************/
boolean RTC_DS3231::begin(const DateTime &dt) {
  Wire.begin();
  Wire.beginTransmission(DS3231_ADDRESS);
  if (Wire.endTransmission() == 0) {
    adjust(dt);
    return true;
  }
  return false;
}

/**************************************************************************/
/*!
    @brief  Is the DS3231 running? Check the Clock Halt bit in register 0
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
bool RTC_DS3231::isrunning(void) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return !(ss >> 7);
}

/**************************************************************************/
/*!
    @brief  Check the status register Oscillator Stop Flag to see if the DS3231
   stopped due to power loss
    @return True if the bit is set (oscillator stopped) or false if it is
   running
*/
/**************************************************************************/
bool RTC_DS3231::lostPower(void) {
  return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

/**************************************************************************/
/*!
    @brief  Set the date and flip the Oscillator Stop Flag
    @param dt DateTime object containing the date/time to set
*/
/**************************************************************************/
void RTC_DS3231::adjust(const DateTime &dt) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)DS3231_TIME); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  // The RTC must know the day of the week for the weekly alarms to work.
  Wire._I2C_WRITE(bin2bcd(dowToDS3231(dt.dayOfTheWeek())));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000U));
  Wire.endTransmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object with the current date/time
*/
/**************************************************************************/
DateTime RTC_DS3231::now() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();
  uint8_t d = bcd2bin(Wire._I2C_READ());
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000U;

  return DateTime(y, m, d, hh, mm, ss);
}

/**************************************************************************/
/*!
    @brief  Read the SQW pin mode
    @return Pin mode, see Ds3231SqwPinMode enum
*/
/**************************************************************************/
Ds3231SqwPinMode RTC_DS3231::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_CONTROL);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x1C;
  if (mode & 0x04)
    mode = DS3231_OFF;
  return static_cast<Ds3231SqwPinMode>(mode);
}

/**************************************************************************/
/*!
    @brief  Set the SQW pin mode
    @param mode Desired mode, see Ds3231SqwPinMode enum
*/
/**************************************************************************/
void RTC_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode) {
  uint8_t ctrl;
  ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  ctrl |= mode;
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);

  // Serial.println( read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), HEX);
}

/**************************************************************************/
/*!
    @brief  Get the current temperature from the DS3231's temperature sensor
    @return Current temperature (float)
*/
/**************************************************************************/
float RTC_DS3231::getTemperature() {
  uint8_t lsb;
  int8_t msb;
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_TEMPERATUREREG);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 2);
  msb = Wire._I2C_READ();
  lsb = Wire._I2C_READ();

  //  Serial.print("msb=");
  //  Serial.print(msb,HEX);
  //  Serial.print(", lsb=");
  //  Serial.println(lsb,HEX);

  return (float)msb + (lsb >> 6) * 0.25f;
}

/**************************************************************************/
/*!
    @brief  Set alarm 1 for DS3231
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see Ds3231Alarm1Mode enum
    @return False if control register is not set, otherwise true
*/
/**************************************************************************/
bool RTC_DS3231::setAlarm1(const DateTime &dt, Ds3231Alarm1Mode alarm_mode) {
  uint8_t ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);
  if (!(ctrl & 0x04)) {
    return false;
  }

  uint8_t A1M1 = (alarm_mode & 0x01) << 7; // Seconds bit 7.
  uint8_t A1M2 = (alarm_mode & 0x02) << 6; // Minutes bit 7.
  uint8_t A1M3 = (alarm_mode & 0x04) << 5; // Hour bit 7.
  uint8_t A1M4 = (alarm_mode & 0x08) << 4; // Day/Date bit 7.
  uint8_t DY_DT = (alarm_mode & 0x10)
                  << 2; // Day/Date bit 6. Date when 0, day of week when 1.

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_ALARM1);
  Wire._I2C_WRITE(bin2bcd(dt.second()) | A1M1);
  Wire._I2C_WRITE(bin2bcd(dt.minute()) | A1M2);
  Wire._I2C_WRITE(bin2bcd(dt.hour()) | A1M3);
  if (DY_DT) {
    Wire._I2C_WRITE(bin2bcd(dowToDS3231(dt.dayOfTheWeek())) | A1M4 | DY_DT);
  } else {
    Wire._I2C_WRITE(bin2bcd(dt.day()) | A1M4 | DY_DT);
  }
  Wire.endTransmission();

  ctrl |= 0x01; // AI1E
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);
  return true;
}

/**************************************************************************/
/*!
    @brief  Set alarm 2 for DS3231
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see Ds3231Alarm2Mode enum
    @return False if control register is not set, otherwise true
*/
/**************************************************************************/
bool RTC_DS3231::setAlarm2(const DateTime &dt, Ds3231Alarm2Mode alarm_mode) {
  uint8_t ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);
  if (!(ctrl & 0x04)) {
    return false;
  }

  uint8_t A2M2 = (alarm_mode & 0x01) << 7; // Minutes bit 7.
  uint8_t A2M3 = (alarm_mode & 0x02) << 6; // Hour bit 7.
  uint8_t A2M4 = (alarm_mode & 0x04) << 5; // Day/Date bit 7.
  uint8_t DY_DT = (alarm_mode & 0x8)
                  << 3; // Day/Date bit 6. Date when 0, day of week when 1.

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_ALARM2);
  Wire._I2C_WRITE(bin2bcd(dt.minute()) | A2M2);
  Wire._I2C_WRITE(bin2bcd(dt.hour()) | A2M3);
  if (DY_DT) {
    Wire._I2C_WRITE(bin2bcd(dowToDS3231(dt.dayOfTheWeek())) | A2M4 | DY_DT);
  } else {
    Wire._I2C_WRITE(bin2bcd(dt.day()) | A2M4 | DY_DT);
  }
  Wire.endTransmission();

  ctrl |= 0x02; // AI2E
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);
  return true;
}

/**************************************************************************/
/*!
    @brief  Disable alarm
        @param 	alarm_num Alarm number to disable
*/
/**************************************************************************/
void RTC_DS3231::disableAlarm(uint8_t alarm_num) {
  uint8_t ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);
  ctrl &= ~(1 << (alarm_num - 1));
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);
}

/**************************************************************************/
/*!
    @brief  Clear status of alarm
        @param 	alarm_num Alarm number to clear
*/
/**************************************************************************/
void RTC_DS3231::clearAlarm(uint8_t alarm_num) {
  uint8_t status = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  status &= ~(0x1 << (alarm_num - 1));
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, status);
}

/**************************************************************************/
/*!
    @brief  Get status of alarm
        @param 	alarm_num Alarm number to check status of
        @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_DS3231::alarmFired(uint8_t alarm_num) {
  uint8_t status = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  return (status >> (alarm_num - 1)) & 0x1;
}

/**************************************************************************/
/*!
    @brief  Enable 32KHz Output
    @details The 32kHz output is enabled by default. It requires an external
    pull-up resistor to function correctly
*/
/**************************************************************************/
void RTC_DS3231::enable32K(void) {
  uint8_t status = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  status |= (0x1 << 0x03);
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, status);
  // Serial.println(read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG), BIN);
}

/**************************************************************************/
/*!
    @brief  Disable 32KHz Output
*/
/**************************************************************************/
void RTC_DS3231::disable32K(void) {
  uint8_t status = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  status &= ~(0x1 << 0x03);
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, status);
  // Serial.println(read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG), BIN);
}

/**************************************************************************/
/*!
    @brief  Get status of 32KHz Output
    @return True if enabled otherwise false
*/
/**************************************************************************/
bool RTC_DS3231::isEnabled32K(void) {
  uint8_t status = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  return (status >> 0x03) & 0x1;
}
