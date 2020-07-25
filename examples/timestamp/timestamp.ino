/* Timestamp functions using a DS1307 RTC connected via I2C and Wire lib
**
** Useful for file name
**		` SD.open(time.timestamp()+".log", FILE_WRITE) `
**
**
** Created: 2015-06-01 by AxelTB
** Last Edit:
*/

#include "RTClib.h"

RTC_DS1307 rtc;

void setup () {
  Serial.begin(57600);

#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
}

void loop() {
 DateTime time = rtc.now();

 // Create array to store timestamp.
 char buffer[20];

 // Full Timestamp
 time.timestamp(buffer, DateTime::TIMESTAMP_FULL);
 Serial.print("DateTime::TIMESTAMP_FULL:\t");
 Serial.println(buffer);

 // Date Only
 time.timestamp(buffer, DateTime::TIMESTAMP_DATE);
 Serial.print("DateTime::TIMESTAMP_DATE:\t");
 Serial.println(buffer);

 // Full Timestamp
 time.timestamp(buffer, DateTime::TIMESTAMP_TIME);
 Serial.print("DateTime::TIMESTAMP_TIME:\t");
 Serial.println(buffer);

 Serial.println("\n");

 //Delay 5s
 delay(5000);
}
