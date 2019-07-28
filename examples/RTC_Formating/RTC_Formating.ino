#include <Wire.h>
#include <RTClib.h>

RTC_DS1307 rtc; 


void setup() {
  Serial.begin(57600);
  Wire.begin();
  rtc.begin();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
}

void loop() {
    DateTime now = rtc.now();
    char buf[100];
    strncpy(buf,"DD.MM.YYYY  hh:mm:ss\0",100);
    Serial.println(now.format(buf));
    strncpy(buf,"YYMMDD\0",100);
    Serial.println(now.format(buf));
    strncpy(buf,"YY.MM.DD\0",100);
    Serial.println(now.format(buf));
    delay(1000);
}
