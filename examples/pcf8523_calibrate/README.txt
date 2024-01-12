  The PCF8523 can be calibrated for:
         - Aging adjustment
         - Temperature compensation
         - Accuracy tuning
  The offset mode to use, once every two hours or once every minute.
  The offset Offset value from -64 to +63. A positive offset makes the clock slower.
  See the Application Note for calculation of offset values.
  https://www.nxp.com/docs/en/application-note/AN11247.pdf
  The deviation in parts per million can be calculated over a period of observation. Both the drift (which can be negative)
  and the observation period must be in seconds. For accuracy the variation should be observed over about 1 week.
  Note: any previous calibration should cancelled prior to any new observation period.
  Recommendation: 
	Syncronise host PC time.
  	run this sketch cancelling any previous calibration,
  	record the output including timestamp,
  	after several days again syncronise host PC time,
	run sketch again record the output including timestamp,
  	calculate period of observation in seconds, and drift in seconds.
  	Run sketch with the calculated figures and uncomment rtc.calibrate line as required.
  Example - RTC gaining 43 seconds in 1 week
  	float drift = 43;                                      // seconds plus or minus over oservation period - set to 0 to cancel previous calibration.
  	float period_sec = (7 * 86400);                        // total obsevation period in seconds (86400 = seconds in 1 day:  7 days = (7 * 86400) seconds )
  	float deviation_ppm = (drift / period_sec * 1000000);  //  deviation in parts per million (?s)
  	float drift_unit = 4.34;                               // use with offset mode PCF8523_TwoHours
  	// float drift_unit = 4.069; //For corrections every min the drift_unit is 4.069 ppm (use with offset mode PCF8523_OneMinute)
  	int8_t offset = round(deviation_ppm / drift_unit);
  	rtc.calibrate(PCF8523_TwoHours, offset); // Un-comment to perform calibration once drift (seconds) and observation period (seconds) are correct
  	// rtc.calibrate(PCF8523_OneMinute, offset); // // Un-comment to perform calibration with offset mode PCF8523_OneMinute
  	// rtc.calibrate(PCF8523_TwoHours, 0); // Un-comment to cancel previous calibration

In order to provide a method of reading the offset register, which may contain an previous calibration
two methods are provided; 1. rtc.readOffsetReg(), or 2. rtc.getOffsetMode() and rtc.getOffset()
See the example sketch: pcf8523_calibrate.ino

Hint:
	Once the calibration Offset mode and Offset are known a line can be entered in the setup of the operating project sketch
	to re-establish the offset register after a battery replacement or clock reset. Note that your sketch will still require a method
	to insert the actual date and time.
	In the case of the above sample the line to insert in setup() would be:
  	rtc.calibrate(PCF8523_TwoHours, 16); // re-insert previously calculated calibration after clock reset. 