#ifndef SYSTRONIX_TMP275_h
#define SYSTRONIX_TMP275_h


/******************************************************************************/
/*!
	@file		Systronix_TMP275.h
	
	@author		B Boyes (Systronix Inc)
    @license	BSD (see license.txt)	
    @section	HISTORY

    v0.1	2017Mar02 bboyes Start based on Systronix_TMP275

*/
/******************************************************************************/

/** TODO LIST

Function to get current temp in degrees C TMP275_GetTempC (uint16_t *data16)
Change constants and add a function to config so it can be something like
TMP275_Setup(TMP275_ONE_SHOT)

**/

/** --------  Description ------------------------------------------------------

This library is based on the TMP275 library. The TMP275 is very similar. Here are 
the differences of TMP275:

- resolution is the same: .0625 °C per count in 12-bit mode, but the TMP275 adds a 
13-bit mode so that it can extend that resolution over a wider range –55 to +150 °C
- TMP275 accuracy is much better
- TMP275 has 8 possible slave address vs TMP275 4 
- TMP275 costs a bit more

See the data sheet at http://www.ti.com/product/tmp275
This lbrary based on data sheet rev E, 2015 Nov, document SBOS363E.

Max operating temperature range is -40 to +125 °C

TODO: update these to '275 values
Accuracy from –25°C to +85°C is 0.5/2°C typ/max.
Accuracy from –40°C to +125°C is 1/3°C typ/max.
Accuracy from –55°C to +150°C is not specified. I plan to compare to a thermocouple
and see what some typical accuracies are.

One lsb is 0.0625°C in 12-bit format. 
Negative numbers are binary twos complement format.

------------------------------------------------------------------------------*/

/** -------- Arduino Device and Version Testing and Issues ---------------------

This library assumes Arduino greater than 1.00, with Wire.read() and Wire.write()

This library was developed and tested on Teensy3 (ARM CortexM4) with I2C_T3 library.

------------------------------------------------------------------------------*/

//---------------------------< I N C L U D E S >--------------------------------------------------------------

#include <Arduino.h>

// Use Teensy improved I2C library
//#if defined (__MK20DX256__) || defined (__MK20DX128__) 	// Teensy 3.1 or 3.2 || Teensy 3.0
// from https://forum.pjrc.com/threads/42411-Communication-impossible-in-I2C-tennsy3-6?p=135630&viewfull=1#post135630
#if defined(KINETISK) || defined(KINETISL)	// Teensy 3.X and LC
#include <i2c_t3.h>		
#else
#include <Wire.h>	// for AVR I2C library
#endif


//---------------------------< D E F I N E S >----------------------------------------------------------------

#define		SUCCESS					0
#define		FAIL					(~SUCCESS)
#define		ABSENT					0xFD

#define		WR_INCOMPLETE		11
#define		SILLY_PROGRAMMER	12


/** --------  Device Addressing --------
TMP275 base address is 0x48 (B 1001 000x) where x is R/W

This is confusing because the address part is just the the upper 7 bits, so if considered
alone, and right-justified, that is  B 0100 1000 which is 0x48 which is how Arduino wants it.
If A[2..0] are low, base is 0x48. If A[2..0] are 100, base is 0x4C.
If A[2..0] are high, base is 0x4F, the max
-------------------------------------*/
#define		TMP275_SLAVE_ADDR_0		0x48
#define		TMP275_SLAVE_ADDR_1		0x49
#define		TMP275_SLAVE_ADDR_2		0x4A
#define		TMP275_SLAVE_ADDR_3		0x4B
#define		TMP275_SLAVE_ADDR_4		0x4C
#define		TMP275_SLAVE_ADDR_5		0x4D
#define		TMP275_SLAVE_ADDR_6		0x4E
#define		TMP275_SLAVE_ADDR_7		0x4F

#define		TMP275_BASE_MIN			TMP275_SLAVE_ADDR_0
#define		TMP275_BASE_MAX			TMP275_SLAVE_ADDR_7


/** --------  Register Addresses --------
The two lsb of the pointer register hold the register bits, which
are used to address one of the four directly-accessible registers.

There are four pointer addresses:
00  Temperature Register (Read Only) 12 bits in ms position
	If temp is positive (msb=0) the value is just the binary value.
	If temp is negative (msb=0) the value is in 2's complement form,
	so take the whole binary value, complement it, and add 1.
01  Configuration Register (Read/Write) 8 bits, MSB first
10  TLOW Limit Register (Read/Write) POR = 80 deg C
11  THIGH Limit Register (Read/Write) POR = 75 deg C
*/

#define		TMP275_TEMP_REG_PTR		0x00		// 16-bit temperature register, read only
#define		TMP275_CONF_REG_PTR		0x01		// 16-bit config register, read/write
#define		TMP275_TLOW_REG_PTR		0x02		// 16-bit Tlow register, read/write
#define		TMP275_THIGH_REG_PTR	0x03		// 16-bit Thigh register, read/write

/** --------  Configuration --------
OR these bits together appropriately, one choice from each option group
and write to the config register for the desired option
This is an 8-bit register, bits 7..0 
I have used the same designation as the data sheet, to be consistent,
instead of making names longer and perhaps more readable;
for example TMP275_CFG_AL = 'AL', the Alert config bit
*/

/*
  0x00 is the POR default read value of the configuration register.
  9-bit resolution 

  We want 12-bit resolution so that is a config value of 0x60

  Note that bit 7 (OS) always reads as zero!
*/	
#define		TMP275_CFG_POR_RD		0x0C90		// always reads as 0x00 after POR

/* One-shot/Conversion Ready is Config bit 7
  When in shutdown mode (SD=1), setting OS starts a single conversion
  during which OS reads '0'. When conversion is complete OS = 1.
  A single 12-bit conversion typically takes 220 ms and a read can take
  place in less than 20ms. 
  Default is OS = 0. Note that this bit always reads as zero!
*/
#define		TMP275_CFG_OS			0x80		// Start a single conversion (if in SD mode)

// Resolution is Config bits 6,5
// 00 = 9 bits,  0.5 °C resolution
// 01 = 10 bits, 0.25 °C
// 10 = 11 bits, 0.125 °C
// 11 = 12 bits, 0.0625 °C resolution
// I don't see a use for anything less than 12 bit mode

#define		TMP275_CFG_RES12		0x60
#define		TMP275_CFG_RES9			0x00		// both bits zero, 9-bit mode

// Fault Queue Config bits 4,3
// how many faults generate an Alert based on T-high and T-low registers
#define		TMP275_CFG_FLTQ_6		0x18		// 6 consecutive faults
#define		TMP275_CFG_FLTQ_4		0x10		// 4 consecutive faults
#define		TMP275_CFG_FLTQ_2		0x08		// 2 consecutive faults
#define		TMP275_CFG_FLTQ_1		0x00		// 1 consecutive fault *default*

// Polarity bit is Config bit 2, adjusts polarity of Alert pin output
// POL=0 means Alert is active LOW
// POL=1 means Alert is active HIGH
#define		TMP275_CFG_POL			0x04		// 0 = Alert active LOW *default*

// TM Thermostat Mode is Config bit 1
// Tells TMP275 to operate in Comparator (TM=0) or Interrupt mode (TM=1)
#define		TMP275_CFG_TM			0x02		// 0 = Comparator mode *default*

/** Shutdown mode Config bit 0
Set this bit to be low power (0.1 uA) between single conversions
In SD mode you set the OS (one shot) bit to start a single conversion,
it converts once then goes back to sleep. But the new conversion takes
220 msec in 12-bit mode so if you read right away you will get the previous
conversion value.
*/

#define		TMP275_CFG_SD			0x01		// 0 = continuous conversion state *default*

class Systronix_TMP275
{
	protected:
		uint8_t		_base;							// base address for this instance; four possible values
		uint8_t		_pointer_reg;					// copy of the pointer register value so we know where it's pointing
		uint8_t		_config_reg;					// copy
		void		tally_transaction (uint8_t);	// maintains the i2c_t3 error counters
		boolean		_base_clipped;

		char* 		_wire_name = (char*)"empty";
		i2c_t3		_wire = Wire;					// why is this assigned value = Wire? [bab]

	public:
		// Instance-specific properties
		/** Data for one instance of a TMP275 temp sensor.
		12-bit mode is assumed 
		Error counters could be larger but then they waste more data in the typical case where they are zero.
		Errors peg at max value for the data type: they don't roll over.
		
		Maybe different structs for data values and part control
		**/
		struct data_t
			{
			uint16_t	raw_temp;				// most recent
			uint16_t	t_high = 0x0C90<<4;		// historical (since reset) high temp (preset here to device max low (-55 C) raw12 format)
			uint16_t	t_low = 0x07FF<<4; 		// historical low temp (preset here to device max high (128 C) raw12 format)
			float		deg_c;        
			float		deg_f;
			bool		fresh;					// data is good and fresh TODO: how does one know that the data are not 'fresh'?
			} data;

		/**
		Array of Wire.status() extended return code strings, 11 as of 29Dec16 i2c_t3 release
		index into this with the value of status.
		There is an array of constant text: const status_text[11]
		char * makes the decl an array of char pointers, each pointing to constant text
		the first const means that array of char pointers can't change.
		We can access this with a const char * text_ptr which means point to char(s) which happen to be const
		Note each literal string has a null terminator added by C compiler.
		See NAP_UI_key_defs.h for similar

		TODO A problem is that SUCCESS returns 0 and gets put into error_val, so
		we can't tell the difference between SUCCESS and I2C_WAITING
		Since requestFrom is blocking, only "I2C message is over" status can occur.
		In Writing, with endTransmission, it is blocking, so only end of message errors can exist.
		*/
#if defined I2C_T3_H
		const char * const status_text[13] =
		{
			"I2C_WAITING", 						// first four are not errors but status; first eleven taken from i2c_t3.h
			"I2C_SENDING",
			"I2C_SEND_ADDR",
			"I2C_RECEIVING",
			"I2C_TIMEOUT", 						// start of 5 errors, status==4
			"I2C_ADDR_NAK",
			"I2C_DATA_NAK",
			"I2C_ARB_LOST",
			"I2C_BUF_OVF",
			"I2C_SLAVE_TX", 					// slave status; not errors
			"I2C_SLAVE_RX",
			"WR_INCOMPLETE",
			"SILLY_PROGRAMMER"					// Doh. Slap forehead.
		};
#else
		// Wire.h returns from endTransmission
		// 0=success, 1=data too long, 2=recv addr NACK, 3=recv data NACK, 4=other error
		const char * const status_text[5] =
		{
			"Success",
			"Data length",
			"Receive addr NAK",
			"Receive data NAK",
			"Other error"
		};
#endif

		/** error stucture
		Note that this can be written by a library user, so it could be cleared if desired as part of
		some error recovery or logging operation. It could also be inadvertenly erased...

		successful_count overflowed at 258.5 hours. Making this a 64-bit unsigned (long long) allows
		for 2**32 times as many hours. So not likely to ever wrap wrap.
		*/
		struct
			{
			boolean		exists;						// set false after an unsuccessful i2c transaction
			uint8_t		error_val;					// the most recent error value, not just SUCCESS or FAIL
			uint32_t	incomplete_write_count;		// Wire.write failed to write all of the data to tx_buffer
			uint32_t	data_len_error_count;		// data too long
			uint32_t	timeout_count;				// slave response took too long
			uint32_t	rcv_addr_nack_count;		// slave did not ack address
			uint32_t	rcv_data_nack_count;		// slave did not ack data
			uint32_t	arbitration_lost_count;
			uint32_t	buffer_overflow_count;
			uint32_t	other_error_count;			// from endTransmission there is "other" error
			uint32_t	unknown_error_count;
			uint32_t	data_value_error_count;		// I2C message OK but value read was wrong; how can this be?
			uint32_t	silly_programmer_error;		// I2C address to big or something else that "should never happen"
			uint64_t	total_error_count;			// quick check to see if any have happened
			uint64_t	successful_count;			// successful access cycle
			} error;

		char*		wire_name;						// name of Wire, Wire1, etc in use

		boolean exists (void);
		boolean base_clipped (void);
		uint8_t base_get (void);

//		Systronix_TMP275 (uint8_t base);								// constructor w/base passed in
		Systronix_TMP275 (void);										// default constructor
		~Systronix_TMP275 (void);										// deconstructor

		uint8_t		setup (uint8_t base, i2c_t3 wire, char* name);
		void 		begin (i2c_pins pins, i2c_rate rate);
		void		begin (void);										// default begin
		uint8_t		init (uint8_t control_value);						// set operation mode, check device present and communicating

		float		raw12_to_c (uint16_t raw12);						// temperature conversion functions
		float		raw12_to_f (uint16_t raw12);
		uint8_t		get_temperature_data (void);

		uint8_t		pointer_write (uint8_t pointer);					// i2c bus dependent functions
		uint8_t		register16_write (uint8_t pointer, uint16_t data);	// write to 16-bit registers
		uint8_t		config_write (uint8_t data);						// write to 8-bit config register)
		uint8_t		register16_read (uint16_t *data);
		uint8_t		config_read (uint8_t *data);						// read 8-bit config register

		void		reset_bus (void);									// invoke Wire[x] resetBus()
		uint32_t	reset_bus_count_read (void);						// read resetBusCount for Wire[x]

//----- THESE FUNCTIONS not yet implemented; all return FAIL
		uint8_t		tempReadDegC (float *tempC);
		uint8_t		tempReadDegF (float *tempF);
		uint8_t		degCToRaw12 (uint16_t *raw13, float *tempC);		// use to store value in comparison registers
		uint8_t		degFToRaw12 (uint16_t *raw13, float *tempC);
		uint8_t		getOneShotDegC (float *tempC);						// TODO may not be best approach for TMP275
		uint8_t		setShutdown (boolean sd);							// ditto

	private:

};

extern Systronix_TMP275 tmp275;

#endif /* SYSTRONIX_TMP275_h */
