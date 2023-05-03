
#include "RTClib.h"

// using an ESP12-E module:
// use 2 Wire SCL: GPIO5 and SDA: GPIO4 to connect to the RTC
RTC_PCF8563 rtc;

char daysOfTheWeek[7][12] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday"};

// Input pin with interrupt capability
// const int timerInterruptPin = 2;  // Most Arduinos
// const int timerInterruptPin = 5; // Adafruit Feather M0/M4/nRF52840
const int timerInterruptPin = 13; // ESP12F

// Variables modified during an interrupt must be declared volatile
volatile bool alarmTriggered = false;

DateTime alarm = DateTime();
uint32_t timeSpan = 120;

// Triggered by the PCF8563 Countdown Timer interrupt at the end of a countdown
// period. Meanwhile, the PCF8563 immediately starts the countdown again.
void isrCheckAlarm() {
  // Set a flag to run code in the loop():
  // Check if the interrupt was triggered by the alarm
  // When using the alarm and the timer, the interrupt could be triggered by
  // both.
  if (rtc.alarmFired()) {
    alarmTriggered = true;
    rtc.clearAlarm();
  }
}

void printDateTime(DateTime dt, String prefix = "") {
  if (prefix != "") {
    Serial.print(prefix);
  }
  Serial.print(dt.year(), DEC);
  Serial.print('/');
  Serial.print(dt.month(), DEC);
  Serial.print('/');
  Serial.print(dt.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[dt.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(dt.hour(), DEC);
  Serial.print(':');
  Serial.print(dt.minute(), DEC);
  Serial.print(':');
  Serial.print(dt.second(), DEC);
  Serial.println();
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

  if (rtc.lostPower()) {
    Serial.println("Detected a 'power loss', RTC is NOT initialized!");
  }

  // Clear any pending alarms or timers
  rtc.clearAlarm();
  rtc.clearTimer();
  rtc.disableAlarm();
  rtc.disableTimer();

  // Use compile time to set the RTC
  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  rtc.adjust(compileTime);
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // DateTime compileTime = DateTime(2014, 5, 21, 11, 22, 33);
  // rtc.adjust(compileTime);

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  // Enable the interrupt to check the alarm
  attachInterrupt(digitalPinToInterrupt(timerInterruptPin), isrCheckAlarm,
                  FALLING);

  // Set the alarm to trigger in 2 Minutes using a DateTime object
  // The alarm will be triggered on exact match, so when the minutes,
  // hours, day of the month and day of the week the alarm time.
  alarm = compileTime + TimeSpan(timeSpan);

  rtc.enableAlarm(alarm, /* minute_alarm */ true,
                  /* hour_alarm */ true, /* day_alarm */ true,
                  /* weekday_alarm*/ true);

  Serial.print(F("Current Time set to compile time: "));
  printDateTime(rtc.now());
  Serial.println();

  Serial.print(F("Waiting for initial alarm interrupt at: "));
  printDateTime(alarm);
  Serial.println();

  Serial.println(F("=> The inital Interrupt will be triggered in ~2 Minutes! "
                   "After that the next alarm "));
  Serial.println(F("   will be set to twice the time span and so on!"));
  Serial.println(F("=> Be aware that the PCF8573s alarm does not have a "
                   "seconds field, so the alarm will"));
  Serial.println(F("   trigger at the minute value! Depending on the time the "
                   "alarm was set, this could be"));
  Serial.println(F("   up to 59 seconds earlier! e.g. if a two minute alarm "
                   "was set at 12:00:59, the alarm"));
  Serial.println(F("   will trigger 61 seconds later at at 12:02:00"));
}

void loop() {
  if (alarmTriggered) {
    Serial.println();
    Serial.print("Alarm Triggered: ");
    printDateTime(rtc.now());

    timeSpan *= 2;
    alarm = rtc.now() + TimeSpan(timeSpan);
    rtc.enableAlarm(alarm, /* minute_alarm */ true,
                    /* hour_alarm */ true, /* day_alarm */ true,
                    /* weekday_alarm*/ true);
    Serial.print(F("Set new Alarm to "));
    printDateTime(alarm);
    Serial.println();
    alarmTriggered = false;
  } else {
    Serial.print(".");
  }

  // Exit condition to double check the Alarm
  // A 10 second buffer is added to the alarm to account
  // for the time it takes to print the output.
  if (rtc.now() > alarm + TimeSpan(10)) {
    Serial.println();
    Serial.println("Alarm should already been triggered:");
    printDateTime(rtc.now(), "Now:   ");
    printDateTime(alarm, "Alarm: ");
    Serial.println(
        F("=> This should not happen, check the alarm settings and wiring!"));
    while (1) {
      delay(1000);
    }
  }

  delay(1000); // wait one second
}
