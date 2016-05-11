This is a fork of JeeLab's fantastic real time clock library for Arduino.

For details on using this library with an RTC module like the DS1307, see the guide at: https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/overview

To download. click the DOWNLOADS button to the right, and rename the uncompressed folder RTClib.

Place the RTClib folder in your *arduinosketchfolder*/libraries/ folder. 
You may need to create the libraries subfolder if its your first library. Restart the IDE.

<!-- START COMPATIBILITY TABLE -->

## Compatibility

MCU               | Tested Works | Doesn't Work | Not Tested  | Notes
----------------- | :----------: | :----------: | :---------: | -----
Atmega328 @ 16MHz |      X       |             |            | 
Atmega328 @ 12MHz |      X       |             |            | 
Atmega32u4 @ 16MHz |      X       |             |            | Use SDA/SCL on pins D3 &amp; D2
Atmega32u4 @ 8MHz |      X       |             |            | Use SDA/SCL on pins D3 &amp; D2
ESP8266           |      X       |             |            | SDA/SCL default to pins 4 &amp; 5 but any two pins can be assigned as SDA/SCL using Wire.begin(SDA,SCL)
Atmega2560 @ 16MHz |      X       |             |            | Use SDA/SCL on Pins 20 &amp; 21
ATSAM3X8E         |      X       |             |            | Use SDA1 and SCL1
ATSAM21D          |      X       |             |            | 
ATtiny85 @ 16MHz  |      X       |             |            | 
ATtiny85 @ 8MHz   |      X       |             |            | 

  * ATmega328 @ 16MHz : Arduino UNO, Adafruit Pro Trinket 5V, Adafruit Metro 328, Adafruit Metro Mini
  * ATmega328 @ 12MHz : Adafruit Pro Trinket 3V
  * ATmega32u4 @ 16MHz : Arduino Leonardo, Arduino Micro, Arduino Yun, Teensy 2.0
  * ATmega32u4 @ 8MHz : Adafruit Flora, Bluefruit Micro
  * ESP8266 : Adafruit Huzzah
  * ATmega2560 @ 16MHz : Arduino Mega
  * ATSAM3X8E : Arduino Due
  * ATSAM21D : Arduino Zero, M0 Pro
  * ATtiny85 @ 16MHz : Adafruit Trinket 5V
  * ATtiny85 @ 8MHz : Adafruit Gemma, Arduino Gemma, Adafruit Trinket 3V

<!-- END COMPATIBILITY TABLE -->
