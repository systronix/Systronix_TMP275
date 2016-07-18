/*
 * Systronix_TMP102.cpp
 *
 *  Created on: Nov 15, 2013
 *      Author: BAB
 */

#include <Systronix_TMP102.h>	

 byte _DEBUG = 1;

/**************************************************************************/
/*!
    @brief  Instantiates a new TMP102 class to use the given base address
	@todo	Test base address for legal range 0x48-0x4B
			Add constructor(void) for default address of 0x48
*/
/**************************************************************************/
Systronix_TMP102::Systronix_TMP102(uint8_t base)
{
	_base = base;
	BaseAddr = base;

  struct data _data;			// instance of the data struct - not used
  _data.address = _base;		// struct - not used
}

/**************************************************************************/
/*!
    @brief  Join the I2C bus as a master
*/
/**************************************************************************/
void Systronix_TMP102::begin(void) {
	// 
	Wire.begin();	// join I2C as master
}

/**************************************************************************/
/*!
    @brief  Convert raw 13-bit temperature to float deg C
			handles neg and positive values specific to TMP102 extended mode 
			13-bit temperature data

	@TODO instead pass a pointer to the float variable? and return error if value out of bounds
*/
/**************************************************************************/
float Systronix_TMP102::raw13ToC(uint16_t raw13)
{
	float degC;

	// 13-bit temp mode
	if (raw13 & 0x8000)			// temp is neg, we must take 2's complement
	{
	//Serial.print (" (neg temp) ");
	raw13 = ~raw13;				// complement each bit
	raw13++;					// add one
	raw13 = raw13>>3;			// we want ms 13 bits
	degC = -0.0625 * (raw13); 	// make result negative
	}
	else
	{
	raw13 = raw13>>3;			// we want ms 13 bits right-justified
	degC = 0.0625 * (raw13);	// make result negative
	}
	return degC;
}


//---------------------------< R A W 1 3 _ T O _ F >----------------------------------------------------------
//
// Convert raw 13-bit TMP102 temperature to degrees Fahrenheit.
//

float Systronix_TMP102::raw13_to_F (uint16_t raw13)
	{
	return (raw13ToC (raw13) * 1.8) + 32.0;
	}


/**
Write to a TMP102 register
Start with slave address, as in any I2C transaction.
Next byte must be the Pointer Register value, in 2 lsbs
If all you want to do is set the Pointer Register for subsequent read(s)
then this 2-byte write cycle is complete.

Data bytes in the write are optional but must always follow the Pointer Register write byte.
The last value written to the Pointer Register persists until changed.

**/

int8_t Systronix_TMP102::writePointer (uint8_t pointer)
{
 
  if (_DEBUG >=4)
  {
    Serial.print ("adr=0x");
    Serial.print(BaseAddr, HEX);
    Serial.print(" ");
    Serial.print("ptr=0x");
    Serial.print(pointer, HEX);
    Serial.print(" ");
  }

//  uint8_t b;  // temp variable - not used
//  uint16_t ui;  // temp variable - not used
  int8_t flag=-1;  // signed flag, init to error result so we know if it has changed
  byte written;  // number of bytes written

  Wire.beginTransmission(BaseAddr);  // base address
  written = Wire.write(pointer);        // pointer in 2 lsb

  flag = Wire.endTransmission();    // flag is zero if no error
  
  
  if (_DEBUG >=4)
  {
    Serial.print ("wrote:");
    Serial.print (written);
    Serial.print (" ");
  
    if (0 == flag)
    {
      Serial.print("TMP102 WrtP OK=");
      Serial.print((int)flag);
    }  
    else
    {
      Serial.print("Error=");
      Serial.print((int)flag);
    }
    Serial.print (" ");
  }
  
  return (flag);  // zero if no error
}

/**

Param pointer is the TMP102 register into which to write the data
data is the 16 bits to write.
returns 0 if no error, positive values for NAK errors
**/
int8_t Systronix_TMP102::writeRegister (uint8_t pointer, uint16_t data)
{
  if (_DEBUG >=4)
  {
    Serial.print("ptr=0x");
    Serial.print(pointer, HEX);
    Serial.print(" dat=0x");
    Serial.print(data, HEX);
    Serial.print(" ");
  }

  uint8_t ub;  // temp variable
//  uint16_t ui;  // temp variable - not used
  int8_t flag=-1;  // signed flag, init to error result so we know if it has changed
  byte written;  // number of bytes written

  Wire.beginTransmission(BaseAddr);  // base address
  written = Wire.write(pointer);        // pointer in 2 lsb
  
  ub = data >> 8;    // put MSB of data into lower byte
  written += Wire.write(ub);    // write MSB of data

  if (_DEBUG >=4)
  {
    Serial.print("MSB=0x");
    Serial.print(ub, HEX);
    Serial.print(" ");
  }
  
  ub = data & 0x00FF;  // mask off upper byte of data ??? Is this necessary?
  written += Wire.write(ub);      // write LSB of data
  
  if (_DEBUG >=4)
  {
    Serial.print("LSB=0x");
    Serial.print(ub, HEX);
    Serial.print(" ");
  }
    
  
  flag = Wire.endTransmission();    // flag is zero if no error
  if (0 != flag)
  {
    Serial.print("Error TMP102 wrtReg flag=");
    Serial.println((int)flag);
  }
  
  if (_DEBUG >=4)
  {
    Serial.print ("wrote:");
    Serial.print (written);
    Serial.print (" ");
    
    if (0 == flag)
    {
      Serial.print("TMP102 WrtR OK=");
      Serial.print((int)flag);
    }  
    else
    {
      Serial.print("Error=");
      Serial.print((int)flag);
    }
    Serial.print (" ");
  }
  
  return (flag);  // zero if no error
}

/**
  Read the 16-bit register addressed by the current pointer value, store the data at the location passed
  
  return 0 if no error, positive bytes read otherwise.
*/
int8_t Systronix_TMP102::readRegister (uint16_t *data)
{
  uint8_t ub1;  // temp variable
  uint8_t ub2;
  uint16_t ui=0;  // temp variable
  int8_t flag=-1;  // signed flag, init to error result so we know if it has changed
  int avail1=0;
  int avail2=0;
//  
  // 
  // Wire.beginTransmission(BaseAddr);  // base address
  Wire.requestFrom(BaseAddr, (uint8_t)2, (uint8_t)true);
  avail1 = Wire.available();
  flag = avail1;
  ub1 = Wire.read();   // read MSB
  ub2 = Wire.read();  // LSB
//  flag = Wire.endTransmission();    // flag is zero if no error
  
  avail2 = Wire.available();
  if (_DEBUG >=4)
  {
    Serial.print("avail-1/2:");
    Serial.print(avail1);
    Serial.print("/");
    Serial.print(avail2);
    Serial.print(" ");
    
    
    Serial.print("ms=0x");
    Serial.print(ub1, HEX);
    Serial.print(" ");
  }
  
  ui = ub1<<8;      // save read byte in upper byte of 16-bit temp
  
  if (_DEBUG >=4)
  {
    Serial.print("ls=0x");
    Serial.print(ub2, HEX);
    Serial.print(" ");
  }
  
  ui |= ub2;        // OR in the byte read into the low byte of 16-bit temp
  if (_DEBUG >=4)
  {
    Serial.print("dat=0x");
    Serial.print(ui, HEX);
    Serial.print(" ");
  }

//  // requuestFrom does not return an error
//  if (0 != flag)
//  {
//    Serial.print("Error-rdReg=");
//    Serial.println((int)flag);
//  }
//  
  *data = ui;     // copy read value into data location
  
  return (flag);  // zero if no error
}

/**
Read the most current temperature already converted and present in the TMP102 temperature registers

In continuous mode, this could be one sample interval old
In one shot mode this data is from the last-requested one shot conversion
**/
int8_t Systronix_TMP102::readTempDegC (float *tempC) 
{
    return 1;
}



/**
Convert deg C float to a raw 13-bit temp value in TMP102 format.
This is needed for Th and Tl registers as thermostat setpoint values

return 0 if OK, error codes if float is outside range of TMP102
**/
int8_t Systronix_TMP102::degCToRaw13 (uint16_t *raw13, float *tempC)
{
    return 1;
}



/**
Trigger a one-shot temperature conversion, wait for the new value, about 26 msec, and update 
the variable passed.

If the TMP102 is in continuous conversion mode, this places the part in One Shot mode, 
triggers the conversion, waits for the result, updates the variable, and leaves the TMP102 in one shot mode.

returns 0 if no error
**/
int8_t Systronix_TMP102::getOneShotDegC (float *tempC)
{
    return 1;
}


/**
Set the TMP102 mode to one-shot, with low power sleep in between

mode: set to One Shot if true. 
If false, sets to continuous sampling mode at whatever sample rate was last set.

returns: 0 if successful
**/
int8_t Systronix_TMP102::setModeOneShot (boolean mode)
{
    return 1;
}

/**
Set TMP102 mode to continuous sampling at the rate given.

rate: must be one of the manifest constants such as TMP102_CFG_RATE_1HZ
if rate is not one of the four supported, it is set to the default 4 Hz

returns: 0 if successful
**/
int8_t Systronix_TMP102::setModeContinuous (int8_t rate)
{
    return 1;
}
