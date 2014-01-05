// Date and time functions using a DS1307 RTC connected via I2C and Wire lib

#include <Wire.h>
#include "RTClib.h"

#define ALARM1_PIN 3
#define ALARM2_PIN 2

RTC_DS1337 rtc;

void setup () {
  pinMode(ALARM1_PIN, INPUT);
  pinMode(ALARM2_PIN, INPUT);
  
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
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  
  DateTime now = rtc.now();
  
  DateTime alarm1time (now.unixtime() + 30);
  rtc.setAlarm1Time(alarm1time);
  Serial.print("Alarm 1 set for ");
  Serial.print(alarm1time.hour(), DEC);
  Serial.print(':');
  Serial.print(alarm1time.minute(), DEC);
  Serial.print(':');
  Serial.print(alarm1time.second(), DEC);
  Serial.println();
  
  DateTime alarm2time (now.unixtime() + 65);
  rtc.setAlarm2Time(alarm2time);
  Serial.print("Alarm 2 set for ");
  Serial.print(alarm2time.hour(), DEC);
  Serial.print(':');
  Serial.print(alarm2time.minute(), DEC);
  Serial.println();
  
  Serial.println();
}

void loop () {
    boolean alarm1 = (digitalRead(ALARM1_PIN) == LOW);
    boolean alarm2 = (digitalRead(ALARM2_PIN) == LOW);
    DateTime now = rtc.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
    
    // calculate a date which is 7 days and 30 seconds into the future
    DateTime future (now.unixtime() + 7 * 86400L + 30);
    
    Serial.print(" now + 7d + 30s: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();
    
    if (alarm1)
    {
        Serial.println("Alarm 1 is set.");
        rtc.clearAlarm1Flag();
    }
    
    if (alarm2)
    {
        Serial.println("Alarm 2 is set.");
        rtc.clearAlarm2Flag();
    }
    
    Serial.println();
    delay(1000);
}
