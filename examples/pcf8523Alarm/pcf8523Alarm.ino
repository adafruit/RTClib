#include "RTClib.h"

// Change this to reflect the interrupt pin you would like to use
#define INTERRUPT_PIN 3

RTC_PCF8523 rtc;

bool alarm_triggered = false;

void setAlarmExample();
void alarmISR();
void handleAlarmTriggered();

void setup() {
    Serial.begin(57600);

    // Ensure RTC initializes
    if (!rtc.begin()) {
         Serial.println("Couldn't find RTC");
         Serial.flush();
         while (1) delay(10);
    }

    doAlarmExample();
}

void loop() {
    if (alarm_triggered) {
        handleAlarmTriggered();
    } else {
        Serial.println("Alarm not triggered. The time is " + rtc.now().timestamp());
        delay(1000);
    }
}

/*
 * Sets alarm one minute later.
 */
void doAlarmExample()
{
    // Make alarm trigger one minute after current time, but not accounting for seconds
    // as the PCF8523 alarm doesn't have an alarm seconds register. So for example if
    // current_time is 12:00:50, and alarm_time is 12:01:50, it will trigger at
    // the exact minute, e.g. 12:01:00, because the PCF8523 hardware does not allow
    // for seconds to be set on the alarm and ignores them.
    DateTime current_time = rtc.now();
    TimeSpan alarm_offset(0, 0, 1, 0); // One minute
    DateTime alarm_time(current_time + alarm_offset);

    // Good to clear other timers and such.
    rtc.deconfigureAllTimers();

    rtc.enableAlarmTimer(alarm_time, PCF8523_AlarmDate);

    // Print the current time for demonstration purposes.
    char current_time_buf[] = "YYYY, MMM, DD, DDD, hh:mm:ss AP";
    Serial.println(String("Current Time: ") + current_time.toString(current_time_buf));
    Serial.flush();

    // Print when the alarm should trigger.
    char alarm_time_buf[] = "YYYY, MMM, DD, DDD, hh:mm:00 AP";
    Serial.println(String("Alarm Time: ") + alarm_time.toString(alarm_time_buf));
    Serial.flush();

    // Set the interrupt.
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), alarmISR, LOW);
}

void handleAlarmTriggered()
{
    alarm_triggered = false;
    rtc.disableAlarmTimer();
    Serial.println("Alarm triggered.\n");
    doAlarmExample();

}

void alarmISR()
{
  alarm_triggered = true;
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
}
