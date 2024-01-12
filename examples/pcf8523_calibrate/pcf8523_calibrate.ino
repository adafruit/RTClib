// Date and time functions using a PCF8523 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

void setup() {
  Serial.begin(57600);
  delay(500);
  while (!Serial)
    ;  // wait for serial port to connect. Needed for native USB

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
  rtc.start();

  // The PCF8523 can be calibrated for:
  //        - Aging adjustment
  //        - Temperature compensation
  //        - Accuracy tuning
  // The offset mode to use, once every two hours or once every minute.
  // The offset Offset value from -64 to +63. See the Application Note for calculation of offset values.
  // https://www.nxp.com/docs/en/application-note/AN11247.pdf
  // The deviation in parts per million can be calculated over a period of observation. Both the drift (which can be negative)
  // and the observation period must be in seconds. For accuracy the variation should be observed over about 1 week.
  // Note: any previous calibration should cancelled prior to any new observation period.
  // Recommendation: Syncronise host PC time.
  // run this sketch cancelling any previous calibration,
  // record the output including timestamp,
  // run sketch again after several days,
  // calculate period of observation in seconds, and drift in seconds.
  // Run sketch with the calculated figures and uncomment rtc.calibrate line as required.
  // Example - RTC gaining 43 seconds in 1 week
  float drift = 43;                                      // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
  float period_sec = (7 * 86400);                        // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
  float deviation_ppm = (drift / period_sec * 1000000);  //  deviation in parts per million (μs)
  float drift_unit = 4.34;                               // use with offset mode PCF8523_TwoHours
  // float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
  int8_t offset = round(deviation_ppm / drift_unit);
  // rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct
  // rtc.calibrate(PCF8523_OneMinute, offset); // // Un-comment to perform calibration with offset mode PCF8523_OneMinute
  // rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration

  Serial.println();
  Serial.print("Calculated Offset for calibration is: ");
  Serial.println(offset);  // Print to control calculated offset

  // read offset register  *******************************
  Serial.println("Read RTC PCF8523 Offset Register");  // Print to control offset

  // Method 1 ****************************
  int8_t OffsetReg = rtc.readOffsetReg();
  Serial.print("Offset mode is: ");
  if bitRead (OffsetReg, 7) {
    Serial.println("PCF8523_OneMinute");
  } else {
    Serial.println("PCF8523_TwoHours ");
  }
  offset = OffsetReg;
  bitWrite(offset, 7, bitRead(OffsetReg, 6));
  Serial.print("Offset is: ");
  Serial.println(offset);  // Print to control offset

  // Method 2 ****************************
  String OffsetMode = String(rtc.getOffsetMode());
  Serial.print("Offset mode is: ");
  Serial.println(OffsetMode);

  offset = rtc.getOffset();
  Serial.print("Offset is: ");
  Serial.println(offset);  // Print to control offset
                           // End read offset register  *******************************

  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void loop() {

  // do nothing
}
