RTClib
======

A fork of Jeelab's fantastic RTC library

This version also partially supports the alarm functionality in the DS1337 (and theoretically in a DS3231).
I've tested this using a DS1337 and an Arduino MEGA.

Things of note:

* The isRunning check does nothing on a DS1337, as it does not have the CH bit.
