// Example of using the non-volatile RAM storage on the DS1307.
// You can write up to 56 bytes from address 0 to 55.
// Data will be persisted as long as the DS1307 has battery power.

#include <Wire.h>
#include "RTClib.h"

#if defined(ARDUINO_ARCH_SAMD)  // for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
#define Serial SerialUSB
#endif

RTC_DS1307 rtc;

void printnvram(uint8_t address) {
  Serial.print("Address 0x");
  Serial.print(address, HEX);
  Serial.print(" = 0x");
  Serial.println(rtc.readnvram(address), HEX); 
}

void setup () {
#ifdef ESP8266
  Wire.pins(2, 14);   // ESP8266 can use any two pins, such as SDA to #2 and SCL to #14
#endif

#ifndef ESP8266
  while (!Serial); // for Leonardo/Micro/Zero
#endif
  Serial.begin(57600);
  rtc.begin();

  // Print old RAM contents on startup.
  Serial.println("Current NVRAM values:");
  for (int i = 0; i < 6; ++i) {
     printnvram(i);
  }

  // Write some bytes to non-volatile RAM storage.
  // NOTE: You can only read and write from addresses 0 to 55 (i.e. 56 byte values).
  Serial.println("Writing NVRAM values.");
  // Example writing one byte at a time:
  rtc.writenvram(0, 0xFE);
  rtc.writenvram(1, 0xED);
  // Example writing multiple bytes:
  uint8_t writeData[4] = { 0xBE, 0xEF, 0x01, 0x02 };
  rtc.writenvram(2, writeData, 4);
  
  // Read bytes from non-volatile RAM storage.
  Serial.println("Reading NVRAM values:");
  // Example reading one byte at a time.
  Serial.println(rtc.readnvram(0), HEX);
  Serial.println(rtc.readnvram(1), HEX);
  // Example reading multiple bytes:
  uint8_t readData[4] = {0};
  rtc.readnvram(readData, 4, 2);
  Serial.println(readData[0], HEX);
  Serial.println(readData[1], HEX);
  Serial.println(readData[2], HEX);
  Serial.println(readData[3], HEX);
  
}

void loop () {
  // Do nothing in the loop.
}
