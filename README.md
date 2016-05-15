# Systronix_TMP102
Arduino library for the Texas Instruments precision I2C temperature sensor TMP102.
I could not find an existing library which supported the full range and resolution of the TMP102 so I wrote one. 
This library supports the full temperature range of the TMP102, from -40C to +125C, at full resolution.

## TMP102 Temperature Sensor Key Features
There are a ton of temperature sensors out there. This one has a great combination of features:
 - $1 in modest quantities, $.50 @1000
 - high accuracy (+/- 0.5 deg C)
 - 12-bit resolution (0.0625 deg C per bit)
 - wide temperature range (-40C to +125C)
 - operates 1.4V to 3.6 V supply, only uses 10 uA
 - alarm output
 - really tiny package so it can fit almost anywhere. It's a pain to solder by hand though.
 - Sparkun has a [breakout](https://www.sparkfun.com/products/11931) which I use and recommend
 - TMP102 [data sheet](http://www.ti.com/product/TMP102)
 
### Comments
 - This is a work in progress but the current code has been extensively tested and is reliable. 
 - Also included is an example/test program which verifies the conversion algorithm by feeding it simulated known data.
 - At the moment output is always in deg C.
 - See the source code for plenty of explanatory comments

### TODO
 - add doxygen docs
 - Support Alarm output, could be used as a simple thermostat 
