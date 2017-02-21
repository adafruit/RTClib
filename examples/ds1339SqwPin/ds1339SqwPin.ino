// SQW/INT pin mode using a DS1339 RTC connected via I2C.
//
// According to the data sheet (https://datasheets.maximintegrated.com/en/ds/DS1339-DS1339U.pdf), the
// DS1339's SQW/INT pin can be set to low, 1Hz, 4.096kHz, 8.192kHz, or 32.768kHz.
//
// This sketch reads the state of the pin, then iterates through the possible values at
// 5 second intervals.
//

// NOTE:
// You must connect a pull up resistor (~10kohm) from the SQW pin up to VCC.  Without
// this pull up the wave output will not work!

#include <Wire.h>
#include "RTClib.h"

#define SERIAL_PORT_SPEED   115200
#ifdef SERIAL_PORT_MONITOR
#define mySerial  SERIAL_PORT_MONITOR
#else
#define mySerial  Serial
#endif

RTC_DS1339 rtc;

Ds1339SqwPinMode modes[] =
{ DS1339_OFF,
  DS1339_SquareWave1HZ,
  DS1339_SquareWave4kHz,
  DS1339_SquareWave8kHz,
  DS1339_SquareWave32kHz
};

int mode_index = 0;

void print_mode() {
  Ds1339SqwPinMode mode = rtc.readSqwPinMode();

  mySerial.print("Sqw Pin Mode: ");
  switch (mode) {
    case DS1339_OFF:
      mySerial.println("OFF");
      break;
    case DS1339_SquareWave1HZ:
      mySerial.println("1HZ");
      break;
    case DS1339_SquareWave4kHz:
      mySerial.println("4.096kHz");
      break;
    case DS1339_SquareWave8kHz:
      mySerial.println("8.192kHz");
      break;
    case DS1339_SquareWave32kHz:
      mySerial.println("32.768kHz");
      break;
    default:
      mySerial.print("UNKNOWN ");
      mySerial.println(mode);
      break;
  }
}

void setup () {

#ifndef ESP8266
  while (!mySerial); // for Leonardo/Micro/Zero
#endif

  mySerial.begin(SERIAL_PORT_SPEED);
  if (! rtc.begin()) {
    mySerial.println("Couldn't find RTC");
    while (1);
  }

  mySerial.println("current");
  print_mode();
  mySerial.println("rotate OFF, 1HZ, 4.096kHz, 8.192kHz and 32.768kHz");
}

void loop () {
  rtc.writeSqwPinMode(modes[mode_index]);
  print_mode();

  mode_index = (mode_index + 1) % (sizeof(modes)/sizeof(modes[0]));

  delay(5000);
}
