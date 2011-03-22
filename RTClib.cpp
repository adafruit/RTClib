// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <avr/pgmspace.h>
#include <WProgram.h>
#include "RTClib.h"

#define HAVE_DS3234
#undef HAVE_DS1307

#define DS1307_ADDRESS 0x68
#define SECONDS_PER_DAY 86400L

#define SECONDS_FROM_1970_TO_2000 946684800

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

static uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

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

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using PSTR would further reduce the RAM footprint
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

uint8_t DateTime::dayOfWeek() const {    
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

char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

// as a string
char* DateTime::toString(char* buf, int maxlen) const
{
	snprintf(buf,maxlen,"%s %02u %04u %02u:%02u:%02u",
			 months[m-1],
			 d,
			 2000 + yOff,
			 hh,
			 mm,
			 ss
	);
	return buf;
}


////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

#ifdef HAVE_DS1307
#include <Wire.h>

uint8_t RTC_DS1307::begin(void) {
  return 1;
}

uint8_t RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.send(0);	
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = Wire.receive();
  return !(ss>>7);
}

void RTC_DS1307::adjust(const DateTime& dt) {
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

DateTime RTC_DS1307::now() {
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
#endif

////////////////////////////////////////////////////////////////////////////////
// RTC_DS3234 implementation

#ifdef HAVE_DS3234
#include <SPI.h>
// Registers we use
const int CONTROL_R = 0x0e;
const int CONTROL_W = 0x8e;
const int CONTROL_STATUS_R = 0x0f;
const int CONTROL_STATUS_W = 0x8f;
const int SECONDS_R = 0x00;
const int SECONDS_W = 0x80;

// Bits we use
const int EOSC = 7;
const int OSF = 7;

uint8_t RTC_DS3234::begin(void) {
	pinMode(cs_pin,OUTPUT);
	cs(HIGH);
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE1);
	
	//Enable oscillator, disable square wave, alarms
	cs(LOW);
	SPI.transfer(CONTROL_W);	
	SPI.transfer(0x0);
	cs(HIGH);	
	delay(1);
	
	//Clear oscilator stop flag, 32kHz pin
	cs(LOW);
	SPI.transfer(CONTROL_STATUS_W);	
	SPI.transfer(0x0);
	cs(HIGH);
	delay(1);
	
	return 1;
}

uint8_t RTC_DS3234::isrunning(void) {
	cs(LOW);
	SPI.transfer(CONTROL_R);
	uint8_t ss = SPI.transfer(-1);
	cs(HIGH);
	return !(ss & _BV(OSF));
}

void RTC_DS3234::adjust(const DateTime& dt) {
	cs(LOW);
	SPI.transfer(SECONDS_W);
    SPI.transfer(bin2bcd(dt.second()));
    SPI.transfer(bin2bcd(dt.minute()));
    SPI.transfer(bin2bcd(dt.hour()));
    SPI.transfer(bin2bcd(dt.dayOfWeek()));
    SPI.transfer(bin2bcd(dt.day()));
    SPI.transfer(bin2bcd(dt.month()));
    SPI.transfer(bin2bcd(dt.year() - 2000));
	cs(HIGH);

}

DateTime RTC_DS3234::now() {
	cs(LOW);
	SPI.transfer(SECONDS_R);
	uint8_t ss = bcd2bin(SPI.transfer(-1) & 0x7F);
	uint8_t mm = bcd2bin(SPI.transfer(-1));
	uint8_t hh = bcd2bin(SPI.transfer(-1));
	SPI.transfer(-1);
	uint8_t d = bcd2bin(SPI.transfer(-1));
	uint8_t m = bcd2bin(SPI.transfer(-1));
	uint16_t y = bcd2bin(SPI.transfer(-1)) + 2000;
	cs(HIGH);
	
	return DateTime (y, m, d, hh, mm, ss);
}
#endif

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
