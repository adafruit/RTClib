/**
 * Sets a timer for 10 seconds, and watches a pin (MONITOR_PIN) for
 * its interrupt signal.
 *
 * The RTC's INT line is pulled down when the timer goes off and the
 * interrupt is active.
 */
#include <Wire.h>
#include "RTClib.h"

#define MONITOR_PIN 5

RTC_PCF8523 rtc;
Pcf8523TimerState state;

void setup () {
  Serial.begin(9600);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  pinMode(MONITOR_PIN, INPUT_PULLUP);

  /*
  struct type signatures:

  typedef struct {
    bool irupt_flag;    // whether the timer has gone off
    bool irupt_enabled; // whether the flag state is tied to the interrupt pin state
  } Pcf8523IruptState;

  typedef struct {
    bool enabled;                  // whether the timer is running
    uint8_t value;                 // the current value of the timer
    Pcf8523FrequencyDivision freq; // the clock divider used
    Pcf8523IruptState irupt_state; // the timer's interrupt state
  } Pcf8523TimerState;
  */

  state.enabled = true;
  state.value = 10;
  state.freq = PCF8523_Freq_second;
  state.irupt_state.irupt_flag = false;
  state.irupt_state.irupt_enabled = true;

  rtc.write_timer(PCF8523_Timer_Countdown_B, &state);
}

void loop () {
    rtc.read_timer(PCF8523_Timer_Countdown_B, &state);
    Serial.print("timer value: ");
    Serial.print(state.value, DEC);
    Serial.print(", enabled: ");
    Serial.print(state.enabled ? "yes": "no");
    Serial.print(", freq: ");
    Serial.print(state.freq, DEC);
    Serial.println();
    Serial.print("irupt flag: ");
    Serial.print(state.irupt_state.irupt_flag, DEC);
    Serial.print(", enabled: ");
    Serial.print(state.irupt_state.irupt_enabled, DEC);
    Serial.println();

    Serial.print("Interrupt pin: ");
    Serial.println(digitalRead(MONITOR_PIN) ? "HIGH": "LOW");

    Serial.println();
    delay(1000);
}
