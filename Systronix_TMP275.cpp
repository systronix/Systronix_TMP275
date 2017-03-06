/*
 * Systronix_TMP275.cpp
 *
 *  Created on: Nov 15, 2013
 *	  Author: BAB
 */

#include <Systronix_TMP275.h>	

//---------------------------< CONSTRUCTOR >--------------------------------------------------------------------
/*!
	@brief  Instantiates a new TMP275 class to use the given base address
	base is clipped to min or max if necessary

*/

Systronix_TMP275::Systronix_TMP275(uint8_t base)
	{
	if (base < TMP_275_ADDR_MIN) 
		{
		base = TMP_275_ADDR_MIN;
		control.base_clipped = true;
		}
	else if (base > TMP_275_ADDR_MAX) 
		{
		base = TMP_275_ADDR_MAX;
		control.base_clipped = true;
		}
	else
		{
		_base = base;
		BaseAddr = base;
		control.base_clipped = false;
		}

	}

// default constructor
Systronix_TMP275::Systronix_TMP275()
	{
	_base = TMP_275_ADDR_MIN;
	BaseAddr = _base;
	}

// destructor
Systronix_TMP275::~Systronix_TMP275()
{
	// Anything to do here? Leave I2C as master? Set flag?
}


//---------------------------< B E G I N >--------------------------------------------------------------------
// One-time startup things here. Call this only at program start.
/*!
	@brief  Join the I2C bus as a master
*/

void Systronix_TMP275::begin(void)
	{
	Wire.begin();	// join I2C as master
	//	struct data _data;			// instance of the data struct - not used
	//	_data.address = _base;		// struct - not used
	}


//---------------------------< I N I T >----------------------------------------------------------------------
//
// Attempts to write the pointer register and the config register.  
// If successful, sets control.exists true, else false.
// This is the same as writeConfig TODO just call that from here

uint8_t Systronix_TMP275::init (uint8_t config)
	{
	uint8_t written;
	
	Wire.beginTransmission (_base);						// base address
	written = Wire.write (TMP275_CONF_REG_PTR);			// pointer in 2 lsb
	written += Wire.write (config);						// write configuration

	if (2 != written)
		{
		control.exists = false;							// unsuccessful i2c_t3 library call
		return FAIL;
		}
	
  	if (Wire.endTransmission())
		{
		control.exists = false;							// unsuccessful i2c transaction
		return FAIL;
		}
	
	control.exists = true;								// if here, we appear to have communicated with
	return SUCCESS;										// the sensor
	}


//---------------------------< T A L L Y _ E R R O R S >------------------------------------------------------
//
// Here we tally errors.  This does not answer the 'what to do in the event of these errors' question; it just
// counts them.  If the device does not ack the address portion of a transaction or if we get a timeout error,
// exists is set to false.  We assume here that the timeout error is really an indication that the automatic
// reset feature of the i2c_t3 library failed to reset the device in which case, the device no longer 'exists'
// for whatever reason.
//

void Systronix_TMP275::tally_errors (uint8_t error)
	{
	switch (error)
		{
		case 0:					// Wire.write failed to write all of the data to tx_buffer
			control.incomplete_write_count ++;
			break;
		case 1:					// data too long from endTransmission() (rx/tx buffers are 259 bytes - slave addr + 2 cmd bytes + 256 data)
		case 8:					// buffer overflow from call to status() (read - transaction never started)
			control.data_len_error_count ++;
			break;
		case 2:					// slave did not ack address (write)
		case 5:					// from call to status() (read)
			control.rcv_addr_nack_count ++;
			control.exists = false;
			break;
		case 3:					// slave did not ack data (write)
		case 6:					// from call to status() (read)
			control.rcv_data_nack_count ++;
			break;
		case 4:					// arbitration lost (write) or timeout (read/write) or auto-reset failed
		case 7:					// arbitration lost from call to status() (read)
			control.other_error_count ++;
			control.exists=false;
		}
	}


//---------------------------< R A W 1 2 T O C >--------------------------------------------------------------
/*!
	@brief  Convert raw 12-bit temperature to float deg C
			handles neg and positive values 

	@TODO instead pass a pointer to the float variable? and return error if value out of bounds
*/
//
// receives uint16_t raw13 argument that should be an int16_t.  But, because readRegister() is a general
// purpose function for reading all of the 16 bit registers we get around that by casting the uint16_t to
// int16_t before we use the value.
//
// TODO: take some time to consider if it is worthwhile to have a separate function that just fetches the
// temperature register.  Obvious downside (if it is a downside) is that such a function would necessarily need
// to set the pointer register every time the temperature is read.
//

float Systronix_TMP275::raw12_to_c (uint16_t raw12)
	{
	uint8_t		shift = 4;		// 
	return 0.0625 * ((int16_t)raw12 >> shift);
	}


//---------------------------< R A W 1 2 _ T O _ F >----------------------------------------------------------
//
// Convert raw 12-bit TMP275 temperature to degrees Fahrenheit.
//

float Systronix_TMP275::raw12_to_f (uint16_t raw12)
	{
	return (raw12_to_c (raw12) * 1.8) + 32.0;
	}


//---------------------------< G E T _ T E M P E R A T U R E _ D A T A >--------------------------------------
//
// Gets current temperature and fills the data struct with the various temperature info
//

uint8_t Systronix_TMP275::get_temperature_data (void)
	{
	
	if (_pointer_reg)									// if not pointed at temperature register
		if (pointerWrite (TMP275_TEMP_REG_PTR))			// attempt to point it
			return FAIL;								// attempt failed; quit
	
	if (register16Read (&data.raw_temp))				// attempt to read the temperature
		return FAIL;									// attempt failed; quit
	
	data.t_high = max((int16_t)data.raw_temp, (int16_t)data.t_high);	// keep track of min/max temperatures
	data.t_low = min((int16_t)data.t_low, (int16_t)data.raw_temp);

	data.deg_c = raw12_to_c (data.raw_temp);					// convert to human-readable forms
	data.deg_f = raw12_to_f (data.raw_temp);
	
	return SUCCESS;
	}


//---------------------------< P O I N T E R W R I T E >------------------------------------------------------
/**
Write to a TMP275 register
Start with slave address, as in any I2C transaction.
Next byte must be the Pointer Register value, in 2 lsbs
If all you want to do is set the Pointer Register for subsequent read(s)
then this 2-byte write cycle is complete.

Data bytes in the write are optional but must always follow the Pointer Register write byte.
The last value written to the Pointer Register persists until changed.

**/

uint8_t Systronix_TMP275::pointerWrite (uint8_t pointer)
	{
	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	_pointer_reg = pointer;								// keep a copy for use by other functions
	Wire.beginTransmission (_base);						// base address
	control.ret_val = Wire.write (_pointer_reg);		// pointer in 2 lsb
	if (1 != control.ret_val)
		{
		control.ret_val = 0;
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}

	control.ret_val = Wire.endTransmission();
  	if (SUCCESS == control.ret_val)
		return SUCCESS;
	tally_errors (control.ret_val);						// increment the appropriate counter
	return FAIL;										// calling function decides what to do with the error
	}

//---------------------------< C O N F I G W R I T E >----------------------------------------------------
/**
Write to the 8-bit config register
returns 0 if no error, positive values for NAK errors
**/

uint8_t Systronix_TMP275::configWrite (uint8_t data)
	{
	uint8_t written;									// number of bytes written

	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	Wire.beginTransmission (_base);						// base address
	written = Wire.write (TMP275_CONF_REG_PTR);			// pointer in 2 lsb
	_pointer_reg = TMP275_CONF_REG_PTR;
	written += Wire.write (data);						// write data

	if (3 != written)
		{
		control.ret_val = 0;
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}
	
  	if (SUCCESS == Wire.endTransmission())
		return SUCCESS;
	tally_errors (control.ret_val);						// increment the appropriate counter
	return FAIL;										// calling function decides what to do with the error
	}

//---------------------------< C O N F I G R E A D >------------------------------------------------------
/**
  Read the 8-bit config register 

  return 0 if no error, positive bytes read otherwise.
*/

uint8_t Systronix_TMP275::configRead (uint8_t *data)
	{
	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	if (_pointer_reg != TMP275_CONF_REG_PTR)			// if not pointing to config reg then do so
	{
		if (SUCCESS != pointerWrite(TMP275_CONF_REG_PTR))
			return FAIL;
	}

	if (1 != Wire.requestFrom(_base, 1, I2C_STOP))
		{
		control.ret_val = Wire.status();				// to get error value
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}

	*data = (uint8_t)Wire.read();
	return SUCCESS;
	}

//---------------------------< R E G I S T E R 1 6 W R I T E >----------------------------------------------------
/**
Param pointer is the TMP275 register into which to write the data
data is the 16 bits to write.
returns 0 if no error, positive values for NAK errors
**/

uint8_t Systronix_TMP275::register16Write (uint8_t pointer, uint16_t data)
	{
	uint8_t written;									// number of bytes written

	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	Wire.beginTransmission (_base);						// base address
	written = Wire.write (pointer);						// pointer in 2 lsb
	written += Wire.write ((uint8_t)(data >> 8));		// write MSB of data
	written += Wire.write ((uint8_t)(data & 0x00FF));	// write LSB of data

	if (3 != written)
		{
		control.ret_val = 0;
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}
	
  	if (SUCCESS == Wire.endTransmission())
		return SUCCESS;
	tally_errors (control.ret_val);						// increment the appropriate counter
	return FAIL;										// calling function decides what to do with the error
	}


//---------------------------< R E G I S T E R 1 6 R E A D >------------------------------------------------------
/**
  Read the 16-bit register addressed by the current pointer value, store the data at the location passed

  TODO: What if current pointer value is the 8-bit config register???
  
  return 0 if no error, positive bytes read otherwise.
*/

uint8_t Systronix_TMP275::register16Read (uint16_t *data)
	{
	if (!control.exists)								// exit immediately if device does not exist
		return ABSENT;

	if (2 != Wire.requestFrom(_base, 2, I2C_STOP))
		{
		control.ret_val = Wire.status();				// to get error value
		tally_errors (control.ret_val);					// increment the appropriate counter
		return FAIL;
		}

	*data = (uint16_t)Wire.read() << 8;
	*data |= (uint16_t)Wire.read();
	return SUCCESS;
	}


//---------------------------< R E A D T E M P D E G C >------------------------------------------------------
/**
Read the most current temperature already converted and present in the TMP275 temperature registers

In continuous mode, this could be one sample interval old
In one shot mode this data is from the last-requested one shot conversion
**/
uint8_t Systronix_TMP275::tempReadDegC (float *tempC) 
	{
	return FAIL;
	}



//---------------------------< D E G C T O R A W 1 2 >--------------------------------------------------------
/**
Convert deg C float to a raw 13-bit temp value in TMP275 format.
This is needed for Th and Tl registers as thermostat setpoint values

return 0 if OK, error codes if float is outside range of TMP275
**/

uint8_t Systronix_TMP275::degCToRaw12 (uint16_t *raw12, float *tempC)
	{
	return FAIL;
	}


//---------------------------< G E T O N E S H O T D E G C>---------------------------------------------------
/**
Trigger a one-shot temperature conversion, wait for the new value, about 26 msec, and update 
the variable passed.

If the TMP275 is in continuous conversion mode, this places the part in One Shot mode, 
triggers the conversion, waits for the result, updates the variable, and leaves the TMP275 in one shot mode.

returns 0 if no error
**/

uint8_t Systronix_TMP275::getOneShotDegC (float *tempC)
	{
	return FAIL;
	}


//---------------------------< S E T M O D E O N E S H O T >--------------------------------------------------
/**
Set the TMP275 mode to one-shot, with low power sleep in between

mode: set to One Shot if true. 
If false, sets to continuous sampling mode at whatever sample rate was last set.

returns: 0 if successful
**/

uint8_t Systronix_TMP275::setShutdown (boolean sd)
	{
	return FAIL;
	}

