// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <Wire.h>
#include "RTClib.h"
#ifdef __AVR__
 #include <avr/pgmspace.h>
#elif defined(ESP8266)
 #include <pgmspace.h>
#elif defined(ARDUINO_ARCH_SAMD)
// nothing special needed
#elif defined(ARDUINO_SAM_DUE)
 #define PROGMEM
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
 #define Wire Wire1
#endif



#if (ARDUINO >= 100)
 #include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
 // Macro to deal with the difference in I2C write functions from old and new Arduino versions.
 #define _I2C_WRITE write
 #define _I2C_READ  read
#else
 #include <WProgram.h>
 #define _I2C_WRITE send
 #define _I2C_READ  receive
#endif


static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((uint8_t)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (uint8_t)1);
  return Wire._I2C_READ();
}

// vals[] must be allocated for not less than howMany bytes!
static uint8_t read_i2c_registers(uint8_t addr, uint8_t start, uint8_t howMany, uint8_t vals[]) {
    uint8_t i,j=0,thisMuch;
    memset(vals,0,howMany*sizeof(uint8_t));
    Wire.beginTransmission(addr);
    Wire._I2C_WRITE((uint8_t)start);
    Wire.endTransmission();
    Wire.requestFrom(addr, (uint8_t)howMany);
    while(howMany>0) {
        thisMuch=Wire.available(); // The slave may return less than requested
        for(i=0;i<thisMuch;++i) {
            vals[i+j]=Wire._I2C_READ();
        }
        j+=thisMuch;
        if(thisMuch) howMany-=thisMuch;
        else howMany=0;
        if(howMany>0) Wire.requestFrom(addr, (uint8_t)howMany);
    }
    return j;
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((uint8_t)reg);
  Wire._I2C_WRITE((uint8_t)val);
  Wire.endTransmission();
}

// vals[] must be allocated for not less than howMany bytes!
static void write_i2c_registers(uint8_t addr, uint8_t start, uint8_t howMany, uint8_t vals[]) {
    uint8_t i;
    Wire.beginTransmission(addr);
    Wire._I2C_WRITE((uint8_t)start);
    for(i=0;i<howMany;++i) {
        Wire._I2C_WRITE((uint8_t)vals[i]);
    }
    Wire.endTransmission();
}

static uint8_t set_i2c_register_bit(uint8_t addr, uint8_t reg, uint8_t bitNum)
{
    uint8_t ss = read_i2c_register(addr,reg);
    ss |= ((uint8_t)1 << bitNum);
    write_i2c_register(addr,reg,ss);
    return read_i2c_register(addr,reg);
}

static uint8_t clear_i2c_register_bit(uint8_t addr, uint8_t reg, uint8_t bitNum)
{
    uint8_t ss = read_i2c_register(addr, reg);
    ss &= ~((uint8_t)1 << bitNum);
    write_i2c_register(addr,reg,ss);
    return read_i2c_register(addr,reg);
}

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

DateTime::DateTime (const DateTime& copy):
  yOff(copy.yOff),
  m(copy.m),
  d(copy.d),
  hh(copy.hh),
  mm(copy.mm),
  ss(copy.ss)
{}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using F() would further reduce the RAM footprint, see below.
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    yOff = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
    switch (date[0]) {
        case 'J': m = date[1] == 'a' ? 1 : m = date[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

// A convenient constructor for using "the compiler's time":
// This version will save RAM by using PROGMEM to store it by using the F macro.
//   DateTime now (F(__DATE__), F(__TIME__));
DateTime::DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    char buff[11];
    memcpy_P(buff, date, 11);
    yOff = conv2d(buff + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (buff[0]) {
        case 'J': m = buff[1] == 'a' ? 1 : m = buff[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = buff[2] == 'r' ? 4 : 8; break;
        case 'M': m = buff[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(buff + 4);
    memcpy_P(buff, time, 8);
    hh = conv2d(buff);
    mm = conv2d(buff + 3);
    ss = conv2d(buff + 6);
}

uint8_t DateTime::dayOfTheWeek() const {    
    uint16_t day = date2days(yOff, m, d);
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

long DateTime::secondstime(void) const {
  long t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  return t;
}

DateTime DateTime::operator+(const TimeSpan& span) {
  return DateTime(unixtime()+span.totalseconds());
}

DateTime DateTime::operator-(const TimeSpan& span) {
  return DateTime(unixtime()-span.totalseconds());
}

TimeSpan DateTime::operator-(const DateTime& right) {
  return TimeSpan(unixtime()-right.unixtime());
}

////////////////////////////////////////////////////////////////////////////////
// TimeSpan implementation

TimeSpan::TimeSpan (int32_t seconds):
  _seconds(seconds)
{}

TimeSpan::TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds):
  _seconds((int32_t)days*86400L + (int32_t)hours*3600 + (int32_t)minutes*60 + seconds)
{}

TimeSpan::TimeSpan (const TimeSpan& copy):
  _seconds(copy._seconds)
{}

TimeSpan TimeSpan::operator+(const TimeSpan& right) {
  return TimeSpan(_seconds+right._seconds);
}

TimeSpan TimeSpan::operator-(const TimeSpan& right) {
  return TimeSpan(_seconds-right._seconds);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

boolean RTC_DS1307::begin(void) {
  Wire.begin();
  return true;
}

uint8_t RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return !(ss>>7);
}

void RTC_DS1307::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();
}

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
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

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

void RTC_DS1307::writeSqwPinMode(Ds1307SqwPinMode mode) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire._I2C_WRITE(mode);
  Wire.endTransmission();
}

void RTC_DS1307::readnvram(uint8_t* buf, uint8_t size, uint8_t address) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t) DS1307_ADDRESS, size);
  for (uint8_t pos = 0; pos < size; ++pos) {
    buf[pos] = Wire._I2C_READ();
  }
}

void RTC_DS1307::writenvram(uint8_t address, uint8_t* buf, uint8_t size) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  for (uint8_t pos = 0; pos < size; ++pos) {
    Wire._I2C_WRITE(buf[pos]);
  }
  Wire.endTransmission();
}

uint8_t RTC_DS1307::readnvram(uint8_t address) {
  uint8_t data;
  readnvram(&data, 1, address);
  return data;
}

void RTC_DS1307::writenvram(uint8_t address, uint8_t data) {
  writenvram(address, &data, 1);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

void RTC_Millis::adjust(const DateTime& dt) {
    offset = dt.unixtime() - millis() / 1000;
}

DateTime RTC_Millis::now() {
  return (uint32_t)(offset + millis() / 1000);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// RTC_PCF8563 implementation

boolean RTC_PCF8523::begin(void) {
  Wire.begin();
  return true;
}

boolean RTC_PCF8523::initialized(void) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return ((ss & 0xE0) != 0xE0);
}

void RTC_PCF8523::adjust(const DateTime& dt) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3); // start at location 3
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(0)); // skip weekdays
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();

  // set to battery switchover mode
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire._I2C_WRITE((byte)0x00);
  Wire.endTransmission();
}

DateTime RTC_PCF8523::now() {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3);	
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  uint8_t d = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();  // skip 'weekdays'
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

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

void RTC_PCF8523::writeSqwPinMode(Pcf8523SqwPinMode mode) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire._I2C_WRITE(mode << 3);
  Wire.endTransmission();
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// RTC_DS3231 implementation

boolean RTC_DS3231::begin(void) {
  Wire.begin();
  return true;
}

bool RTC_DS3231::lostPower(void) {
  return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

void RTC_DS3231::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);
}

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
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

Ds3231SqwPinMode RTC_DS3231::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_CONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds3231SqwPinMode>(mode);
}

void RTC_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode) {
  uint8_t ctrl;
  ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  if (mode == DS3231_OFF) {
    ctrl |= 0x04; // turn on INTCN
  } else {
    ctrl |= mode;
  } 
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);

  //Serial.println( read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), HEX);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// RTC_PCF85263 implementation

bool RTC_PCF85263::begin(void)
{
    Wire.begin();
    return true;
}

bool RTC_PCF85263::initialized(void)
{
    uint8_t ss = clear_i2c_register_bit(my_I2C_address, PCF85263_SECONDS, PCF85263_OS_BIT);
    return !(ss & ((uint8_t)1 << PCF85263_OS_BIT)); // Check the OS bit
}

DateTime RTC_PCF85263::now()
{
    uint8_t vals[7];
    read_i2c_registers(my_I2C_address,PCF85263_SECONDS,7,vals);
    uint8_t ss = bcd2bin(vals[0] & ~((uint8_t)1 << PCF85263_OS_BIT)); // clear OS bit
    uint8_t mm = bcd2bin(vals[1] & ~((uint8_t)1 << PCF85263_EMON_BIT)); // clear EMON bit
    uint8_t hh = bcd2bin(vals[2]);
    uint8_t d = bcd2bin(vals[3]);
    // skip 'weekdays' = vals[4]
    uint8_t m = bcd2bin(vals[5]);
    uint16_t y = bcd2bin(vals[6]) + 2000;
    return DateTime (y, m, d, hh, mm, ss);
}

void RTC_PCF85263::adjust(const DateTime& dt)
{
    // Enable STOP bit
    set_i2c_register_bit(my_I2C_address,PCF85263_STOP,PCF85263_STOP_BIT);
    
    // Clear prescaler
    write_i2c_register(my_I2C_address,PCF85263_RESET,PCF85263_PRESCALER_BITS);
    
    // Set date/time
    uint8_t vals[7];
    vals[0]=bin2bcd(dt.second());
    vals[1]=bin2bcd(dt.minute());
    vals[2]=bin2bcd(dt.hour());
    vals[3]=bin2bcd(dt.day());
    vals[4]=bin2bcd(dt.dayOfTheWeek());
    vals[5]=bin2bcd(dt.month());
    vals[6]=bin2bcd(dt.year() - 2000);
    write_i2c_registers(my_I2C_address,PCF85263_SECONDS,7,vals);
    
    // Disable STOP bit
    clear_i2c_register_bit(my_I2C_address,PCF85263_STOP,PCF85263_STOP_BIT);
}

Pcf85263SqwPinMode RTC_PCF85263::readSqwPinMode()
{
    uint8_t mode=read_i2c_register(my_I2C_address,PCF85263_FUNCTION);
    mode &= 0x7;
    return static_cast<Pcf85263SqwPinMode>(mode);
}

void RTC_PCF85263::writeSqwPinMode(Pcf85263SqwPinMode mode)
{
    uint8_t m=(uint8_t)mode;
    m |= 0xF8 & read_i2c_register(my_I2C_address,PCF85263_FUNCTION);
    write_i2c_register(my_I2C_address,PCF85263_FUNCTION,m);
}

void RTC_PCF85263::defaultBatteryMode()
{
    // set to battery switchover default mode:
    // enabled, low refresh rate, switching at Vth level, Vth=1.5V
    write_i2c_register(my_I2C_address,PCF85263_BATTSWITCH,0x00);
}

void RTC_PCF85263::disableCLKOUTenableINTA()
{
    // Disable CLK and enable interrupts on CLK/INTA pin
    set_i2c_register_bit(my_I2C_address, PCF85263_PIN_IO, PCF85263_CLKPM_BIT);
    set_i2c_register_bit(my_I2C_address, PCF85263_PIN_IO, PCF85263_INTA_OUT_BIT);
}

void RTC_PCF85263::setAlarm(uint8_t alarm,const DateTime& dt)
{
    uint8_t vals[5];
    switch(alarm) {
        case 1:
        default:
            vals[0]=bin2bcd(dt.second());
            vals[1]=bin2bcd(dt.minute());
            vals[2]=bin2bcd(dt.hour());
            vals[3]=bin2bcd(dt.day());
            vals[4]=bin2bcd(dt.month());
            write_i2c_registers(my_I2C_address,PCF85263_ALARM1_SECONDS,5,vals);
            break;
        case 2:
            vals[0]=bin2bcd(dt.minute());
            vals[1]=bin2bcd(dt.hour());
            vals[2]=bin2bcd(dt.dayOfTheWeek());
            write_i2c_registers(my_I2C_address,PCF85263_ALARM2_MINUTES,3,vals);
            break;
    }
}

void RTC_PCF85263::setAlarm(uint8_t alarm,const uint8_t hour, const uint8_t minute)
{
    DateTime dt(0,0,0,hour,minute,0);
    setAlarm(alarm,dt);
}

// mode should be composed of individual bits. For example
// mode = (1 << PCF85263_ALARM2_MINUTES) | (1 << PCF85263_ALARM2_HOURS)
// will enable hour and minute of alarm 2
void RTC_PCF85263::enableAlarm(uint8_t mode)
{
    disableCLKOUTenableINTA();
    write_i2c_register(my_I2C_address,PCF85263_ALARM_ENABLES,mode);
    // Figure out the alarm number and set proper INTA bit(s)
    if(mode & ((1 << PCF85263_ALARM1_SECONDS_BIT) |
               (1 << PCF85263_ALARM1_MINUTES_BIT) |
               (1 << PCF85263_ALARM1_HOURS_BIT) |
               (1 << PCF85263_ALARM1_DAYS_BIT) |
               (1 << PCF85263_ALARM1_MONTHS_BIT)))
    {
        set_i2c_register_bit(my_I2C_address,PCF85263_INTA_ENABLES,PCF85263_INTA_A1IEA_BIT);
    }
    if(mode & ((1 << PCF85263_ALARM2_MINUTES_BIT) |
                  (1 << PCF85263_ALARM2_HOURS_BIT) |
                  (1 << PCF85263_ALARM2_DAY_OTW_BIT)))
    {
        set_i2c_register_bit(my_I2C_address,PCF85263_INTA_ENABLES,PCF85263_INTA_A2IEA_BIT);
    }
}

void RTC_PCF85263::disableAlarm(uint8_t alarm)
{
    switch(alarm) {
        case 1:
            clear_i2c_register_bit(my_I2C_address,PCF85263_INTA_ENABLES,PCF85263_INTA_A1IEA_BIT);
            break;
        case 2:
            clear_i2c_register_bit(my_I2C_address,PCF85263_INTA_ENABLES,PCF85263_INTA_A2IEA_BIT);
            break;
        default:
            break;
    }
}

// mode should be composed of individual bits. For example
// mode = (1 << PCF85263_INTA_PI_SECOND_BIT) | (1 << PCF85263_INTA_PI_MINUTE_BIT)
// will set periodic interrupt every hour
void RTC_PCF85263::setPeriodicInterrupt(PCF85263PeriodicIntMode mode)
{
    if((uint8_t)mode & (1 << PCF85263_INTA_PI_SECOND_BIT)) set_i2c_register_bit(my_I2C_address,PCF85263_FUNCTION,PCF85263_INTA_PI_SECOND_BIT);
    if((uint8_t)mode & (1 << PCF85263_INTA_PI_MINUTE_BIT)) set_i2c_register_bit(my_I2C_address,PCF85263_FUNCTION,PCF85263_INTA_PI_MINUTE_BIT);
}

void RTC_PCF85263::enablePeriodicInterrupt()
{
    disableCLKOUTenableINTA();
    set_i2c_register_bit(my_I2C_address,PCF85263_INTA_ENABLES,PCF85263_INTA_PIEA_BIT);
}

void RTC_PCF85263::disablePeriodicInterrupt()
{
    clear_i2c_register_bit(my_I2C_address,PCF85263_INTA_ENABLES,PCF85263_INTA_PIEA_BIT);
}

// mode = 0 for normal 7pF, 1 for low 6pF, 2 for high 12.5 pF
void RTC_PCF85263::setOscillatorCapacitance(uint8_t mode)
{
    switch(mode) {
        case 0:
        default:
            clear_i2c_register_bit(my_I2C_address,PCF85263_OSCILLATOR,PCF85263_OSC_HIGH_CAPACITANCE_BIT);
            clear_i2c_register_bit(my_I2C_address,PCF85263_OSCILLATOR,PCF85263_OSC_LOW_CAPACITANCE_BIT);
            break;
        case 1:
            clear_i2c_register_bit(my_I2C_address,PCF85263_OSCILLATOR,PCF85263_OSC_HIGH_CAPACITANCE_BIT);
            set_i2c_register_bit(my_I2C_address,PCF85263_OSCILLATOR,PCF85263_OSC_LOW_CAPACITANCE_BIT);
            break;
        case 2:
            set_i2c_register_bit(my_I2C_address,PCF85263_OSCILLATOR,PCF85263_OSC_HIGH_CAPACITANCE_BIT);
            clear_i2c_register_bit(my_I2C_address,PCF85263_OSCILLATOR,PCF85263_OSC_LOW_CAPACITANCE_BIT);
            break;
    }
}

void RTC_PCF85263::clearAllFlags()
{
    write_i2c_register(my_I2C_address,PCF85263_FLAGS,0x00);
}

////////////////////////////////////////////////////////////////////////////////
