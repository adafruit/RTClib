#include "RTClib.h"

RTC_DS3231 rtc;

int mode_index = 0;

Ds3231SqwPinMode modes[] = { DS3231_OFF, DS3231_SquareWave1Hz,
  DS3231_SquareWave1kHz, DS3231_SquareWave4kHz, DS3231_SquareWave8kHz };

void print_mode() {
  Ds3231SqwPinMode mode = rtc.readSqwPinMode();

  Serial.print("Sqw Pin Mode: ");
  switch(mode) {
    case DS3231_OFF:              Serial.println("OFF");       break;
    case DS3231_SquareWave1Hz:    Serial.println("1Hz");       break;
    case DS3231_SquareWave1kHz:   Serial.println("1.024kHz");  break;
    case DS3231_SquareWave4kHz:   Serial.println("4.096kHz");  break;
    case DS3231_SquareWave8kHz:   Serial.println("8.192kHz");  break;
    default:                      Serial.println("UNKNOWN");   break;
  }
}

void setup () {
  Serial.begin(57600);

#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  print_mode();
}

void loop () {
  rtc.writeSqwPinMode(modes[mode_index++]);
  print_mode();

  if (mode_index > 4) {
    mode_index = 0;
  }

  delay(5000);
}
