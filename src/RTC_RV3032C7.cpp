#include "RTClib.h"
// TODO: only what is needed in basic examples is implemented right now.
//       The RV3032C7 chip supports a lot more functionality...

//#define DEBUG_SERIAL Serial
//#define DEBUG_SERIAL SerialUSB
//#ifdef DEBUG_SERIAL
//     #include <Arduino.h>
//#endif

#define RV3032C7_ADDRESS 0x51   ///< I2C address for RV3032C7
#define RV3032C7_100TH_SEC 0x00 ///< Time - 100th seconds
#define RV3032C7_SECONDS 0x01   ///< time - seconds
#define RV3032C7_ALARM1 0x08    ///< Alarm 1 register - Minutes
#define RV3032C7_ALARM2 0x09    ///< Alarm 2 register - Hours
#define RV3032C7_ALARM3 0x0A    ///< Alarm 2 register - Date
#define RV3032C7_TIMER0 0x0B    ///< Timer Value 0 reg. lower 8 bits
#define RV3032C7_TIMER1 0x0C    ///< Timer Value 1 reg. upper 4 bits 

#define RV3032C7_STATUSREG 0x0D ///< Status register
#define RV3032C7_CONTROL1 0x10  ///< Control register
#define RV3032C7_CONTROL2 0x11  ///< Control register
#define RV3032C7_CONTROL3 0x12  ///< Control register
#define RV3032C7_INT_MASK 0x14  ///< Clock Interrupt Mask Register
#define RV3032C7_TEMPERATUREREG                                                \
  0x0E ///< Temperature register (bit 4-7 = lowest 4 bits.
       ///<  high byte is at RV3032C7_TEMPERATURE8BIT), 12-bit
       ///<  Temperature value
#define RV3032C7_TEMPERATURE8BIT 0x0F ///< 8-bit temperature value

// RAM Addresses that mirror EEPROM registers
#define RV3032C7_PMU 0xC0 ///< Power Management Unit (PMU)

// Status register flags
#define RV3032C7_THF 0x80  ///< Temp. High
#define RV3032C7_TLF 0x40  ///< Temp. Low
#define RV3032C7_UF 0x20   ///< Periodic Time Update
#define RV3032C7_TF 0x10   ///< Periodic Coundown Timer
#define RV3032C7_AF 0x08   ///< Alarm
#define RV3032C7_EVF 0x04  ///< External Event
#define RV3032C7_PORF 0x02 ///< Power On Reset
#define RV3032C7_VLF 0x01  ///< Voltage Low

// Control register flags/fields (some)
#define RV3032C7_XBIT 0x20 ///< Control1, "X" bit (must be set to 1)
#define RV3032C7_TE 0x08 ///< Control1, Periodic Countdown Timer Enable bit
#define RV3032C7_TD 0x03 ///< Control1, Timer Clock Frequency selection
#define RV3032C7_EERD 0x04 ///< Control1, ROM Memory Refresh Disable bit.
#define RV3032C7_STOP 0x01 ///< Control2, STOP bit
#define RV3032C7_EIE 0x04  ///< Control2, External Event Interrupt Enable bit
#define RV3032C7_AIE 0x08  ///< Control2, Alarm Interrupt Enable bit
#define RV3032C7_TIE                                                           \
  0x10 ///< Control2, Periodic Countdown Timer Interrupt Enable bit
#define RV3032C7_UIE                                                           \
  0x20 ///< Control2, Periodic Time Update Interrupt Enable bit
#define RV3032C7_CLKIE                                                         \
  0x40 ///< Control2, Interrupt Controlled Clock Output Enable bit

// Clock Interrupt Mask register flags (only those used in this file)
#define RV3032C7_CAIE 0x10 ///< Clock output when Alarm Interrupt Enable bit
#define RV3032C7_CTIE 0x08 ///< Clock output when Periodic Countdown Timer Interrupt Enable bit

// Power Management Unit (PMU) register flags (only those used in this file)
#define RV3032C7_NCLKE                                                         \
  0x40 ///< Not CLKOUT Enable Bit in Power Management Unit (PMU)
#define RV3032C7_BSM                                                           \
  0x30 ///< Backup Switchover Mode

// Temperature register flags (some flags ended up here, albeit unrelated to
// temperature)
#define RV3032C7_CLKF 0x02 ///< Clock Output Interrupt Flag (CLKF)

/**************************************************************************/
/*!
    @brief  Start I2C for the RV3032C7, test succesful connection
            and initialize the chip for use with RTClib
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find RV3032C7 or false otherwise.
*/
/**************************************************************************/
bool RTC_RV3032C7::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(RV3032C7_ADDRESS, wireInstance);
  i2c_dev->begin(false); // no detection as the I2C i/f might become
                         // unresponsive when switched from VDD to Vbackup
  short retries = 0;
  while (
      !i2c_dev->detected()) { // retry 3 times to make sure we reinitialize an
                              // unresponsive chip (see RV3032C7 app. manual)
    if (++retries >= 3) {
      return false;
    }
  }

  // Next we turn off automatic refresh from EEPROM to behave like other chips
  // in RTClib. Future updates may add more explit control over EEPROM refreshes
  uint8_t ctrl1 = read_register(RV3032C7_CONTROL1);
  if ((ctrl1 & RV3032C7_EERD) == 0) {
    write_register(RV3032C7_CONTROL1, ctrl1 | RV3032C7_XBIT | RV3032C7_EERD);
  }
  // Finally, we check if Backup Switchover Mode (BSM) is 00b in the PMU
  // register (default for a new chip). If it is, we set it to Level Switching
  // Mode (LSM) as most users of this library expect backup power to work
  uint8_t pmu = read_register(RV3032C7_PMU);
  if ((pmu & RV3032C7_BSM) == 0) { // Backup Switchover is disabled
    write_register(RV3032C7_PMU,
                   pmu | 0x20); // Enable Level Switching Mode (LSM)
  }
  return true;
}

/**************************************************************************/
/*!
    @brief  Check the status register PORF flag to see if the RV3032C7
   stopped due to power loss. After Power On, this function will
   continue to return true until the time is set via adjust()
    @return True if the oscillator stopped or false if it is
   running without interruption since last time the time was set by adjust().
*/
/**************************************************************************/
bool RTC_RV3032C7::lostPower(void) {
  // TODO: add an option to check for loss of precision (VLF flag)
  return (read_register(RV3032C7_STATUSREG) & RV3032C7_PORF) != 0 ? true
                                                                  : false;
}

/**************************************************************************/
/*!
    @brief  Set the date and time. After this function returns, lostPower() will
   return false until the next power loss
    @param dt DateTime object containing the date/time to set
*/
/**************************************************************************/
void RTC_RV3032C7::adjust(const DateTime &dt) {
  uint8_t buffer[8] = {RV3032C7_SECONDS,
                       bin2bcd(dt.second()),
                       bin2bcd(dt.minute()),
                       bin2bcd(dt.hour()),
                       bin2bcd(dowToRV3032C7(dt.dayOfTheWeek())),
                       bin2bcd(dt.day()),
                       bin2bcd(dt.month()),
                       bin2bcd(dt.year() - 2000U)};
  i2c_dev->write(buffer, 8);
  write_register(RV3032C7_STATUSREG, ~RV3032C7_PORF); // clear PORF flag
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object with the current date/time
*/
/**************************************************************************/
DateTime RTC_RV3032C7::now() {
  uint8_t buffer[7];
  buffer[0] = RV3032C7_SECONDS;
  i2c_dev->write_then_read(buffer, 1, buffer, 7);

  return DateTime(bcd2bin(buffer[6]) + 2000U, bcd2bin(buffer[5]),
                  bcd2bin(buffer[4]), bcd2bin(buffer[2]), bcd2bin(buffer[1]),
                  bcd2bin(buffer[0])); // Note: RV3032C7 unused bits read = 0
}

/**************************************************************************/
/*!
    @brief  Get the current temperature from the RV3032C7's temperature sensor.
    The resoluton is 12 bits (corresponding to 0.0625 C)
    @return Current temperature (float)
*/
/**************************************************************************/
float RTC_RV3032C7::getTemperature() {
  uint8_t buffer1[2];
  uint8_t buffer2[2];
  do { // no blocking, so read twice as suggested in the app. manual
    buffer1[0] = buffer2[0] = RV3032C7_TEMPERATUREREG;
    i2c_dev->write_then_read(buffer1, 1, buffer1, 2);
    i2c_dev->write_then_read(buffer2, 1, buffer2, 2);
  } while ((buffer1[0] != buffer2[0]) || (buffer1[1] != buffer2[1]));
  return float(int((int8_t)buffer1[1]) * 16 + (buffer1[0] >> 4)) * 0.0625f;
}

/**************************************************************************/
/*!
    @brief  Set alarm for RV3032C7.
        - If event_type is RV3032C7_EV_Poll the alarm status can be polled with
   alarmFired()
        - If event_type is RV3032C7_EV_Int, in addition the INT PIN goes low
   (usually this is used to generate an interrupt)
        - If event_type is RV3032C7_EV_IntClock, in addition to the INT PIN
   going low, the clock is output on the CLKOUT pin while the INT pin is low
        (even if it was turned off via disableClkOut()). The clock will be
   output until the INT pin is cleared by clearAlarm() or disabled with
   disableAlarm().
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see RV3032C7AlarmMode enum
        @param   event_type Desired event type, see RV3032C7EventTyp enum
    @return False if alarm registers are not set, otherwise true
*/
/**************************************************************************/
bool RTC_RV3032C7::setAlarm(const DateTime &dt, RV3032C7AlarmMode alarm_mode,
                            RV3032C7EventType event_type) {
  uint8_t A1M1 = (alarm_mode & 0x01) << 7; // Minutes bit 7.
  uint8_t A1M2 = (alarm_mode & 0x02) << 6; // Hour bit 7.
  uint8_t A1M3 = (alarm_mode & 0x04) << 5; // Day/Date bit 7.
  uint8_t buffer[4] = {RV3032C7_ALARM1, uint8_t(bin2bcd(dt.minute()) | A1M1),
                       uint8_t(bin2bcd(dt.hour()) | A1M2),
                       uint8_t(bin2bcd(dt.day()) | A1M3)};

  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  write_register(RV3032C7_CONTROL2,
                 ctrl2 & (~RV3032C7_AIE));          // Avoid spurious interrupts
  write_register(RV3032C7_STATUSREG, ~RV3032C7_AF); // clear Alarm flag
  i2c_dev->write(buffer, 4);
  if (event_type & 0x01) { // Enable Interrupt at alarm match
    ctrl2 |= RV3032C7_AIE;
    write_register(RV3032C7_CONTROL2, ctrl2); // Set AIE
  }
  if (event_type & 0x02) {   // Enable Clock Output at alarm match and select
                             // alarm as interrupt source
    ctrl2 |= RV3032C7_CLKIE; // Set CLKIE
    write_register(RV3032C7_CONTROL2, ctrl2); // write ctrl2 to register
    write_register(RV3032C7_INT_MASK, intmask | RV3032C7_CAIE); // Set CAIE
  } else { // Disable Clock Output at alarm match to be sure
    intmask &= ~RV3032C7_CAIE;
    write_register(RV3032C7_INT_MASK, intmask); // Clear CAIE
    if ( (intmask & 0x1F) == 0x00) {  // No user left in clock output mask register
        ctrl2 &= (~RV3032C7_CLKIE);               // clear CLKIE
        write_register(RV3032C7_CONTROL2, ctrl2); // write ctrl2 to register
    }
  }
  return true; // No check needed for now, may be added in the future
}

/**************************************************************************/
/*!
    @brief  Get the date/time value of the Alarm
    @return DateTime object with the Alarm data set in the
            day, hour, minutes, and seconds fields

    At power on and after disableAlarm() returns RV3032C7InvalidDate
*/
/**************************************************************************/
DateTime RTC_RV3032C7::getAlarm() {
  uint8_t buffer[3] = {RV3032C7_ALARM1, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 3);

  uint8_t minutes = bcd2bin(buffer[0] & 0x7F);
  uint8_t hour = bcd2bin(buffer[1] & 0x3F); // Only 24 hour format supported by RV3032C7 (same as this library)
  uint8_t day = bcd2bin(buffer[2] & 0x3F);

  // Chosen in order to match the year and month returned by RTC_DS3231::getAlarm();
  return DateTime(2000, 5, day, hour, minutes);
}

/**************************************************************************/
/*!
    @brief  Get the mode for the Alarm
    @return RV3032C7AlarmMode enum value for the current Alarm mode
*/
/**************************************************************************/
RV3032C7AlarmMode RTC_RV3032C7::getAlarmMode() {
  uint8_t buffer[3] = {RV3032C7_ALARM1, 0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 3);
  uint8_t alarm_mode = (buffer[0] & 0x80) >> 7    // A1M1 - Minutes bit
                       | (buffer[1] & 0x80) >> 6  // A1M2 - Hour bit
                       | (buffer[2] & 0x80) >> 5; // A1M3 - Date bit
  return (RV3032C7AlarmMode)alarm_mode; // No need to check because all possible values are valid
}

/**************************************************************************/
/*!
    @brief  Get the event type for the Alarm
    @return RV3032C7EventType enum value for the current Alarm event type
*/
/**************************************************************************/
RV3032C7EventType RTC_RV3032C7::getAlarmEventType() {
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  uint8_t event_type= (ctrl2 & RV3032C7_AIE) >> 3;
  if ( ((intmask & RV3032C7_CAIE) != 0) &&  ((ctrl2 & RV3032C7_CLKIE) != 0) ) {
      event_type |= 0x02; 
  }
  switch(event_type) {
      case RV3032C7_EV_Poll:   
      case RV3032C7_EV_Int:
      case RV3032C7_EV_IntClock:
          return (RV3032C7EventType) event_type;
      default:
          return RV3032C7_EV_Poll;
  }
}

/**************************************************************************/
/*!
    @brief  Disable alarm
    @details this function disables the alarm and in addition clears it (same as
   clearAlarm())
*/
/**************************************************************************/
void RTC_RV3032C7::disableAlarm(void) {
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  // reset to power on default, preventing any further match 
  uint8_t buffer[4] = {RV3032C7_ALARM1, 0x00, 0x00, 0x00};
  i2c_dev->write(buffer, 4);
  
  intmask &= ~RV3032C7_CAIE; // Clear CAIE 
  write_register(RV3032C7_INT_MASK, intmask); // write register
  if ( (intmask & 0x1F) == 0x00) {  // No user left in clock output mask register
     ctrl2 &= (~RV3032C7_CLKIE);             // clear CLKIE
  }
  ctrl2 &= ~RV3032C7_AIE; // clear Alarm Interrupt Enable (AIE)
  write_register(RV3032C7_CONTROL2, ctrl2);  // write register

  clearAlarm();
}

/**************************************************************************/
/*!
    @brief  Clear status of alarm so that alarmFired() will return false
    This also cause the INT PIN to go high (not active). If CLKOUT was activated
   by the alarm, it will stop outputing the clock.
*/
/**************************************************************************/
void RTC_RV3032C7::clearAlarm(void) {
  write_register(RV3032C7_STATUSREG, ~RV3032C7_AF); // clear Alarm flag
  // In addition we clear the CLKF flag since it can be set as well if
  // RV3032C7_EV_IntClock
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  if (intmask & RV3032C7_CAIE) {
     uint8_t treg = read_register(RV3032C7_TEMPERATUREREG); // CLKF happens to be in the temperature register
     write_register(RV3032C7_TEMPERATUREREG, treg & (~RV3032C7_CLKF));
  }
}

/**************************************************************************/
/*!
    @brief  Get status of alarm
        @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_RV3032C7::alarmFired(void) {
  return (read_register(RV3032C7_STATUSREG) & RV3032C7_AF) != 0 ? true : false;
}

/**************************************************************************/
/*!
    @brief  Enable Periodic Countdown Timer on the RV3032C7.
        - If event_type is RV3032C7_EV_Poll the alarm status can be polled with
   CountdownTimerFired()
        - If event_type is RV3032C7_EV_Int, in addition the INT PIN goes low
   (usually this is used to generate an interrupt)
        - If event_type is RV3032C7_EV_IntClock, in addition to the INT PIN
   going low, the clock is output on the CLKOUT pin while the INT pin is low
        (even if it was turned off via disableClkOut()). The clock will be
   output until the INT pin is cleared by clearCountdownTimer() or disabled with
   disableCountdownTimer().
        @param clkFreq One of the RV3032C7's Periodic Countdown Timer Clock Frequencies.
         See the #RV3032C7TimerClockFreq enum for options and associated time ranges.
        @param numPeriods The number of clkFreq periods (1-4095) to count down.
        @param   event_type Desired event type, see #RV3032C7EventTyp enum
*/
/**************************************************************************/
void RTC_RV3032C7::enableCountdownTimer(RV3032C7TimerClockFreq clkFreq, uint8_t numPeriods, RV3032C7EventType event_type) {
  uint8_t buffer[3] = { RV3032C7_TIMER0, lowByte(numPeriods), highByte(numPeriods) };

  uint8_t ctrl1 = read_register(RV3032C7_CONTROL1);
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);  

  ctrl1 &= ~RV3032C7_TE;  // clear TE bit
  write_register(RV3032C7_CONTROL2, ctrl1); // Disable Countdown Timer
  ctrl2 &= ~RV3032C7_TIE;  // clear TIE bit
  write_register(RV3032C7_CONTROL2, ctrl2); // Disable Timer Interrupt            
  write_register(RV3032C7_STATUSREG, ~RV3032C7_TF); // clear Timer flag

  ctrl1 = (ctrl1 & (~RV3032C7_TD)) | (clkFreq & RV3032C7_TD);
  write_register(RV3032C7_CONTROL2, ctrl1); // Set TD field
  i2c_dev->write(buffer, 3); // Write Timer Value (12 bits)
  
  if (event_type & 0x01) { // Enable Interrupt at alarm match
    ctrl2 |= RV3032C7_TIE;  //set Timer Interrupt Enable (TIE)
    write_register(RV3032C7_CONTROL2, ctrl2); //enable Timer interrupt 
  }
  if (event_type & 0x02) {   // Enable Clock Output at timer interrupt
    ctrl2 |= RV3032C7_CLKIE; // Set CLKIE
    write_register(RV3032C7_CONTROL2, ctrl2); // write ctrl2 to register
    write_register(RV3032C7_INT_MASK, intmask | RV3032C7_CTIE); // Set CTIE
  } else { // Disable Clock Output at alarm match to be sure
    intmask &= ~RV3032C7_CTIE;
    write_register(RV3032C7_INT_MASK, intmask & (~RV3032C7_CTIE)); // Clear CTIE
    if ( (intmask & 0x1F) == 0x00) {  // No user left in clock output mask register
        ctrl2 &= (~RV3032C7_CLKIE);               // clear CLKIE
        write_register(RV3032C7_CONTROL2, ctrl2); // write ctrl2 to register
    }
  }
  write_register(RV3032C7_CONTROL2, ctrl1 | RV3032C7_TE); // Enable Countdown Timer
}

/**************************************************************************/
/*!
    @brief  Get the value of the Periodic Countdown Timer
    @return the number of clkFreq periods to count down (uint16_t). Range: 0-4095 

    The preset value of the countdown timer is returned and not the actual value (which is not possible to read).
    At power on returns 0, otherwise it will return the last value set (valid values 1-4095)
*/
/**************************************************************************/
uint16_t RTC_RV3032C7::getCountdownTimer() {
  uint8_t buffer[3] = { RV3032C7_TIMER0, 0};
  i2c_dev->write_then_read(buffer, 1, buffer, 2);
  uint16_t numPeriods = ((uint16_t)buffer[1])<<8 | buffer[0];
  return numPeriods;
}

/**************************************************************************/
/*!
    @brief  Get the mode for the Periodic Countdown Timer
    @return RV3032C7's Periodic Countdown Timer Clock Frequency. See the #RV3032C7TimerClockFreq enum for options and associated time ranges.
*/
/**************************************************************************/
RV3032C7TimerClockFreq RTC_RV3032C7::getCountdownTimerClock() {
  uint8_t ctrl1 = read_register(RV3032C7_CONTROL1);
  return (RV3032C7TimerClockFreq) (ctrl1 & RV3032C7_TD);

}

/**************************************************************************/
/*!
    @brief  Get the event type for the Periodic Countdown Timer
    @return RV3032C7EventType enum value for the current Periodic Countdown Timer event type
*/
/**************************************************************************/
RV3032C7EventType RTC_RV3032C7::getCountdownTimerEventType() {
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  uint8_t event_type= (ctrl2 & RV3032C7_TIE) >> 4;
  if ( ((intmask & RV3032C7_CTIE) != 0) &&  ((ctrl2 & RV3032C7_CLKIE) != 0) ) {
      event_type |= 0x02; 
  }
  switch(event_type) {
      case RV3032C7_EV_Poll:   
      case RV3032C7_EV_Int:
      case RV3032C7_EV_IntClock:
          return (RV3032C7EventType) event_type;
      default:
          return RV3032C7_EV_Poll;
  }
}

/**************************************************************************/
/*!
    @brief  Disable Periodic Countdown Timer
    @details this function disables the Periodic Countdown Timer and in addition clears it (same as
   clearCountdownTimer()
*/
/**************************************************************************/
void RTC_RV3032C7::disableCountdownTimer(void) {
  uint8_t ctrl1 = read_register(RV3032C7_CONTROL1);
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  
  // disable Periodic Countdown Timer 
  ctrl1 &= ~RV3032C7_TE; // clear TE bit
  write_register(RV3032C7_CONTROL1, ctrl1);  // write register

  // disable Clock output when Periodic Countdown Timer Interrupt
  intmask &= ~RV3032C7_CTIE; // Clear CTIE 
  write_register(RV3032C7_INT_MASK, intmask); // write register
  if ( (intmask & 0x1F) == 0x00) {  // No user left in clock output mask register
     ctrl2 &= (~RV3032C7_CLKIE);             // clear CLKIE
  }
  
  // clear Periodic Countdown Timer Interrupt Enable bit (TIE)
  ctrl2 &= ~RV3032C7_TIE; // clear TIE bit
  write_register(RV3032C7_CONTROL2, ctrl2);  // write register

  clearCountdownTimer();
  
}

/**************************************************************************/
/*!
    @brief  Clear status of Periodic Countdown Timer so that CountdownTimerFired() will return false
    This also cause the INT PIN to go high (not active). If CLKOUT was activated by the timer, it will stop outputing the clock.
*/
/**************************************************************************/
void RTC_RV3032C7::clearCountdownTimer(void) {
  write_register(RV3032C7_STATUSREG, ~RV3032C7_TF); // clear Timer flag
  // In addition we clear the CLKF flag since it can be set as well if
  // RV3032C7_EV_IntClock
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  if (intmask & RV3032C7_CTIE) {
     uint8_t treg = read_register(RV3032C7_TEMPERATUREREG); // CLKF happens to be in the temperature register
     write_register(RV3032C7_TEMPERATUREREG, treg & (~RV3032C7_CLKF));
  }
}

/**************************************************************************/
/*!
    @brief  Get status of the Periodic Countdown Timer
        @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_RV3032C7::CountdownTimerFired(void) {
  return (read_register(RV3032C7_STATUSREG) & RV3032C7_TF) != 0 ? true : false;  
}

/**************************************************************************/
/*!
    @brief  Enable normal clock output on CLKOUT pin (default 32.768 kHz)
    @details The CLKOUT output is enabled by default at power on. It is a
   push-pull output, no pull-up resistor required.
    TODO: Add option to set specific frequency
*/
/**************************************************************************/
void RTC_RV3032C7::enableClkOut(void) {
  uint8_t pmureg = read_register(RV3032C7_PMU);
  pmureg &= (~RV3032C7_NCLKE);
  write_register(RV3032C7_PMU, pmureg);
}

/**************************************************************************/
/*!
    @brief  Disable normal clock output on CLKOUT pin
    @details Disable normal clock output on CLKOUT pin.
    When the clock is disabled, it can still be enabled via setAlarm
    with event_type set to RV3032C7_EV_IntClock:
    when the Alarm triggers CLKOUT is enabled.
    It will remain enabled until the alarm is cleared by clearAlarm() or
   disabled with disableAlarm().
*/
/**************************************************************************/
void RTC_RV3032C7::disableClkOut(void) {
  uint8_t pmureg = read_register(RV3032C7_PMU);
  pmureg |= RV3032C7_NCLKE;
  write_register(RV3032C7_PMU, pmureg);
}

/**************************************************************************/
/*!
    @brief  Get status of clock output on CLKOUT pin
    @details only checks if the CLKOUT pin is enabled/disabled via
   enableClkOut() and disableClkOut(). When the clock is disabled, it can still
   be enabled via setAlarm with event_type set to RV3032C7_EV_IntClock, this is
   intentionally not checked by isEnabledClkOut();
    @return True if enabled otherwise false
*/
/**************************************************************************/
bool RTC_RV3032C7::isEnabledClkOut(void) {
  return (read_register(RV3032C7_PMU) & RV3032C7_NCLKE) == 0 ? true : false;
}
