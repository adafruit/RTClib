#include "RTClib.h"
// TODO: only what is needed in basic example implemented right now. 

//#define DEBUG_SERIAL Serial
#define DEBUG_SERIAL SerialUSB
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
       ///  high byte is at 0x0F), 12-bit
       ///< temperature value
#define RV3032C7_TEMPERATURE8BIT 0x0F  ///< 8-bit temperature

// Status register flags
#define RV3032C7_THF 0x80   ///< Temp. High 
#define RV3032C7_TLF 0x40   ///< Temp. Low 
#define RV3032C7_UF 0x20    ///< Periodic Time Update
#define RV3032C7_TF 0x10    ///< Periodic Coundown Timer
#define RV3032C7_AF 0x08    ///< Alarm
#define RV3032C7_EVF 0x04   ///< External Event
#define RV3032C7_PORF 0x02  ///< Power On Reset
#define RV3032C7_VLF 0x01   ///< Voltage Low

// Control register flags
#define RV3032C7_STOP 0x01   ///< Control2, STOP bit 
#define RV3032C7_EIE 0x04    ///< Control2, External Event Interrupt Enable bit 
#define RV3032C7_AIE 0x08    ///< Control2, Alarm Interrupt Enable bit 
#define RV3032C7_TIE 0x10    ///< Control2, Periodic Countdown Timer Interrupt Enable bit 
#define RV3032C7_UIE 0x20    ///< Control2, Periodic Time Update Interrupt Enable bit 
#define RV3032C7_CLKIE 0x40  ///< Control2, Interrupt Controlled Clock Output Enable bit 
  
//Clock Interrupt Mask register flags (only those used in this file)
#define RV3032C7_CAIE 0x10  ///<Clock output when Alarm Interrupt Enable bit


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

  // Clear the Power On Reset Flag (PORF)
   uint8_t stat = read_register(RV3032C7_STATUSREG);  // TODO: remove read_register, not needed after initial debug period
   #ifdef DEBUG_SERIAL
       DEBUG_SERIAL.print(F("RTCLib RV3032C7_STATUSREG=")); DEBUG_SERIAL.println(stat, BIN);
   #endif  
   write_register(RV3032C7_STATUSREG, ~RV3032C7_PORF);
   stat = read_register(RV3032C7_STATUSREG);  
   #ifdef DEBUG_SERIAL
       DEBUG_SERIAL.print(F("STATUS after clearing PORF=")); DEBUG_SERIAL.println(stat, BIN);
   #endif  

  /*
  // Check STOP bit, clear if set
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);

  #ifdef DEBUG_SERIAL
      DEBUG_SERIAL.print(F("RTCLib RV3032C7_CONTROL2=")); DEBUG_SERIAL.println(ctrl2, BIN);
  #endif  
  if ( (ctrl2 & RV3032C7_STOP) >0) {  // Oscillator is stopped
       ctrl2 &= ~RV3032C7_STOP;       // clear STOP bit
       #ifdef DEBUG_SERIAL
           DEBUG_SERIAL.print(F("RTCLib RV3032C7_CONTROL2=")); DEBUG_SERIAL.println(ctrl2, BIN);
       #endif  
       //write_register(RV3032C7_CONTROL2, ctrl2);
  }
  */
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
    @brief  Read the SQW pin mode
    @return Pin mode, see Ds3231SqwPinMode enum
*/
/**************************************************************************/
Ds3231SqwPinMode RTC_RV3032C7::readSqwPinMode() {
  int mode;
  mode = read_register(RV3032C7_CONTROL1) & 0x1C;
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
void RTC_RV3032C7::writeSqwPinMode(Ds3231SqwPinMode mode) {
  uint8_t ctrl = read_register(RV3032C7_CONTROL1);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  //write_register(RV3032C7_CONTROL1, ctrl | mode);
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
      write_register(RV3032C7_CONTROL2, ctrl2 | RV3032C7_AIE ); // Set AIE
  } // else it is already cleared
  if (event_type & 0x02) { // Enable Clock Output at alarm match
      write_register(RV3032C7_INT_MASK, intmask | RV3032C7_CAIE ); // Set CAIE
  } else { // Disable Clock Output at alarm match
      write_register(RV3032C7_INT_MASK, intmask & (~RV3032C7_CAIE) ); // Clear CAIE    
  }
  return true;  // No check needed for now, may be added in the future   
}

/**************************************************************************/
/*!
    @brief  Disable alarm
        @param 	alarm_num Alarm number to disable
*/
/**************************************************************************/
void RTC_RV3032C7::disableAlarm(void) {
  uint8_t ctrl2 = read_register(RV3032C7_CONTROL2);
  uint8_t intmask = read_register(RV3032C7_INT_MASK);
  write_register(RV3032C7_CONTROL2, ctrl2 & (~RV3032C7_AIE) ); // Disable Alarm Interrupt
  write_register(RV3032C7_STATUSREG, ~RV3032C7_AF ); // clear Alarm flag
  write_register(RV3032C7_INT_MASK, intmask & (~RV3032C7_CAIE) ); // Clear CAIE
}

/**************************************************************************/
/*!
    @brief  Clear status of alarm
        @param 	alarm_num Alarm number to clear
*/
/**************************************************************************/
void RTC_RV3032C7::clearAlarm(void) {
  write_register(RV3032C7_STATUSREG, ~RV3032C7_AF ); // clear Alarm flag
}

/**************************************************************************/
/*!
    @brief  Get status of alarm
        @param 	alarm_num Alarm number to check status of
        @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_RV3032C7::alarmFired(void) {
  return ((read_register(RV3032C7_STATUSREG) & RV3032C7_AF) != 0 );
}

/**************************************************************************/
/*!
    @brief  Enable 32KHz Output
    @details The 32kHz output is enabled by default. It requires an external
    pull-up resistor to function correctly
*/
/**************************************************************************/
void RTC_RV3032C7::enable32K(void) {
  uint8_t status = read_register(RV3032C7_STATUSREG);
  status |= (0x1 << 0x03);
  write_register(RV3032C7_STATUSREG, status);
}

/**************************************************************************/
/*!
    @brief  Disable 32KHz Output
*/
/**************************************************************************/
void RTC_RV3032C7::disable32K(void) {
  uint8_t status = read_register(RV3032C7_STATUSREG);
  status &= ~(0x1 << 0x03);
  write_register(RV3032C7_STATUSREG, status);
}

/**************************************************************************/
/*!
    @brief  Get status of 32KHz Output
    @return True if enabled otherwise false
*/
/**************************************************************************/
bool RTC_RV3032C7::isEnabled32K(void) {
  return (read_register(RV3032C7_STATUSREG) >> 0x03) & 0x01;
}
