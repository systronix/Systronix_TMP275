# Systronix_TMP102
Arduino library for the Texas Instruments precision I2C temperature sensor TMP102
This library supports the full temperature range of the TMP102, from -40C to +125C, at full resolution.
This is a work in progress but the current code has been extensively tested and works. Also included is
an example/test program which verifies the conversion algorithm by feeding it simulated known data.
At the moment output is always in deg C.
See the source code for plenty of explanatory comments

###TODO
 - add doxygen docs
 - Support Alarm output, could be used as a simple thermostat 
