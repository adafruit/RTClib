// SQW/OUT pin mode using a DS1307 RTC connected via I2C.
//
// According to the data sheet (http://datasheets.maxim-ic.com/en/ds/DS1307.pdf), the
// DS1307's SQW/OUT pin can be set to low, high, 1Hz, 4.096kHz, 8.192kHz, or 32.768kHz.
//
// This sketch reads the state of the pin, then iterates through the possible values at
// 5 second intervals.
//

// NOTE:
// You must connect a pull up resistor (~10kohm) from the SQW pin up to VCC.  Without
// this pull up the wave output will not work!

#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

int mode_index = 0;

Ds1307SqwPinMode modes[] = {OFF, ON, SquareWave1HZ, SquareWave4kHz, SquareWave8kHz, SquareWave32kHz};


void print_mode() {
  Ds1307SqwPinMode mode = rtc.readSqwPinMode();
  
  Serial.print("Sqw Pin Mode: ");
  switch(mode) {
  case OFF:             Serial.println("OFF");       break;
  case ON:              Serial.println("ON");        break;
  case SquareWave1HZ:   Serial.println("1Hz");       break;
  case SquareWave4kHz:  Serial.println("4.096kHz");  break;
  case SquareWave8kHz:  Serial.println("8.192kHz");  break;
  case SquareWave32kHz: Serial.println("32.768kHz"); break;
  default:              Serial.println("UNKNOWN");   break;
  }
}

void setup () {
  Serial.begin(57600);
  Wire.begin();
  rtc.begin();

  print_mode();
}

void loop () {
  rtc.writeSqwPinMode(modes[mode_index++]);
  print_mode();

  if (mode_index > 5) {
    mode_index = 0;
  }

  delay(5000);
}
