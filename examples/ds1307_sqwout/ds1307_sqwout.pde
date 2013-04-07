// SQW/OUT signal functions using a DS1307 RTC connected via I2C and Wire lib.
// 2012-11-14 idreammicro.com http://opensource.org/licenses/mit-license.php

#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 RTC;

void setup () {
    Serial.begin(57600);
    Wire.begin();
    RTC.begin();
    
    // Set SQW/Out signal frequency to 1 Hz.
    RTC.setSqwOutSignal(RTC_DS1307::Frequency_1Hz);
}

void loop () {
    // Nothing to do.
}

