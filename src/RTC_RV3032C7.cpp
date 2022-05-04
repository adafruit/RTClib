#include "RTClib.h"
// TODO: only what is needed in basic examples is implemented right now. 
//       The RV3032C7 chip supports a lot more functionality...

//#define DEBUG_SERIAL Serial
//#define DEBUG_SERIAL SerialUSB
//#ifdef DEBUG_SERIAL
//     #include <Arduino.h>
//#endif  

#define RV3032C7_ADDRESS 0x51    ///< I2C address for RV3032C7
#define RV3032C7_100TH_SEC 0x00  ///< Time - 100th seconds
#define RV3032C7_SECONDS 0x01    ///< time - seconds
#define RV3032C7_ALARM1 0x08     ///< Alarm 1 register - Minutes
#define RV3032C7_ALARM2 0x09     ///< Alarm 2 register - Hours
#define RV3032C7_ALARM3 0x0A     ///< Alarm 2 register - Date
#define RV3032C7_STATUSREG 0x0D  ///< Status register
#define RV3032C7_CONTROL1 0x10   ///< Control register
#define RV3032C7_CONTROL2 0x11   ///< Control register
#define RV3032C7_CONTROL3 0x12   ///< Control register
#define RV3032C7_INT_MASK 0x14   ///< Clock Interrupt Mask Register
#define RV3032C7_TEMPERATUREREG                            \
  0x0E ///< Temperature register (bit 4-7 = lowest 4 bits. 
       ///<  high byte is at RV3032C7_TEMPERATURE8BIT), 12-bit
       ///<  Temperature value
#define RV3032C7_TEMPERATURE8BIT 0x0F  ///< 8-bit temperature value

// RAM Addresses that mirror EEPROM registers
#define RV3032C7_PMU 0xC0  ///< Power Management Unit (PMU)

// Status register flags
#define RV3032C7_THF 0x80   ///< Temp. High 
#define RV3032C7_TLF 0x40   ///< Temp. Low 
#define RV3032C7_UF 0x20    ///< Periodic Time Update
#define RV3032C7_TF 0x10    ///< Periodic Coundown Timer
#define RV3032C7_AF 0x08    ///< Alarm
#define RV3032C7_EVF 0x04   ///< External Event
#define RV3032C7_PORF 0x02  ///< Power On Reset
#define RV3032C7_VLF 0x01   ///< Voltage Low

// Control register flags (some)
#define RV3032C7_XBIT 0x20   ///< Control1, "X" bit (must be set to 1)
#define RV3032C7_EERD 0x04   ///< Control1, ROM Memory Refresh Disable bit.
#define RV3032C7_STOP 0x01   ///< Control2, STOP bit 
#define RV3032C7_EIE 0x04    ///< Control2, External Event Interrupt Enable bit 
#define RV3032C7_AIE 0x08    ///< Control2, Alarm Interrupt Enable bit 
#define RV3032C7_TIE 0x10    ///< Control2, Periodic Countdown Timer Interrupt Enable bit 
#define RV3032C7_UIE 0x20    ///< Control2, Periodic Time Update Interrupt Enable bit 
#define RV3032C7_CLKIE 0x40  ///< Control2, Interrupt Controlled Clock Output Enable bit 
  
//Clock Interrupt Mask register flags (only those used in this file)
#define RV3032C7_CAIE 0x10  ///<Clock output when Alarm Interrupt Enable bit

//Clock Interrupt Mask register flags (only those used in this file)
#define RV3032C7_NCLKE 0x40 ///< Not CLKOUT Enable Bit in Power Management Unit (PMU)

//Temperature register flags (some flags ended up here, albeit unrelated to temperature)
#define RV3032C7_CLKF 0x02 ///< Clock Output Interrupt Flag (CLKF)

/**************************************************************************/
/*!
    @brief  Start I2C for the RV3032C7 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find RV3032C7 or false otherwise.
*/
/**************************************************************************/
boolean RTC_RV3032C7::begin(TwoWire *wireInstance) {
  if (i2c_dev)
    delete i2c_dev;
  i2c_dev = new Adafruit_I2CDevice(RV3032C7_ADDRESS, wireInstance);
  if (!i2c_dev->begin())
    return false;
    
  // Next we turn off automatic refresh from EEPROM to behave like other chips in RTClib.
  // Future updates may add more explit control over EEPROM refreshes
  uint8_t ctrl1 = read_register(RV3032C7_CONTROL1);
  if ( (ctrl1 & RV3032C7_EERD) == 0 ) {
      write_register(RV3032C7_CONTROL1, ctrl1 | RV3032C7_XBIT | RV3032C7_EERD);  
  }
  return true;
}

/**************************************************************************/
/*!
    @brief  Check the status register PORF flag to see if the RV3032C7
   stopped due to power loss
    @return True if the bit is set (oscillator stopped) or false if it is
   running. TODO: add an option to check for loss of precision (VLF flag)
*/
/**************************************************************************/
bool RTC_RV3032C7::lostPower(void) {
  return (read_register(RV3032C7_STATUSREG) & RV3032C7_PORF) != 0 ? true : false;
}

/**************************************************************************/
/*!
    @brief  Set the date and make sure the Oscillator Stop bit is cleared
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
    @brief  Get the current temperature from the RV3032C7's temperature sensor
    @return Current temperature (float)
*/
/**************************************************************************/
float RTC_RV3032C7::getTemperature() {
  uint8_t buffer1[2]; 
  uint8_t buffer2[2];
  do {  // no blocking, so read twice as suggested in the app. manual
      buffer1[0] = buffer2[0] = RV3032C7_TEMPERATUREREG;
      i2c_dev->write_then_read(buffer1, 1, buffer1, 2);  
      i2c_dev->write_then_read(buffer2, 1, buffer2, 2);
  } while ( (buffer1[0] != buffer2[0]) || (buffer1[1] != buffer2[1]) );   
  return float( int((int8_t) buffer1[1])*16 + (buffer1[0]>>4) )  * 0.0625f;
}

/**************************************************************************/
/*!
    @brief  Set alarm 1 for RV3032C7 
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see Ds3231Alarm1Mode enum
        @param   event_type Desired event type, see RV3032C7EventTyp enum
    @return False if control register is not set, otherwise true
*/
/**************************************************************************/
bool RTC_RV3032C7::setAlarm(const DateTime &dt, RV3032C7AlarmMode alarm_mode, RV3032C7EventType event_type) { 
  uint8_t A1M1 = (alarm_mode & 0x01) << 7; // Minutes bit 7.
  uint8_t A1M2 = (alarm_mode & 0x02) << 6; // Hour bit 7.
  uint8_t A1M3 = (alarm_mode & 0x04) << 5; // Day/Date bit 7.
  uint8_t buffer[4] = {RV3032C7_ALARM1, uint8_t(bin2bcd(dt.minute()) | A1M1),
                       uint8_t(bin2bcd(dt.hour()) | A1M2),
                       uint8_t(bin2bcd(dt.day()) | A1M3 )};

  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  write_register(RV3032C7_CONTROL2, ctrl2 & (~RV3032C7_AIE) ); // Avoid spurious interrupts
  write_register(RV3032C7_STATUSREG, ~RV3032C7_AF ); // clear Alarm flag
  i2c_dev->write(buffer, 4);
  if (event_type & 0x01) { // Enable Interrupt at alarm match
      ctrl2 |= RV3032C7_AIE;
      write_register(RV3032C7_CONTROL2, ctrl2 ); // Set AIE
  } // else it is already cleared
  if (event_type & 0x02) { // Enable Clock Output at alarm match and select alarm as interrupt source
      ctrl2 |= RV3032C7_CLKIE;                   // Set CLKIE
      write_register(RV3032C7_CONTROL2, ctrl2 ); // write ctrl2 to register
      write_register(RV3032C7_INT_MASK, intmask | RV3032C7_CAIE ); // Set CAIE
  } else { // Disable Clock Output at alarm match to be sure
      ctrl2 &= (~RV3032C7_CLKIE);                // clear CLKIE
      write_register(RV3032C7_CONTROL2, ctrl2 ); // write ctrl2 to register
      write_register(RV3032C7_INT_MASK, intmask & (~RV3032C7_CAIE) ); // Clear CAIE    
  }
  return true;  // No check needed for now, may be added in the future   
}

/**************************************************************************/
/*!
    @brief  Disable alarm
    @details this function disables the alarm and then clears it (same as clearAlarm())
 /
/**************************************************************************/
void RTC_RV3032C7::disableAlarm(void) {
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  write_register(RV3032C7_INT_MASK, intmask & (~RV3032C7_CAIE) ); // Clear CAIE
  // TODO: if we ever implement other functions that can set CLKIE then 
  // check intmask to see if we are the last user left before clearing the following flags 
  write_register(RV3032C7_CONTROL2, ctrl2 & (~(RV3032C7_AIE | ~RV3032C7_CLKIE)) ); // Disable Alarm Interrupts
  clearAlarm();
}

/**************************************************************************/
/*!
    @brief  Clear status of alarm
    @details this function clear the Alarm. This cause the INT PIN to go high (not active) and
    (in case the alarm event was set to RV3032C7_EV_IntClock) the CLKOUT pin will 
*/
/**************************************************************************/
void RTC_RV3032C7::clearAlarm(void) {
  write_register(RV3032C7_STATUSREG, ~RV3032C7_AF ); // clear Alarm flag
  // In addition we clear the CLKF flag since it can be set as well if RV3032C7_EV_IntClock
  // TODO: if we ever implement other functions that can set CLKF then add option to clear only AF/CLKF 
  uint8_t treg = read_register(RV3032C7_TEMPERATUREREG); // CLKF happens to be in the temperature register
  write_register(RV3032C7_TEMPERATUREREG, treg & (~RV3032C7_CLKF) );
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
    @brief  Enable normal clock output on CLKOUT pin (default 32.768 kHz)
    @details The CLKOUT output is enabled by default. It is a push-pull output
    no pull-up resistor required. 
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
    It will remain enabled until the alarm is cleared by clearAlarm().  
    So this function may not actually stop the clock if there is an alarm active at the time it is called.
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
    @details only checks if the CLKOUT pin is enabled all the time. 
    The pin could also be enabled by an alarm, this is not checked
    @return True if enabled otherwise false
*/
/**************************************************************************/
bool RTC_RV3032C7::isEnabledClkOut(void) {
  return  (read_register(RV3032C7_PMU) & RV3032C7_NCLKE) == 0 ? true : false;
}
