// Battery backed memory functions using a DS1307 RTC connected via I2C.
//
// According to the data sheet (http://datasheets.maxim-ic.com/en/ds/DS1307.pdf), the
// DS1307 has 56 bytes of general purpose RAM with unlimited writes.  The RAM registers
// are located at 0x08 - 0x3F.  During read/write, if the data would go past 0x3F, it
// will wrap around to 0x00 and read/write to the clock registers.
//
// The readMemory and writeMemory functions assume the caller is intending to write into
// the RAM above the clock registers and so 8 is added to the passed in offset.
//
// This sketch writes a character array and a long into the memory of the DS1307 during
// setup and then reads them back on each iteration of the loop. The sketch demonstrates
// how the RAM could be used to store the local UTC offset once and then used on each read
// to adjust the local results of calling unixtime to UTC unixtime.
//
// To see the battery backed feature in action:
//   1. Upload the sketch as-is
//   2. Comment out the section in setup that writes to memory and upload the sketch again
//   3. You can now disconnect power, reconnect power and see the values being read and printed 

#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 RTC;

void setup () {
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  // Write a char array
  char message[12] = "Hello World";
  RTC.writeMemory(0, (uint8_t *)message, 11);
  Serial.print("storing message to memory : ");
  Serial.println(message);
  
  // Write a long
  uint8_t buffer[4];
  long utc_offset = (-5 * 60 * 60); // UTC-5
  buffer[0] = (utc_offset >> 24) & 0x000000FF;
  buffer[1] = (utc_offset >> 16) & 0x000000FF;
  buffer[2] = (utc_offset >> 8) & 0x000000FF;
  buffer[3] = utc_offset & 0x000000FF;
  RTC.writeMemory(12, buffer, 4);
  Serial.print("storing long (UTC offset) to memory : ");
  Serial.println(utc_offset, DEC);
}

void loop () {
  // Read a char array
  char message[12];
  RTC.readMemory(0x00, (uint8_t *)message, 11);
  message[11] = '\0';
  Serial.print("reading message from memory : ");
  Serial.println(message);
  
  // Read a long
  uint8_t buffer[4];
  RTC.readMemory(12, buffer, 4);
  long utc_offset = ((long)buffer[0] << 24) + ((long)buffer[1] << 16) + ((long)buffer[2] << 8) + ((long)buffer[3]);
  Serial.print("reading long (UTC offset) from memory : ");
  Serial.println(utc_offset, DEC);

  // Print the unaltered unixtime
  DateTime now = RTC.now();  
  Serial.print("time since midnight 1/1/1970 = ");
  Serial.println(now.unixtime());

  // Print the unixtime less the UTC offset
  Serial.print("time since midnight 1/1/1970 (adjusted to UTC) = ");
  Serial.println(now.unixtime() - utc_offset);
  
  delay(3000);
}
