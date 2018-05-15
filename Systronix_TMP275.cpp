/*
 * Systronix_TMP275.cpp
 *
 *  Created on: Nov 15, 2013
 *	  Author: BAB
 */

#include <Systronix_TMP275.h>

/**---------------------------< CONSTRUCTOR >----------------------------------

	@brief  Instantiates a new TMP275 class to use the given base address
	base is clipped to min or max if necessary

-----------------------------------------------------------------------------*/
/*
Systronix_TMP275::Systronix_TMP275 (uint8_t base)
	{
	if (base < TMP275_SLAVE_ADDR_0) 
		{
		_base = TMP275_SLAVE_ADDR_0;
		_base_clipped = true;
		}
	else if (base > TMP275_SLAVE_ADDR_7) 
		{
		_base = TMP275_SLAVE_ADDR_7;
		_base_clipped = true;
		}
	else
		{
		_base = base;
		_base_clipped = false;
		}

	}
*/

//---------------------------< D E F A U L T   C O N S R U C T O R >------------------------------------------
//
// default constructor assumes lowest base address
//

Systronix_TMP275::Systronix_TMP275 (void)
	{
	_base = TMP275_SLAVE_ADDR_0;
	_base_clipped = false;						// since it's constant it must be OK
	error.total_error_count = 0;				// clear the error counter
	}


//---------------------------< D E S T R U C T O R >----------------------------------------------------------
//
// destructor
//

Systronix_TMP275::~Systronix_TMP275 (void)
{
	// Anything to do here? Leave I2C as master? Set flag?
}


//---------------------------< E X I S T S >------------------------------------------------------------------
//
// sensor is instantiated and responds to I2C at base address
//

boolean Systronix_TMP275::exists (void)
{
	return error.exists;
}


//---------------------------< B A S E _ C L I P P E D >------------------------------------------------------
//
// base address passed in constructor or setup() was out of range so clipped to value min <= base <= max
//

boolean Systronix_TMP275::base_clipped (void)
{
	return _base_clipped;
}


//---------------------------< B A S E _ G E T >--------------------------------------------------------------
//
// return the base address; will be the same as the address in constructor or setup() unless clipped in which
// case base address shall be TMP275_SLAVE_ADDR_0 or TMP275_SLAVE_ADDR_7
//

uint8_t Systronix_TMP275::base_get (void)
{
	return _base;
}


//---------------------------< S E T U P >--------------------------------------------------------------------
//
// TODO: merge with begin()? This function doesn't actually do anything, it just sets some private values. It's
// redundant and some params must be effectively specified again in begin (Wire net and pins are not independent).	what parameters are specified again? [wsk]
//

uint8_t Systronix_TMP275::setup (uint8_t base, i2c_t3 wire, char* name)
	{
	if ((TMP275_BASE_MIN > base) || (TMP275_BASE_MAX < base))
		{
		i2c_common.tally_transaction (SILLY_PROGRAMMER, &error);
		return FAIL;
		}

	_base = base;
	_wire = wire;
	_wire_name = wire_name = name;		// protected and public
	return SUCCESS;
	}


//---------------------------< B E G I N >--------------------------------------------------------------------
//
//
//

void Systronix_TMP275::begin (i2c_pins pins, i2c_rate rate)
	{
	_wire.begin (I2C_MASTER, 0x00, pins, I2C_PULLUP_EXT, rate);	// join I2C as master
//	Serial.printf ("275 lib begin %s\r\n", _wire_name);
	_wire.setDefaultTimeout (200000); 							// 200ms
	}


//---------------------------< D E F A U L T   B E G I N >----------------------------------------------------
// One-time startup things here. Call this only at program start.
/*!
	@brief  Join the I2C bus as a master
*/

void Systronix_TMP275::begin (void)
	{
	Wire.begin ();	// join I2C as master
	//	struct data _data;			// instance of the data struct - not used
	//	_data.address = _base;		// struct - not used
	}



//---------------------------< I N I T >----------------------------------------------------------------------
//
// Attempts to write the pointer register and the config register.  
// If successful, sets error.exists true, else false.
// This is the same as writeConfig

uint8_t Systronix_TMP275::init (uint8_t config)
	{
	uint8_t ret_val;

	error.exists = true;					// so we can use config_write(); we'll find out later if device does not exist

//	Serial.printf ("257 lib init %s at base 0x%.2X\r\n", _wire_name, _base);
	ret_val = config_write (config);		// if successful this means we got two ACKs from slave device
	if (SUCCESS != ret_val)
		{
//		Serial.printf ("275 lib init %s at base 0x%.2X failed with %s (0x%.2X)\r\n", _wire_name, _base, status_text[error.error_val], error.error_val);
		error.exists = false;				// only place error.exists is set false
		return ABSENT;
		}

	return SUCCESS;
	}


//---------------------------< R E S E T _ B U S >------------------------------------------------------------
/**
	Invoke resetBus of whichever Wire net this class instance is using
	@return nothing
*/
void Systronix_TMP275::reset_bus (void)
	{
	_wire.resetBus();
	}


//---------------------------< R E S E T _ B U S _ C O U N T _ R E A D >--------------------------------------
/**
	Return the resetBusCount of whichever Wire net this class instance is using
	@return number of Wire net resets, clips at UINT32_MAX
*/
uint32_t Systronix_TMP275::reset_bus_count_read (void)
	{
	return _wire.resetBusCountRead();
	}


//---------------------------< R A W _ 1 2 _ T O _ C >--------------------------------------------------------
/*!
	@brief  Convert raw 12-bit temperature to float deg C
			handles neg and positive values 

	@TODO instead pass a pointer to the float variable? and return error if value out of bounds
*/
//
// receives uint16_t raw12 argument that should be an int16_t.  But, because readRegister() is a general
// purpose function for reading all of the 16 bit registers we get around that by casting the uint16_t to
// int16_t before we use the value.
//
// TODO: take some time to consider if it is worthwhile to have a separate function that just fetches the
// temperature register.  Obvious downside (if it is a downside) is that such a function would necessarily need
// to set the pointer register every time the temperature is read.
//

float Systronix_TMP275::raw12_to_c (uint16_t raw12)
	{
	return 0.0625 * ((int16_t)raw12 >> 4);
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
		if (pointer_write (TMP275_TEMP_REG_PTR))		// attempt to point it
			return FAIL;								// attempt failed; quit

	if (register16_read (&data.raw_temp))				// attempt to read the temperature
		return FAIL;									// attempt failed; quit

	data.t_high = max((int16_t)data.raw_temp, (int16_t)data.t_high);	// keep track of min/max temperatures
	data.t_low = min((int16_t)data.t_low, (int16_t)data.raw_temp);

	data.deg_c = raw12_to_c (data.raw_temp);			// convert to human-readable forms
	data.deg_f = raw12_to_f (data.raw_temp);

	data.fresh = true;									// identify the current data set as new and fresh
	return SUCCESS;
	}


//---------------------------< P O I N T E R _ W R I T E >----------------------------------------------------
/**
Write to a TMP275 register
Start with slave address, as in any I2C transaction.
Next byte must be the Pointer Register value, in 2 lsbs
If all you want to do is set the Pointer Register for subsequent read(s)
then this 2-byte write cycle is complete.

Data bytes in the write are optional but must always follow the Pointer Register write byte.
The last value written to the Pointer Register persists until changed.

**/

uint8_t Systronix_TMP275::pointer_write (uint8_t pointer)
	{
	uint8_t ret_val;

	if (!error.exists)								// exit immediately if device does not exist
		return ABSENT;

	_wire.beginTransmission (_base);				// base address
	ret_val = _wire.write (pointer);				// pointer in 2 lsb
	if (1 != ret_val)
		{
		i2c_common.tally_transaction (WR_INCOMPLETE, &error);			// increment the appropriate counter
		return FAIL;
		}

	ret_val = _wire.endTransmission();
	if (SUCCESS != ret_val)
		{
		i2c_common.tally_transaction (ret_val, &error);				// increment the appropriate counter
		return FAIL;								// calling function decides what to do with the error
		}

	_pointer_reg = pointer;							// update shadow copy to remember this setting

	i2c_common.tally_transaction (SUCCESS, &error);
	return SUCCESS;
	}


//---------------------------< C O N F I G _ W R I T E >------------------------------------------------------
/**
Write to the 8-bit config register
returns SUCCESS if no error, ABSENT or FAIL else
**/

uint8_t Systronix_TMP275::config_write (uint8_t config)
	{
	uint8_t ret_val;

	if (!error.exists)									// exit immediately if device does not exist
		return ABSENT;

	_wire.beginTransmission (_base);				// base address
	ret_val = _wire.write (TMP275_CONF_REG_PTR);	// pointer in 2 lsb
	ret_val += _wire.write (config);				// write configuration register data

	if (2 != ret_val)
		{
		i2c_common.tally_transaction (WR_INCOMPLETE, &error);			// increment the appropriate counter
		return FAIL;
		}

	ret_val = _wire.endTransmission();
	if (SUCCESS != ret_val)
		{
		i2c_common.tally_transaction (ret_val, &error);				// increment the appropriate counter
		return FAIL;								// calling function decides what to do with the error
		}

	_pointer_reg = TMP275_CONF_REG_PTR;				// update shadow copies to remember these settings
	_config_reg = config;

	i2c_common.tally_transaction (SUCCESS, &error);
	return SUCCESS;
	}


//---------------------------< C O N F I G _ R E A D >--------------------------------------------------------
/**
Read the 8-bit config register
returns SUCCESS if no error, ABSENT or FAIL else
*/

uint8_t Systronix_TMP275::config_read (uint8_t *data_ptr)
	{
	uint8_t		ret_val;

	if (!error.exists)									// exit immediately if device does not exist
		return ABSENT;

	if (_pointer_reg != TMP275_CONF_REG_PTR)		// if not pointing to config reg then do so
		{
		if (SUCCESS != pointer_write (TMP275_CONF_REG_PTR))
			return FAIL;
		}

	if (1 != _wire.requestFrom (_base, 1, I2C_STOP))
		{
		ret_val = _wire.status();					// to get error value
		i2c_common.tally_transaction (ret_val, &error);				// increment the appropriate counter
		return FAIL;
		}

	*data_ptr = _wire.readByte();

	i2c_common.tally_transaction (SUCCESS, &error);
	return SUCCESS;
	}


//---------------------------< R E G I S T E R 1 6 _ W R I T E >----------------------------------------------
/**
Param pointer is the TMP275 register into which to write the 16-bit data
returns SUCCESS if no error, ABSENT or FAIL else
**/

uint8_t Systronix_TMP275::register16_write (uint8_t pointer, uint16_t data)
	{
	uint8_t ret_val;

	if (!error.exists)									// exit immediately if device does not exist
		return ABSENT;

	_wire.beginTransmission (_base);					// base address
	ret_val = _wire.write (pointer);					// pointer in 2 lsb
	ret_val += _wire.write ((uint8_t)(data >> 8));		// write MSB of data
	ret_val += _wire.write ((uint8_t)(data & 0x00FF));	// write LSB of data

	if (3 != ret_val)
		{
		error.error_val = 0;
		i2c_common.tally_transaction (WR_INCOMPLETE, &error);			// increment the appropriate counter
		return FAIL;
		}

	ret_val = _wire.endTransmission();
	if (SUCCESS != ret_val)
		{
		i2c_common.tally_transaction (ret_val, &error);				// increment the appropriate counter
		return FAIL;								// calling function decides what to do with the error
		}

	_pointer_reg = pointer;							// update shadow copies to remember these settings

	i2c_common.tally_transaction (SUCCESS, &error);					// increment the appropriate counter
	return SUCCESS;
	}


//---------------------------< R E G I S T E R 1 6 _ R E A D >------------------------------------------------
/**
  Read the 16-bit register addressed by the current pointer value, store the data at the location passed

  TODO: What if current pointer value is the 8-bit config register???
  
returns SUCCESS if no error, ABSENT or FAIL else
*/

uint8_t Systronix_TMP275::register16_read (uint16_t *data_ptr)
	{
	uint8_t		ret_val;

	if (!error.exists)									// exit immediately if device does not exist
		return ABSENT;

	if (2 != _wire.requestFrom (_base, 2, I2C_STOP))
		{
		ret_val = _wire.status();					// to get error value
		i2c_common.tally_transaction (ret_val, &error);				// increment the appropriate counter
		return FAIL;
		}

	*data_ptr = (uint16_t)_wire.readByte() << 8;	// save the data
	*data_ptr |= (uint16_t)_wire.readByte();

	i2c_common.tally_transaction (SUCCESS, &error);					// increment the appropriate counter
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

