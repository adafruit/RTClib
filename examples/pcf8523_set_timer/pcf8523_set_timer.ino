/**
 * Sets a timer for 10 seconds on start, and monitors the status/flag
 * values in a loop.
 */
#include <Wire.h>
#include "RTClib.h"

RTC_PCF8523 rtc;

void setup () {
  Serial.begin(9600);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  Pcf8523TimerState state = Pcf8523TimerState(
    10,     // timer value
    PCF8523_FrequencySecond, // timer frequency
    true,   // timer set to run
    false,  // timer interrupt flag (when timer has gone off)
    true    // timer flag -> signal enable
  );

  rtc.write_timer(PCF8523_Timer_Countdown_A, state);
}

void loop () {
    Pcf8523TimerState state = rtc.read_timer(PCF8523_Timer_Countdown_A);

    Serial.print("timer value: ");
    Serial.print(state.value, DEC);
    Serial.print(", enabled: ");
    Serial.print(state.enabled ? "yes": "no");
    Serial.print(", freq: ");
    Serial.print(state.freq, DEC);
    Serial.println();

    Serial.print("irupt flag: ");
    Serial.print(state.irupt_flag, DEC);
    Serial.print(", enabled: ");
    Serial.print(state.irupt_enabled, DEC);
    Serial.println();

    Serial.println();
    delay(1000);
}
