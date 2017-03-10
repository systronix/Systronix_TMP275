
/** ---------- TMP275_Test3 Example ------------------------

Copyright 2017 Systronix Inc www.systronix.com

I2C notes
See notes in Systronix_TMP275 example

To read three bytes, do a requestFrom(address, 2, true). << insists the first param is int.  
Compiler has major whines if called as shown in the online Wire reference.

**/
 
/** ---------- REVISIONS ----------

2017Mar09 bboyes  Start, based on TMP275_Test example
--------------------------------**/

#include <Arduino.h>
#include <Systronix_TMP275.h>	// best version of I2C library is #included by the library. Don't include it here!


 /**
 * debug level
 * 0 = quiet, suppress everything
 * 3 = max, even more or less trivial message are emitted
 * 4 = emit debug info which checks very basic data conversion, etc
 */
 byte DEBUG = 0;

uint16_t rawtemp1, rawtemp2, rawtemp3;
uint16_t faketemp;

uint16_t dtime;  // delay in loop

uint8_t configOptions;

boolean fake;    // if true use simulated temperature data

float temperature = 0.0;
  uint16_t raw16 = 0;  // place to put what we just read 16 bits
  uint8_t raw8 = 0;

Systronix_TMP275 tmp275_48(TMP275_SLAVE_ADDR_0);    
Systronix_TMP275 tmp275_49(TMP275_SLAVE_ADDR_1);   
Systronix_TMP275 tmp275_50(TMP275_SLAVE_ADDR_2);  

/* ========== SETUP ========== */
void setup(void) 
{


//  uint16_t wrt16=0;  // temp write variable
  int8_t stat = -1;
//  int16_t temp_int16 = 0;
  
  Serial.begin(115200);     // use max baud rate
  // Teensy3 doesn't reset with Serial Monitor as do Teensy2/++2, or wait for Serial Monitor window
  // Wait here for 10 seconds to see if we will use Serial Monitor, so output is not lost
  while((!Serial) && (millis()<10000));    // wait until serial monitor is open or timeout, which seems to fall through
 
  Serial.printf("TMP275 Library Test Code at 0x%x and 0x%x\r\n", tmp275_48.base_address(), tmp275_49.base_address());
   
//  int8_t flag = -1;  // I2C returns 0 if no error
  
  // Teensy++2 PE7/INT7/AIN1, pin 32 on module.
  // Teensy3 Digital 3, module pin 4
  // make it input so we can use it for ALERT
  pinMode(3, INPUT_PULLUP);  
  
  // start TMP275 library
  tmp275_48.begin();
  tmp275_49.begin();
  tmp275_50.begin();

  configOptions = 0x0;  // 
  configOptions |= TMP275_CFG_RES12;

  // initialize sensor
	stat = tmp275_48.init(configOptions);
  Serial.printf(" write CFG: %X\r\n", configOptions); 

  if (SUCCESS != stat) Serial.print (" config init error! ");
  stat = tmp275_48.configRead (&raw8);
  if (SUCCESS != stat) Serial.print (" config read error! ");

  Serial.printf(" read CFG: %X\r\n", raw8); 

  if ( tmp275_48.base_clipped() )
    Serial.printf("base address out of range, clipped to 0x%x", tmp275_48.base_address());

  if ( tmp275_48.exists() )
    Serial.printf("Sensor exists at 0x%x\r\n", tmp275_48.base_address());
  else 
    Serial.printf("Sensor does not exist!\r\n");

    // initialize sensor 2
  stat = tmp275_49.init(configOptions);
  Serial.printf(" write CFG: %X\r\n", configOptions); 

  if (SUCCESS != stat) Serial.print (" config init error! ");
  stat = tmp275_49.configRead (&raw8);
  if (SUCCESS != stat) Serial.print (" config read error! ");

  Serial.printf(" read CFG: %X\r\n", raw8); 

  if ( tmp275_49.base_clipped() )
    Serial.printf("base address out of range, clipped to 0x%x", tmp275_49.base_address());

  if ( tmp275_49.exists() )
    Serial.printf("Sensor exists at 0x%x\r\n", tmp275_49.base_address());
  else 
    Serial.printf("Sensor does not exist!\r\n");

    // initialize sensor 3
  stat = tmp275_50.init(configOptions);
  Serial.printf(" write CFG: %X\r\n", configOptions); 

  if (SUCCESS != stat) Serial.print (" config init error! ");
  stat = tmp275_50.configRead (&raw8);
  if (SUCCESS != stat) Serial.print (" config read error! ");

  Serial.printf(" read CFG: %X\r\n", raw8); 

  if ( tmp275_50.base_clipped() )
    Serial.printf("base address out of range, clipped to 0x%x", tmp275_50.base_address());

  if ( tmp275_50.exists() )
    Serial.printf("Sensor exists at 0x%x\r\n", tmp275_50.base_address());
  else 
    Serial.printf("Sensor does not exist!\r\n");  
  
  delay(250);    // 220 msec for conversion
  
  stat = tmp275_48.pointerWrite(TMP275_TLOW_REG_PTR);
  stat = tmp275_48.register16Read (&raw16);
  Serial.print("Tlo:");
  Serial.print(raw16, HEX);
  Serial.print(" ");
  
  stat = tmp275_48.pointerWrite(TMP275_THIGH_REG_PTR);
  stat = tmp275_48.register16Read (&raw16);
  Serial.print("Thi:");
  Serial.print(raw16, HEX);
  Serial.print(" ");
  
  // leave the pointer set to read temperature
  stat = tmp275_48.pointerWrite(TMP275_TEMP_REG_PTR);
  if (SUCCESS != stat) Serial.println ("Sensor 1 temp reg error! ");

  stat = tmp275_49.pointerWrite(TMP275_TEMP_REG_PTR);
  if (SUCCESS != stat) Serial.println ("Sensor 2 temp reg error! ");

  stat = tmp275_50.pointerWrite(TMP275_TEMP_REG_PTR);
  if (SUCCESS != stat) Serial.println ("Sensor 3 temp reg error! ");

  
//  // fake temp  
//  fake = true;
//  dtime = 0;  // fast loop
//  faketemp = (uint16_t)0x7FF;  // max 12-bit value in raw 16-bit format
  
  // use real temperature data
  fake = false;      // switch to real data after full cycle of simulated
  dtime = 1000;      // msec between samples, 1000 = 1 sec, 60,000 = 1 minute
  Serial.print(" Interval is ");
  Serial.print(dtime/1000);
  Serial.print(" sec, ");
 
  Serial.println("Setup Complete!");
  Serial.println(" "); 
  
  if (1 == DEBUG)
  {
    Serial.println("sec deg C");
  }
}


uint16_t good=0;
uint16_t bad=0;


/* ========== LOOP ========== */
void loop(void) 
{
//  int16_t temp0;
  int8_t stat=-1;  // status flag
  float temp, temp2, temp3;
  
  Serial.print("@");
  Serial.print(millis()/1000);
  Serial.print(" ");
  

  stat = tmp275_48.register16Read (&rawtemp1);
  temp = tmp275_48.raw12_to_c(rawtemp1);
  if (stat != SUCCESS) Serial.printf("Sensor 1 error, stat=%u\r\n", stat);

  delay(1); // separate reads on scope

  stat = tmp275_49.register16Read (&rawtemp2);
  temp2 = tmp275_49.raw12_to_c(rawtemp2);
  if (stat != SUCCESS) Serial.printf("Sensor 2 error, stat=%u\r\n", stat);

  stat = tmp275_50.register16Read (&rawtemp3);
  temp3 = tmp275_50.raw12_to_c(rawtemp3);
  if (stat != SUCCESS) Serial.printf("Sensor 3 error, stat=%u\r\n", stat);  

  temperature = temp;  // for Ethernet client
  

  
 Serial.print (temp, 4);       // 2 dec pts good enough for real data 0.0625 deg C per count
 Serial.print (" ");
 Serial.print (temp2, 4);
 Serial.print (" ");
 Serial.print (temp3, 4);

  Serial.print (" C ");
  
  Serial.println();
  
  
  delay(dtime);
}



/**
Read the most current temperature already converted and present in the TMP275 temperature registers

In continuous mode, this could be one sample interval old
In one shot mode this data is from the last-requested one shot conversion
**/
uint8_t readTempDegC (float *tempC) 
{
    return 1;
}



/**
Convert deg C float to a raw 13-bit temp value in TMP275 format.
This is needed for Th and Tl registers as thermostat setpoint values

return 0 if OK, error codes if float is outside range of TMP275
**/
int8_t degCToRaw13 (uint16_t *raw13, float *tempC)
{
    return 1;
}



/**
Trigger a one-shot temperature conversion, wait for the new value, about 26 msec, and update 
the variable passed.

If the TMP275 is in continuous conversion mode, this places the part in One Shot mode, 
triggers the conversion, waits for the result, updates the variable, and leaves the TMP275 in one shot mode.

returns 0 if no error
**/
uint8_t getOneShotDegC (float *tempC)
{
    return 1;
}


/**
Set the TMP275 mode to one-shot, with low power sleep in between

mode: set to One Shot if true. 
If false, sets to continuous sampling mode at whatever sample rate was last set.

returns: 0 if successful
**/
int8_t setModeOneShot (boolean mode)
{
    return 1;
}

/**
Set TMP275 mode to continuous sampling at the rate given.

rate: must be one of the manifest constants such as TMP275_CFG_RATE_1HZ
if rate is not one of the four supported, it is set to the default 4 Hz

returns: 0 if successful
**/
int8_t setModeContinuous (int8_t rate)
{
    return 1;
}





