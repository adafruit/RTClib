// Timestamp functions using a DS1307 RTC connected via I2C and Wire lib

#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

void setup() {
   Serial.begin(57600);
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

}

void loop() {
 DateTime now = rtc.now();
 
 //Full Timestamp
 Serial.print("DateTime::TIMESTAMP_FULL:\t");
 Serial.println(now.timestamp(DateTime::TIMESTAMP_FULL));
 
 //Date Only
 Serial.print("DateTime::TIMESTAMP_DATE:\t");
 Serial.println(now.timestamp(DateTime::TIMESTAMP_DATE));
 
 //Full Timestamp
 Serial.print("DateTime::TIMESTAMP_TIME:\t");
 Serial.println(now.timestamp(DateTime::TIMESTAMP_TIME));
 
 Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
 
 //Delay 5s
 delay(5000);
}
