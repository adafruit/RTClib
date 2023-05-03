/**************************************************************************/
/*
  Countdown Timer using a PCF8563 RTC connected via I2C and Wire lib
  with the INT pin wired to an interrupt-capable input.

  This sketch sets a countdown timer, and executes code when it reaches 0,
  then blinks the built-in LED like BlinkWithoutDelay, but without millis()!

  NOTE:
  You must connect the PCF8563's interrupt pin to your Arduino or other
  microcontroller on an input pin that can handle interrupts, and that has a
  pullup resistor, or add an external pull-up resistor. The pin will be briefly
  pulled low each time the countdown reaches 0. This example will not work
  without the interrupt pin connected!

  On Adafruit breakout boards, the interrupt pin is labeled 'INT' or 'SQW'.
*/
/**************************************************************************/

#include "RTClib.h"

// using an ESP12-E module:
// use 2 Wire SCL: GPIO5 and SDA: GPIO4 to connect to the RTC
RTC_PCF8563 rtc;

// Input pin with interrupt capability
// const int timerInterruptPin = 2;  // Most Arduinos
// const int timerInterruptPin = 5; // Adafruit Feather M0/M4/nRF52840
const int timerInterruptPin = 13; // ESP12F

// Variables modified during an interrupt must be declared volatile
volatile bool countdownInterruptTriggered = false;
volatile int numCountdownInterrupts = 0;

// Triggered by the PCF8563 Countdown Timer interrupt at the end of a countdown
// period. Meanwhile, the PCF8563 immediately starts the countdown again.
void IRAM_ATTR countdownOver() {
  // Set a flag to run code in the loop():
  Serial.println("ISR");
  countdownInterruptTriggered = true;
  numCountdownInterrupts++;
  rtc.clearTimer();
}

// Triggered by normal operation of timer INT pin
void IRAM_ATTR toggleLed() {
  // Run certain types of fast executing code here:
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  // The clearTimer() function is called to reset the timer Flag
  rtc.clearTimer();
}

// Triggered by TI_TP mode of timer INT pin
void IRAM_ATTR pulsedToggleLed() {
  // Run certain types of fast executing code here:
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  // As we are using the TI_TP mode, the clearTimer() function is not called
  // to reset the timer Flag as it is done automatically by the PCF8563.
}

void setup() {

  Serial.begin(57600);
  Serial.println("\n");

#ifndef ESP8266
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB
#endif

  // initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1)
      delay(10);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println(F("\nStarting PCF8563 Countdown Timer example."));
  Serial.print(
      F("Configured to expect PCF8563 INT/SQW pin connected to input pin: "));
  Serial.println(timerInterruptPin);
  Serial.println(
      F("This example will not work without the interrupt pin connected!\n\n"));

  // Timer configuration is not cleared on an RTC reset due to battery backup!
  rtc.disableAlarm();
  rtc.disableTimer();

  Serial.println(
      F("First, use the PCF8563's 'Countdown Timer' with an interrupt."));
  Serial.println(
      F("Set the countdown for 10 seconds and we'll let it run for 2 rounds."));
  Serial.println(F("Starting Countdown Timer now..."));

  // These are the PCF8563's built-in "Timer Source Clock Frequencies".
  // They are predefined time periods you choose as your base unit of time,
  // depending on the length of countdown timer you need.
  // The minimum length of your countdown is 1 time period.
  // The maximum length of your countdown is 255 time periods.
  //
  // PCF8563_TimerFrequencyMinute = 1 minute, max 4.25 hours
  // PCF8563_TimerFrequencySecond = 1 second, max 4.25 minutes
  // PCF8563_TimerFrequency64Hz   = 1/64 of a second (15.625 milliseconds),
  // max 3.984 seconds
  // PCF8563_TimerFrequency4kHz   = 1/4096 of a second (244
  // microseconds), max 62.256 milliseconds
  //
  // Uncomment an example below:

  // rtc.enableTimer(PCF8563_TimerFrequencyHour, 24);    // 1 day
  // rtc.enableTimer(PCF8563_TimerFrequencyMinute, 150); // 2.5 hours
  rtc.enableTimer(PCF8563_TimerFrequencySecond, 10); // 10 seconds
  // rtc.enableTimer(PCF8563_TimerFrequency64Hz, 32);    // 1/2 second
  // rtc.enableTimer(PCF8563_TimerFrequency64Hz, 16);    // 1/4 second
  // rtc.enableTimer(PCF8563_TimerFrequency4kHz, 205);   // 50
  // milliseconds

  attachInterrupt(digitalPinToInterrupt(timerInterruptPin), countdownOver,
                  FALLING);

  // This message proves we're not blocked while counting down!
  Serial.println(F("  While we're waiting, a word of caution:"));
  Serial.println(F("  When starting a new countdown timer, the first time "
                   "period is not of fixed"));
  Serial.println(F("  duration. The amount of inaccuracy for the first time "
                   "period is up to one full"));
  Serial.println(F("  clock frequency. Example: just the first second of the "
                   "first round of a new"));
  Serial.println(
      F("  countdown based on PCF8563_TimerFrequencySecond may be off by "
        "as much as 1 second!"));
  Serial.println(F("  For critical timing, consider starting actions on the "
                   "first interrupt."));
}

void loop() {
  if (countdownInterruptTriggered && numCountdownInterrupts == 1) {
    Serial.println(
        F("Countdown interrupt triggered. Accurate timekeeping starts now."));
    countdownInterruptTriggered = false; // don't come in here again
  } else if (countdownInterruptTriggered && numCountdownInterrupts == 2) {
    Serial.println(F("2nd countdown interrupt triggered. Disabling countdown "
                     "and detaching interrupt.\n\n"));
    rtc.disableTimer();
    detachInterrupt(digitalPinToInterrupt(timerInterruptPin));
    delay(2000);

    Serial.println(F("Now, set up the PCF8563's Timer to toggle the "
                     "built-in LED at 1Hz..."));
    Serial.println(
        F("This time, we'll use the PCF8563's Timer Interrupt Pin "
          "and reset the Timer Flag within the interrupt service routine."));
    attachInterrupt(digitalPinToInterrupt(timerInterruptPin), toggleLed,
                    FALLING);
    rtc.enableTimer(PCF8563_TimerFrequency64Hz, 64);
    Serial.println(F("Look for the built-in LED to flash 1 second ON, 1 second "
                     "OFF, repeat. "));
    Serial.println(F("Meanwhile this program will use delay() to block code "
                     "execution briefly"));
    Serial.println(F("before moving on to the last example. Notice the LED "
                     "keeps blinking!\n\n"));
    delay(20000); // less accurate, blocks execution here. Meanwhile Second
                  // Timer keeps running.

    Serial.println(F("Now, set up the PCF8563's Timer to toggle the "
                     "built-in LED at 2Hz..."));
    Serial.println(F("This time, we'll use the PCF8563's Timer Interrupt Pin "
                     "with a pulsed timer interrupt. So no need to reset the "
                     "Timer Flag within the interrupt service routine."));
    attachInterrupt(digitalPinToInterrupt(timerInterruptPin), pulsedToggleLed,
                    FALLING);
    rtc.enableTimer(PCF8563_TimerFrequency64Hz, 32,
                    /* Enable the pulse */ true);
    Serial.println(
        F("Look for the built-in LED to flash 0.5 second ON, 0.5 second "
          "OFF, repeat. "));
    Serial.println(F("This is the end of this example, the LED will keep "
                     "blinking forever.\n\n"));

    countdownInterruptTriggered = false; // don't come in here again
  }

  // Nothing to do here...
}