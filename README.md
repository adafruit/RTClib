# RTClib [![Build Status](https://github.com/adafruit/RTClib/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/RTClib/actions)[![Documentation](https://github.com/adafruit/ci-arduino/blob/master/assets/doxygen_badge.svg)](http://adafruit.github.io/RTClib/html/index.html)

This is a fork of JeeLab's fantastic real time clock library for Arduino.

Works great with Adafruit RTC breakouts:

- [DS3231 Precision RTC](https://www.adafruit.com/product/3013)
- [PCF8523 RTC](https://www.adafruit.com/product/3295)
- [DS1307 RTC](https://www.adafruit.com/product/3296)

Please note that dayOfTheWeek() ranges from 0 to 6 inclusive with 0 being 'Sunday'.

<!-- START COMPATIBILITY TABLE -->

## Compatibility

MCU                | Tested Works | Doesn't Work | Not Tested  | Notes
------------------ | :----------: | :----------: | :---------: | -----
Atmega328 @ 16MHz  |      X       |             |            |
Atmega328 @ 12MHz  |      X       |             |            |
Atmega32u4 @ 16MHz |      X       |             |            | Use SDA/SCL on pins D3 &amp; D2
Atmega32u4 @ 8MHz  |      X       |             |            | Use SDA/SCL on pins D3 &amp; D2
ESP8266            |      X       |             |            | SDA/SCL default to pins 4 &amp; 5 but any two pins can be assigned as SDA/SCL using Wire.begin(SDA,SCL)
Atmega2560 @ 16MHz |      X       |             |            | Use SDA/SCL on Pins 20 &amp; 21
ATSAM3X8E          |      X       |             |            | Use SDA1 and SCL1
ATSAM21D           |      X       |             |            |
ATtiny85 @ 16MHz   |      X       |             |            |
ATtiny85 @ 8MHz    |      X       |             |            |
Intel Curie @ 32MHz |             |             |     X       |
STM32F2            |             |             |     X       |

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
Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

# Dependencies
 * [TinyWireM](https://github.com/adafruit/TinyWireM)

# Contributing

Contributions are welcome! Please read our [Code of Conduct](https://github.com/adafruit/RTClib/blob/master/CODE_OF_CONDUCT.md>)
before contributing to help this project stay welcoming.

## Documentation and doxygen
For the detailed API documentation, see https://adafruit.github.io/RTClib/html/index.html
Documentation is produced by doxygen. Contributions should include documentation for any new code added.

Some examples of how to use doxygen can be found in these guide pages:

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen-tips

Written by JeeLabs
MIT license, check license.txt for more information
All text above must be included in any redistribution

To install, use the Arduino Library Manager and search for "RTClib" and install the library.

### Pcf8523 Timers (Adalogger FeatherWing RTC+SD)
The Pcf8523's timers are similar to other timers found in microcontrollers and RTCs. A timer starts at some value, and counts down with some frequency. When the timer hits 0, its interrupt flag is set. If the interrupt signal is enabled, the flag drives an interrupt pin.

#### Timer Theory of Operation
This RTC has three variable timers: countdown timer A, watchdog timer A, and countdown timer B. Two of these can be run at a time.

##### Timers and Timing
A timer counts down from a value (1-255), at a counting frequency (see the enumeration PCF8523TimerClockFreq). Altogether, a timer counts out the period P = (value / frequency) from the time it is started. This means that the RTC can time an operation from a few hundred microseconds (value=1 at frequency=4kHz, yields 244 microseconds) to several days (value=255 at frequency=1/3600Hz or 1 hour, yields 10.625 days).

Countdown timers A and B count down to 0, and then start over counting from their original value. Watchdog timer A counts down from some value, but does not begin counting again when zero is reached. WDT A is a mode of timer A, and cannot be run concurrently with countdown timer A. When a timer is disabled, its value is cleared rather than paused.

##### Timer Interrupts
Each timer has an interrupt; an interrupt has a flag and a signal. When a timer's value reaches 0, its interrupt flag is set. If its interrupt signal is enabled, timer A's flag drives the INT1 pin, and timer B's flag drives the INT1 and INT2 pins.

For countdown timers A and B, these pins remain set until the flag is cleared (by writing 0 into the flag), or until the signal is disabled.

Watchdog timer A's interrupt flag differs slightly: the flag set in the same way, but is automatically cleared with the next read of the interrupt register (control 2). Reading the timer state, the interrupt state, or writing the interrupt state will accomplish this register-clearing read as a side-effect.

The INT1 line is shared with the CLKOUT/square wave function. Only one function can be used, and enabling the interrupt signal will disable the square wave generation function.

#### Wiring and Configuring the Interrupt Lines

The INT1 and INT2 pins are open collectors, and transition from high-impedance to low-impedance to ground when any interrupt signal becomes active. When an INT pin is connected to a pulled-up GPIO pin, the GPIO pin's digital state will read low when the interrupt is active, and high otherwise.

#### Pinouts and Form-factors

INT1 is available on Adafruit's feather and standalone boards for the pcf, labeled as "INT" on the feather (https://www.adafruit.com/product/2922) and "SQW" on the standalone board (https://www.adafruit.com/product/3295). INT2 is available only in the TSSOP14 form-factor of the pcf.

This means that timer A and B's interrupt output are indistinguishable on a hardware level on Adafruit's boards.
