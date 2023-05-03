#include "RTClib.h"

#define PCF8563_ADDRESS 0x51       ///< I2C address for PCF8563
#define PCF8563_CLKOUTCONTROL 0x0D ///< CLKOUT control register
#define PCF8563_CONTROL_1 0x00     ///< Control and status register 1
#define PCF8563_CONTROL_2 0x01     ///< Control and status register 2
#define PCF8563_VL_SECONDS 0x02    ///< register address for VL_SECONDS
#define PCF8563_CLKOUT_MASK 0x83   ///< bitmask for SqwPinMode on CLKOUT pin

#define PCF8563_MINUTE_ALARM 0x09 ///< Minute alarm register

#define PCF8563_TIMER_CONTROL 0x0E ///< Timer control register
#define PCF8563_TIMER_VALUE 0x0F   ///< Timer register

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8563 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find PCF8563 or false otherwise.
*/
/**************************************************************************/
bool RTC_PCF8563::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(PCF8563_ADDRESS, wireInstance);
  if (!i2c_dev->begin())
    return false;
  return true;
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
bool RTC_PCF8563::lostPower(void) {
  return read_register(PCF8563_VL_SECONDS) >> 7;
}

/**************************************************************************/
/*!
    @brief  Set the date and time
    @param dt DateTime to set
*/
/**************************************************************************/
void RTC_PCF8563::adjust(const DateTime &dt) {
  uint8_t buffer[8] = {
      PCF8563_VL_SECONDS, // start at location 2, VL_SECONDS
      bin2bcd(dt.second()), bin2bcd(dt.minute()),
      bin2bcd(dt.hour()),   bin2bcd(dt.day()),
      dt.dayOfTheWeek(), // Weekday needs to be set for alarms to work correctly
      bin2bcd(dt.month()),  bin2bcd(dt.year() - 2000U)};
  i2c_dev->write(buffer, 8);
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object containing the current date/time
*/
/**************************************************************************/
DateTime RTC_PCF8563::now() {
  uint8_t buffer[7];
  buffer[0] = PCF8563_VL_SECONDS; // start at location 2, VL_SECONDS
  i2c_dev->write_then_read(buffer, 1, buffer, 7);

  return DateTime(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5] & 0x1F),
                  bcd2bin(buffer[3] & 0x3F), bcd2bin(buffer[2] & 0x3F),
                  bcd2bin(buffer[1] & 0x7F), bcd2bin(buffer[0] & 0x7F));
}

/**************************************************************************/
/*!
    @brief  Resets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8563::start(void) {
  uint8_t ctlreg = read_register(PCF8563_CONTROL_1);
  if (ctlreg & (1 << 5))
    write_register(PCF8563_CONTROL_1, ctlreg & ~(1 << 5));
}

/**************************************************************************/
/*!
    @brief  Sets the STOP bit in register Control_1
*/
/**************************************************************************/
void RTC_PCF8563::stop(void) {
  uint8_t ctlreg = read_register(PCF8563_CONTROL_1);
  if (!(ctlreg & (1 << 5)))
    write_register(PCF8563_CONTROL_1, ctlreg | (1 << 5));
}

/**************************************************************************/
/*!
    @brief  Is the PCF8563 running? Check the STOP bit in register Control_1
    @return 1 if the RTC is running, 0 if not
*/
/**************************************************************************/
uint8_t RTC_PCF8563::isrunning() {
  return !((read_register(PCF8563_CONTROL_1) >> 5) & 1);
}

/**************************************************************************/
/*!
    @brief  Get status of timer
    @return True if timer has been fired otherwise false
*/
/**************************************************************************/
bool RTC_PCF8563::timerFired() {
  return (read_register(PCF8563_CONTROL_2) & 0x04);
}

/**************************************************************************/
/*!
    @brief  Clear status of timer flag
*/
/**************************************************************************/
void RTC_PCF8563::clearTimer() {
  write_register(PCF8563_CONTROL_2, read_register(PCF8563_CONTROL_2) & ~0x04);
}

/**************************************************************************/
/*!
    @brief  Get status of alarm
    @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_PCF8563::alarmFired() {
  return read_register(PCF8563_CONTROL_2) & 0x08;
}

/**************************************************************************/
/*!
    @brief  Clear status of alarm flag
*/
/**************************************************************************/
void RTC_PCF8563::clearAlarm() {
  write_register(PCF8563_CONTROL_2, read_register(PCF8563_CONTROL_2) & ~0x08);
}

/**************************************************************************/
/*!
    @brief  Disable the Countdown Timer on the PCF8523.
    @details For simplicity, this function strictly disables Timer by setting
   TE to 0. The datasheet describes TE as the Timer on/off switch.
   The following flags have no effect while TE is off, they are *not* cleared:
      - TF: Timer flag
      - TIE: Timer interrupt would be triggered if TBC were on.
      - Set timer source clock frequency to 1/60 Hz:
      These bits determine the source clock for the countdown timer; when not in
      use, TD[1:0] should be set to 1⁄60 Hz for power saving.
*/
/**************************************************************************/
void RTC_PCF8563::disableTimer() {
  // clear TE to disable Timer (Bit 7 of Timer Control)
  write_register(PCF8563_TIMER_CONTROL, 0x03);

  // clear TIE, TF and TI_TP flags (Bit 4, 2, 0 of Control_2)
  write_register(PCF8563_CONTROL_2, read_register(PCF8563_CONTROL_2) & ~0x15);

  write_register(PCF8563_TIMER_VALUE, 0);
}

/**************************************************************************/
/*!
  @brief  Enable the Countdown Timer Interrupt on the PCF8563.
  @param clkFreq One of the PCF8623's Timer Source Clock Frequencies.
    See the PCF8563TimerClockFreq enum for options and associated time ranges.
  @param numPeriods The number of clkFreq periods (1-255) to count down.
  @param pulse Default false, if true, the INT pin will go low for a short
    time when the timer interrupt is fired. If false, the INT pin will go
    low until the timer interrupt is cleared. The time the INT pin is low
    depends on the clkFreq and numPeriods. Check Docs for exact timings.
  @note If AF and AIE are active then INT will be permanently active.
  @details When pulse is true the Timer Interrupt Pulse (TI_TP) uses the
    internal clock and is dependent on the selected source clock for the
    countdown timer and countdown value. As a consequence, the width of the
    interrupt pulse varies. For loaded countdown values of numPeriods=1:
      - at 4kHz   INT is low for  1/8192 seconds
      - at 64Hz   INT is low for  1/128 seconds
      - at 1Hz    INT is low for  1/64 seconds
      - at 1/60Hz INT is low for  1/64 seconds
    For loaded countdown values of numPeriods>1:
      - at 4kHz   INT is low for  1/4096 seconds
      - at 64Hz   INT is low for  1/64 seconds
      - at 1Hz    INT is low for  1/64 seconds
      - at 1/60Hz INT is low for  1/64 seconds
*/
/**************************************************************************/
void RTC_PCF8563::enableTimer(Pcf8563TimerClockFreq clkFreq, uint8_t numPeriods,
                              bool pulse) {
  // Datasheet cautions against updating countdown value while it's running,
  // so disabling allows repeated calls with new values to set new countdowns
  disableTimer();

  if (pulse) {
    write_register(PCF8563_CONTROL_2, read_register(PCF8563_CONTROL_2) | 0x10);
  } else {
    write_register(PCF8563_CONTROL_2, read_register(PCF8563_CONTROL_2) & ~0x10);
  }

  // Timer value (number of source clock periods)
  write_register(PCF8563_TIMER_VALUE, numPeriods);

  // TIE Timer interrupt enabled (Bit 0 of Control_2)
  write_register(PCF8563_CONTROL_2, read_register(PCF8563_CONTROL_2) | 0x01);

  // Set timer frequency and enable timer (TE, Bit 7)
  write_register(PCF8563_TIMER_CONTROL, clkFreq | 0x80);
}

/**************************************************************************/
/*!
  @brief  Enable the Countdown Timer Interrupt on the PCF8563.
  @param clkFreq One of the PCF8623's Timer Source Clock Frequencies.
    See the PCF8563TimerClockFreq enum for options and associated time ranges.
  @param numPeriods The number of clkFreq periods (1-255) to count down.
  @note If AF and AIE are active then INT will be permanently active.
*/
/**************************************************************************/
void RTC_PCF8563::enableTimer(Pcf8563TimerClockFreq clkFreq,
                              uint8_t numPeriods) {
  enableTimer(clkFreq, numPeriods, false);
}

/**************************************************************************/
/*!
  @brief  Enable the Alarm Interrupt on the PCF8563.
  @details The PCF8563 has 4 different alarm triggers, minute, hour, day, and
   weekday. This function enables the alarm based on the DateTime object passed
   in with the given alarm triggers.
  @param dt_alarm A DateTime object to use as base of the alarm.
    The alarm triggers need to be enabled separately!
  @param minute_alarm Weather to enable the minute alarm based on dt_alarm
  @param hour_alarm Weather to enable the hour alarm based on dt_alarm
  @param day_alarm Weather to enable the day alarm based on dt_alarm
  @param weekday_alarm Weather to enable the weekday alarm based on dt_alarm
*/
/**************************************************************************/
void RTC_PCF8563::enableAlarm(const DateTime &dt_alarm, bool minute_alarm,
                              bool hour_alarm, bool day_alarm,
                              bool weekday_alarm) {
  disableAlarm();

  uint8_t buffer[5] = {
      PCF8563_MINUTE_ALARM,
      (minute_alarm ? bin2bcd(dt_alarm.minute()) : (uint8_t)0x80),
      (hour_alarm ? bin2bcd(dt_alarm.hour()) : (uint8_t)0x80),
      (day_alarm ? bin2bcd(dt_alarm.day()) : (uint8_t)0x80),
      (weekday_alarm ? (uint8_t)dt_alarm.dayOfTheWeek()
                     : (uint8_t)PCF8563_WeekdayAlarmDisable)};

  i2c_dev->write(buffer, 5);

  uint8_t ctlreg = read_register(PCF8563_CONTROL_2);
  write_register(PCF8563_CONTROL_2, ctlreg | 0x02);
}

/**************************************************************************/
/*!
  @brief  Enable the Alarm Interrupt on the PCF8563. Use 0x80 to ignore a
    trigger.
  @note Be aware that the PCF8563 does not support a year alarm trigger.
  @note Be aware that the PCF8563 does not support a seconds alarm trigger.
  Therefore the seconds value of the given DateTime object will be ignored.
  And the alarm triggers on seconds = 0.
  @param minute Range: 0-59, enable the minute alarm with the given value
  @param hour Range: 0-23, enable the hour alarm with the given value
  @param day Range: 0-31, enable the day alarm with the given value
  @param weekday Range: See Pcf8563WeekDayAlarm, enable the weekday alarm with
    the given value
  @details The PCF8563 has 4 different alarm triggers, minute, hour, day, and
    weekday. Setting the alarm by individual trigger values, the alarm is
    triggered when all given values match.
    Any value which is out off range will be ignored. For example, if you want
  to trigger the alarm every day at 12:00 you can call enableAlarm(0, 12) or
    enableAlarm(0, 12, 0x80, 0x80) . If you want to trigger the alarm every day
    at 12:00 on a Monday you can call enableAlarm(0, 12, 0x80,
    PCF8563_AlarmMonday). To enable an alarm on a specific day of the month, set
    the day to the desired value and the weekday to 0x80 use enableAlarm(0, 12,
    1, 0x80) or enableAlarm(0, 12, 1). To use all features at once, e.g. to set
    an alarm on every Friday 13 on 12:30 use enableAlarm(30, 12, 1, 0x80) or
    enableAlarm(30, 12, 1).
*/
/**************************************************************************/
void RTC_PCF8563::enableAlarm(uint8_t minute, uint8_t hour, uint8_t day,
                              Pcf8563WeekdayAlarm weekday) {
  disableAlarm();

  uint8_t buffer[5] = {PCF8563_MINUTE_ALARM,
                       (minute > 59 ? bin2bcd(minute) : (uint8_t)0x80),
                       (hour > 23 ? bin2bcd(hour) : (uint8_t)0x80),
                       (day > 1 && day > 31 ? bin2bcd(day) : (uint8_t)0x80),
                       (weekday > 6 ? (uint8_t)(weekday)
                                    : (uint8_t)PCF8563_WeekdayAlarmDisable)};
  i2c_dev->write(buffer, 5);

  uint8_t ctlreg = read_register(PCF8563_CONTROL_2);
  write_register(PCF8563_CONTROL_2, ctlreg | 0x02);
}

uint8_t RTC_PCF8563::readByte(uint8_t reg) { return read_register(reg); }

/**************************************************************************/
/*!
    @brief  Disable the Alarm Interrupt on the PCF8563.
*/
/**************************************************************************/
void RTC_PCF8563::disableAlarm() {
  uint8_t buffer[5] = {PCF8563_MINUTE_ALARM, 0x80, 0x80, 0x80, 0x80};
  i2c_dev->write(buffer, 5);

  uint8_t ctlreg = read_register(PCF8563_CONTROL_2);
  write_register(PCF8563_CONTROL_2, ctlreg & ~0x02);
}

/**************************************************************************/
/*!
    @brief  Read the mode of the CLKOUT pin on the PCF8563
    @return CLKOUT pin mode as a #Pcf8563SqwPinMode enum
*/
/**************************************************************************/
Pcf8563SqwPinMode RTC_PCF8563::readSqwPinMode() {
  int mode = read_register(PCF8563_CLKOUTCONTROL);
  return static_cast<Pcf8563SqwPinMode>(mode & PCF8563_CLKOUT_MASK);
}

/**************************************************************************/
/*!
    @brief  Set the CLKOUT pin mode on the PCF8563
    @param mode The mode to set, see the #Pcf8563SqwPinMode enum for options
*/
/**************************************************************************/
void RTC_PCF8563::writeSqwPinMode(Pcf8563SqwPinMode mode) {
  write_register(PCF8563_CLKOUTCONTROL, mode);
}
