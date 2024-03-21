/**************************************************************************/
/*
  Timer examples using a RV3032C7 RTC connected via I2C and Wire lib
  with the INT pin wired to an interrupt-capable input.

  According to the application manual, the RV3032C7 countdown timer can count from 244.14 microseconds to 4095 minutes (almost 3 days):
  https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3032-C7_App-Manual.pdf

  This sketch sets a countdown timer, and executes code when it reaches 0,
  then blinks the built-in LED like BlinkWithoutDelay, but without millis()!

  NOTE:
  You must connect the RV3032C7's INT pin to your Arduino or other
  microcontroller on an input pin that can handle interrupts, and that has a
  pullup resistor. The pin will be briefly pulled low each time the countdown
  reaches 0. This example will not work without the interrupt pin connected!

  On boards that defines PIN_NEOPIXEL (such as the Adafruit QT Py) the neopixel will be blinked rather than the LED_BUILTIN

*/
/**************************************************************************/

#include "RTClib.h"

#ifdef PIN_NEOPIXEL
#include <Adafruit_NeoPixel.h>
// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL);
const uint32_t  neopixel_on = pixels.Color(255, 0, 0);
#endif //PIN_NEOPIXEL

RTC_RV3032C7 rtc;

// Input pin with interrupt capability
const int timerInterruptPin = 2;  // Most Arduinos, Adafruit Qt Py 
//const int timerInterruptPin = 5;  // Adafruit Feather M0/M4/nRF52840

// Variables modified during an interrupt must be declared volatile
volatile bool countdownInterruptTriggered = false;
volatile int numCountdownInterrupts = 0;

void setup () {
  Serial.begin(57600);

#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  //dumpRegisters();

  if (rtc.lostPower()) {
     // this will adjust to the date and time at compilation
     rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  //we don't need the CLKOUT Pin, so disable it
  rtc.disableClkOut();

# ifdef PIN_NEOPIXEL
    pixels.begin();  // initialize the pixel  
# else
    pinMode(LED_BUILTIN, OUTPUT);
# endif //PIN_NEOPIXEL

  // Set the pin attached to RV3032C7 INT to be an input with pullup to HIGH.
  // The RV3032C7 interrupt pin will pull it LOW at the end of a given
  // countdown period, then it will be released to be pulled HIGH again when clearCountdownTimer() is called.
  pinMode(timerInterruptPin, INPUT_PULLUP);

  Serial.println(F("\nStarting RV3032C7 Periodic Countdown Timer example."));
  Serial.print(F("Configured to expect RV3032C7 INT pin connected to input pin: "));
  Serial.println(timerInterruptPin);
  Serial.println(F("This example will not work without the interrupt pin connected!\n\n"));

  // Timer configuration is not cleared on an RTC reset due to battery backup!
  rtc.deconfigureAllTimers();

  Serial.println(F("First, use the RV3032C7's 'Countdown Timer' with an interrupt."));
  Serial.println(F("Set the countdown for 10 seconds and we'll let it run for 2 rounds."));

  //dumpRegisters();
  Serial.println(F("Starting Countdown Timer now..."));

  // These are the RV3032C7's built-in "Timer Clock Frequency selection field TD".
  // They are predefined time periods you choose as your base unit of time,
  // depending on the length of countdown timer you need.
  // The minimum length of your countdown is 1 time period.
  // The maximum length of your countdown is 4095 time periods.
  //
  // RV3032C7_FrequencyMinute = 1 minute, max 4095 minutes (68 hours and 15 minutes)
  // RV3032C7_FrequencySecond = 1 second, max 4095 seconds (68 minutes and 15 seconds)
  // RV3032C7_Frequency64Hz   = 1/64 of a second (15.625 milliseconds), max approx 64 seconds
  // RV3032C7_Frequency4096Hz   = 1/4096 of a second (244 microseconds), max approx 1 second
  //
  // Uncomment an example below:
  // rtc.enableCountdownTimer(RV3032C7_FrequencyMinute, 150); // 2.5 hours
  rtc.enableCountdownTimer(RV3032C7_FrequencySecond, 10);  // 10 seconds
  // rtc.enableCountdownTimer(RV3032C7_Frequency64Hz, 32);    // 1/2 second
  // rtc.enableCountdownTimer(RV3032C7_Frequency64Hz, 16);    // 1/4 second
  // rtc.enableCountdownTimer(RV3032C7_Frequency4096Hz, 4095);   // slightly below 1 second
  dumpRegisters();

  attachInterrupt(digitalPinToInterrupt(timerInterruptPin), countdownOver, FALLING);

  // This message proves we're not blocked while counting down!
  Serial.println(F("  While we're waiting, a word of caution:"));
  Serial.println(F("  When starting a new countdown timer, the first time period is not of fixed"));
  Serial.println(F("  duration. The amount of inaccuracy for the first time period is up to one full"));
  Serial.println(F("  clock frequency. Example: just the first second of the first round of a new"));
  Serial.println(F("  countdown based on RV3032C7_FrequencySecond may be off by as much as 1 second!"));
  Serial.println(F("  See also the FIRST PERIOD DURATION chapter in the RV3032C7 Application Manual"));
  Serial.println(F("  For critical timing, consider starting actions on the first interrupt."));
}

// Triggered by the RV3032C7 Countdown Timer interrupt at the end of a countdown
// period. Meanwhile, the RV3032C7 immediately starts the countdown again.
void countdownOver () {
  // Set a flag to run code in the loop():
  countdownInterruptTriggered = true;
  numCountdownInterrupts++;
}

// Triggered by the RV3032C7 Second Timer every second.
void toggleLed () {
  // Run certain types of fast executing code here:
# ifdef PIN_NEOPIXEL
    if (pixels.getPixelColor(0) == neopixel_on) {
       pixels.clear(); 
    } else {
       pixels.setPixelColor(0, neopixel_on);
    }
    pixels.show();
# else
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
# endif //PIN_NEOPIXEL
}

void loop () {
  if (countdownInterruptTriggered && numCountdownInterrupts == 1) {
    Serial.println(F("1st countdown interrupt triggered. Accurate timekeeping starts now."));
    countdownInterruptTriggered = false; // don't come in here again
  } else if (countdownInterruptTriggered && numCountdownInterrupts == 2) {
    Serial.println(F("2nd countdown interrupt triggered. Disabling countdown and detaching interrupt.\n\n"));
    rtc.disableCountdownTimer();
    detachInterrupt(digitalPinToInterrupt(timerInterruptPin));
    delay(2000);

    // TODO: 2nd timer not implemented - enable when implemented 
    /*
    Serial.println(F("Now, set up the RV3032C7's 'Second Timer' to toggle the built-in LED at 1Hz..."));
    attachInterrupt(digitalPinToInterrupt(timerInterruptPin), toggleLed, FALLING);
    rtc.enableSecondTimer();
    Serial.println(F("Look for the built-in LED to flash 1 second ON, 1 second OFF, repeat. "));
    Serial.println(F("Meanwhile this program will use delay() to block code execution briefly"));
    Serial.println(F("before moving on to the last example. Notice the LED keeps blinking!\n\n"));
    delay(20000); // less accurate, blocks execution here. Meanwhile Second Timer keeps running.
    rtc.disableSecondTimer();
    detachInterrupt(digitalPinToInterrupt(timerInterruptPin));
    */

    Serial.println(F("Lastly, set up a Countdown Timer that works without using the RV3032C INT pin (Polling via i2c)..."));
    rtc.enableCountdownTimer(RV3032C7_Frequency64Hz, 32, RV3032C7_EV_Poll);
    dumpRegisters();
    Serial.println(F("Look for the LED to toggle every 1/2 second"));
    Serial.println(F("The countdown was set to a source clock frequency of 64 Hz (1/64th of a second)"));
    Serial.println(F("for a length of 32 time periods. 32 * 1/64th of a second is 1/2 of a second."));
    Serial.println(F("When the timer fires the led is toggled."));
    Serial.println(F("The loop() polls the RV3032C7 via i2c to detect the timer event. When detected the timer is cleared and the led is toggled."));

    countdownInterruptTriggered = false; // don't come in here again
  } else if (rtc.countdownTimerFired()) { // detect the timer event reading the Timer Flag (TF) via i2c
    rtc.clearCountdownTimer();     // clears the flag, until the next time
    toggleLed();
  }
}

void dumpRegisters(void) {
  Serial.println(F("Alarm registers:"));
  print_i2c_register_hex(0x08);
  print_i2c_register_hex(0x09);
  print_i2c_register_hex(0x0A);

  Serial.println();
  Serial.println(F("Status register:"));
  print_i2c_register_bin(0x0D);
  Serial.println(F("Control registers:"));
  print_i2c_register_bin(0x10);
  print_i2c_register_bin(0x11);
  print_i2c_register_bin(0x12);
  Serial.println(F("Clock INT mask:"));
  print_i2c_register_bin(0x14);

  Serial.println();
  Serial.println(F("EEPROM MIRROR:"));
  print_i2c_register_bin(0xC0);
  print_i2c_register_bin(0xC1);
  print_i2c_register_bin(0xC2);
  print_i2c_register_bin(0xC3);
}

static uint8_t read_i2c_register(uint8_t reg) {
  Wire.beginTransmission(0x51);
  Wire.write((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(0x51, (byte)1);
  return Wire.read();
}

static void print_i2c_register_bin(uint8_t reg) {
  Serial.print(F("REG "));
  Serial.print(reg, HEX);
  Serial.print(F(" = "));
  Serial.print(read_i2c_register(reg), BIN);
  Serial.println(F(" b"));
}

static void print_i2c_register_hex(uint8_t reg) {
  Serial.print(F("REG 0x"));
  Serial.print(reg, HEX);
  Serial.print(F(" = 0x"));
  Serial.println(read_i2c_register(reg), HEX);
}
