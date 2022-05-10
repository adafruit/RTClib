/* Example implementation of an alarm using RV3032C7
 *
 * VCC and GND of RTC should be connected to some power source
 * SDA, SCL of RTC should be connected to SDA, SCL of arduino
 * INT should be connected to RTC_INTERRUPT_PIN
 * RTC_INTERRUPT_PIN needs to work with interrupts
 */

#include <RTClib.h>
// #include <Wire.h>

RTC_RV3032C7 rtc;

// the pin that is connected to INT
#define RTC_INTERRUPT_PIN 2

void setup() {
    Serial.begin(9600);

    // initializing the rtc
    if(!rtc.begin()) {
        Serial.println("Couldn't find RTC!");
        Serial.flush();
        while (1) delay(10);
    }

    if(rtc.lostPower()) {
        // this will adjust to the date and time at compilation
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    //we don't need the CLKOUT Pin, so disable it
    rtc.disableClkOut();

    // Making it so, that the alarm will trigger an interrupt
    pinMode(RTC_INTERRUPT_PIN, INPUT_PULLUP); // pullup is needed with RV3032C7 INT pin
    attachInterrupt(digitalPinToInterrupt(RTC_INTERRUPT_PIN), onAlarm, FALLING);

    // set alarm flag to false (so alarm didn't happen so far)
    // if not done, this easily leads to problems, as chip register aren't reset on reboot/recompile
    rtc.clearAlarm();
    rtc.disableAlarm();

    // schedule an alarm 60 seconds in the future
    if(!rtc.setAlarm(
            rtc.now() + TimeSpan(60),
            RV3032C7_A_Minute // this mode triggers the alarm when the minutes match. See Doxygen for other options
    )) {
        Serial.println("Error, alarm wasn't set!");
    }else {
        Serial.println("Alarm will happen within 60 seconds!");
    }
}

void loop() {
    // print current time
    char date[10] = "hh:mm:ss";
    rtc.now().toString(date);
    Serial.print(date);
    // the value at INT-Pin (because of pullup 1 means no alarm)
    Serial.print(" INT: ");
    Serial.print(digitalRead(RTC_INTERRUPT_PIN));
    // whether a alarm happened happened
    Serial.print(" Alarm: ");
    Serial.println(rtc.alarmFired());
    // status register values (see https://www.microcrystal.com/fileadmin/Media/Products/RTC/App.Manual/RV-3028-C7_App-Manual.pdf page 22)
    // Serial.print(" Control: 0d");
    // Serial.println(read_i2c_register(0x51, 0xd), BIN);

    // resetting INT and alarm 1 flag
    // using setAlarm, the next alarm could now be configured
    if(rtc.alarmFired()) {
        rtc.clearAlarm();
        Serial.println(); Serial.println("Alarm cleared"); Serial.println();
    }

    delay(2000);
}

void onAlarm() {
    Serial.println("Alarm occured!");
}

/*static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write((byte)reg);
    Wire.endTransmission();

    Wire.requestFrom(addr, (byte)1);
    return Wire.read();
}*/
