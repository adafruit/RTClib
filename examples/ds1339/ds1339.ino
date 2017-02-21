// Date and time functions using a DS1339 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"

#define SERIAL_PORT_SPEED   9600
#ifdef SERIAL_PORT_MONITOR
#define mySerial  SERIAL_PORT_MONITOR
#else
#define mySerial  Serial
#endif

RTC_DS1339 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup () {

#ifndef ESP8266
  while (!mySerial); // for Leonardo/Micro/Zero
#endif

  mySerial.begin(9600);

  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    mySerial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    mySerial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void loop () {
    DateTime now = rtc.now();

    mySerial.print(now.year(), DEC);
    mySerial.print('/');
    mySerial.print(now.month(), DEC);
    mySerial.print('/');
    mySerial.print(now.day(), DEC);
    mySerial.print(" (");
    mySerial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    mySerial.print(") ");
    mySerial.print(now.hour(), DEC);
    mySerial.print(':');
    mySerial.print(now.minute(), DEC);
    mySerial.print(':');
    mySerial.print(now.second(), DEC);
    mySerial.println();

    mySerial.print(" since midnight 1/1/1970 = ");
    mySerial.print(now.unixtime());
    mySerial.print("s = ");
    mySerial.print(now.unixtime() / 86400L);
    mySerial.println("d");

    // calculate a date which is 7 days and 30 seconds into the future
    DateTime future (now + TimeSpan(7,12,30,6));

    mySerial.print(" now + 7d + 12h + 30m + 6s: ");
    mySerial.print(future.year(), DEC);
    mySerial.print('/');
    mySerial.print(future.month(), DEC);
    mySerial.print('/');
    mySerial.print(future.day(), DEC);
    mySerial.print(' ');
    mySerial.print(future.hour(), DEC);
    mySerial.print(':');
    mySerial.print(future.minute(), DEC);
    mySerial.print(':');
    mySerial.print(future.second(), DEC);
    mySerial.println();

    mySerial.println();
    delay(3000);
}
