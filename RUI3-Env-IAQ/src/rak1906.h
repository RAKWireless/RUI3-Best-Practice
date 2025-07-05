#ifndef __RAK1906_H__
#define __RAK1906_H_

#include "Wire.h"
#include <math.h>

#ifndef _BV
#define _BV(bit) (1 << (bit)) // /< This macro isn't pre-defined on all platforms
#endif

#define RAK1906_ADDRESS 0x76
#define CONCAT_BYTES(msb, lsb) (((uint16_t)msb << 8) | (uint16_t)lsb) // /< combine msb & lsb bytes

typedef enum
{
	RAK1906_STATUS_REGISTER = 0x1D,			  // /< Device status register
	RAK1906_GAS_HEATER_REGISTER0 = 0x5A,	  // /< Heater Register 0 address
	RAK1906_GAS_DURATION_REGISTER0 = 0x64,	  // /< Heater Register 0 address
	RAK1906_CONTROL_GAS_REGISTER1 = 0x70,	  // /< Gas control register on/off
	RAK1906_CONTROL_GAS_REGISTER2 = 0x71,	  // /< Gas control register settings
	RAK1906_CONTROL_HUMIDITY_REGISTER = 0x72, // /< Humidity control register
	RAK1906_SPI_REGISTER = 0x73,			  // /< Status register for SPI memory
	RAK1906_CONTROL_MEASURE_REGISTER = 0x74,  // /< Temp, Pressure control register
	RAK1906_CONFIG_REGISTER = 0x75,			  // /< Configuration register
	RAK1906_CHIPID_REGISTER = 0xD0,			  // /< Chip-Id register
	RAK1906_SOFTRESET_REGISTER = 0xE0,		  // /< Reset when 0xB6 is written here
	RAK1906_CHIPID = 0x61,					  // /< Hard-coded value 0x61 for BME680
	RAK1906_RESET_CODE = 0xB6,				  // /< Reset when this put in reset reg
	RAK1906_MEASURING_BIT_POSITION = 5,		  // /< Bit position for measuring flag
	RAK1906_HUMIDITY_MASK = 0xF8,			  // /< Mask is binary B11111000
	RAK1906_PRESSURE_MASK = 0xE3,			  // /< Mask is binary B11100011
	RAK1906_TEMPERATURE_MASK = 0x1F,		  // /< Mask is binary B00011111
} RAK1906_Commands;

/***************************************************************************************************
** Declare the constants used for calibration                                                     **
***************************************************************************************************/
#define RAK1906_COEFF_SIZE1 25				  // /< First array with coefficients
#define RAK1906_COEFF_SIZE2 16				  // /< Second array with coefficients
#define RAK1906_COEFF_START_ADDRESS1 0x89	  // /< start address for array 1
#define RAK1906_COEFF_START_ADDRESS2 0xE1	  // /< start address for array 2
#define RAK1906_HUM_REG_SHIFT_VAL 4			  // /< Ambient humidity shift value
#define RAK1906_BIT_H1_DATA_MSK 0x0F		  // /< Mask for humidity
#define RAK1906_T2_LSB_REG 1				  // /< Register for temperature calibration
#define RAK1906_T2_MSB_REG 2				  // /< Register for temperature calibration
#define RAK1906_T3_REG 3					  // /< Register for temperature calibration
#define RAK1906_P1_LSB_REG 5				  // /< Register for pressure calibration
#define RAK1906_P1_MSB_REG 6				  // /< Register for pressure calibration
#define RAK1906_P2_LSB_REG 7				  // /< Register for pressure calibration
#define RAK1906_P2_MSB_REG 8				  // /< Register for pressure calibration
#define RAK1906_P3_REG 9					  // /< Register for pressure calibration
#define RAK1906_P4_LSB_REG 11				  // /< Register for pressure calibration
#define RAK1906_P4_MSB_REG 12				  // /< Register for pressure calibration
#define RAK1906_P5_LSB_REG 13				  // /< Register for pressure calibration
#define RAK1906_P5_MSB_REG 14				  // /< Register for pressure calibration
#define RAK1906_P7_REG 15					  // /< Register for pressure calibration
#define RAK1906_P6_REG 16					  // /< Register for pressure calibration
#define RAK1906_P8_LSB_REG 19				  // /< Register for pressure calibration
#define RAK1906_P8_MSB_REG 20				  // /< Register for pressure calibration
#define RAK1906_P9_LSB_REG 21				  // /< Register for pressure calibration
#define RAK1906_P9_MSB_REG 22				  // /< Register for pressure calibration
#define RAK1906_P10_REG 23					  // /< Register for pressure calibration
#define RAK1906_H2_MSB_REG 0				  // /< Register for humidity calibration
#define RAK1906_H2_LSB_REG 1				  // /< Register for humidity calibration
#define RAK1906_H1_LSB_REG 1				  // /< Register for humidity calibration
#define RAK1906_H1_MSB_REG 2				  // /< Register for humidity calibration
#define RAK1906_H3_REG 3					  // /< Register for humidity calibration
#define RAK1906_H4_REG 4					  // /< Register for humidity calibration
#define RAK1906_H5_REG 5					  // /< Register for humidity calibration
#define RAK1906_H6_REG 6					  // /< Register for humidity calibration
#define RAK1906_H7_REG 7					  // /< Register for humidity calibration
#define RAK1906_T1_LSB_REG 8				  // /< Register for gas calibration
#define RAK1906_T1_MSB_REG 9				  // /< Register for gas calibration
#define RAK1906_GH2_LSB_REG 10				  // /< Register for gas calibration
#define RAK1906_GH2_MSB_REG 11				  // /< Register for gas calibration
#define RAK1906_GH1_REG 12					  // /< Register for gas calibration
#define RAK1906_GH3_REG 13					  // /< Register for gas calibration
#define RAK1906_ADDR_RES_HEAT_RANGE_ADDR 0x02 // /< Register for gas calibration
#define RAK1906_RHRANGE_MSK 0x30			  // /< Register for gas calibration
#define RAK1906_ADDR_RES_HEAT_VAL_ADDR 0x00	  // /< Register for gas calibration
#define RAK1906_ADDR_RANGE_SW_ERR_ADDR 0x04	  // /< Register for gas calibration
#define RAK1906_RSERROR_MSK 0xF0			  // /< Register for gas calibration

typedef enum
{
	RAK1906_NO_ERROR = 0,
	RAK1906_NOT_INIT = -1,
	RAK1906_WRONG_ARG = -2,
	RAK1906_NACK = -3,
} RAK1906_ErrorCode;

enum sensorTypes
{
	TemperatureSensor,
	HumiditySensor,
	PressureSensor,
	GasSensor,
	UnknownSensor
};
enum iirFilterTypes
{
	IIROff,
	IIR2,
	IIR4,
	IIR8,
	IIR16,
	IIR32,
	IIR64,
	IIR128,
	UnknownIIR
};
enum oversamplingTypes
{
	SensorOff,
	Oversample1,
	Oversample2,
	Oversample4,
	Oversample8,
	Oversample16,
	UnknownOversample
};

class rak1906
{
public:
	rak1906();

#ifdef SUPPORT_RAK1906
	/**@addtogroup	Rak1906
	 * @{
	 */
#endif

	/**@brief	This function enabled the RAK1906 sensor and puts the device into
	 * 	standby mode / sleep mode for lowest power consumption
	 *
	 * @return bool		True if successfully query and identify RAK1906 module.
	 * 			FALSE if RAK1906 init failed
	 */
	bool init(void);

	/**@brief	This function will trigger the RAK1906 to update the Environment values,
	 * 	after the values are updated theRAK1906 will go back to power saving mode
	 *
	 * @return bool		True if values updated successfully. FALSE if update failed
	 */
	bool update(void);

	/**@brief	This function will trigger the RAK1906 to return the latest TEMPERATURE value
	 *
	 * @return float
	 */
	float temperature(void);

	/**@brief	This function will trigger the RAK1906 to return the latest HUMIDITY value
	 *
	 * @return float
	 */
	float humidity(void);

	/**@brief	This function will trigger the RAK1906 to return the latest AIR PRESSURE value
	 *
	 * @return float
	 */
	float pressure(void);

	/**@brief	This function will trigger the RAK1906 to return the latest GAS value
	 *
	 * @return float
	 */
	float gas(void);
	/**@}*/

	/**@brief	This function sets the oversampling rate for the sensor types
	 * @var sensor Sensor type TemperatureSensor, HumiditySensor, PressureSensor
	 * @var oversampling IIR filter to IIROff, IIR2, IIR4, IIR8, IIR16, IIR32, IIR64 or IIR128
	 * @return float
	 */
	uint8_t setOversampling(const uint8_t sensor, const uint8_t oversampling);

	/**@brief	This function sets the IIR filter to be used
	 * @var iirFilterSetting IIR filter to IIROff, IIR2, IIR4, IIR8, IIR16, IIR32, IIR64 or IIR128
	 * @return uint8_t
	 */
	uint8_t setIIRFilter(const uint8_t iirFilterSetting = UINT8_MAX) const; // Set IIR Filter

	/**@brief	This function sets gas sensor temperature and heating time
	 * @var GasTemp Temperature in degree
	 * @var GasMillis Heating time in milliseconds
	 * @return bool
	 */
	bool setGas(uint16_t GasTemp, uint16_t GasMillis) const; // Gas heating temperature and time

	int32_t tmpTemperature,
		tmpHumidity,
		tmpPressure,
		tmpGas;

private:
	RAK1906_ErrorCode sendCommand(RAK1906_Commands cmd);
	RAK1906_ErrorCode readData(uint16_t *);

	void waitForReadings() const; // /< Wait for readings to finish
	uint8_t readByte(const uint8_t) const;
	void getCalibration(); // /< Load calibration from registers
	float altitude(const int32_t press, const float seaLevel =
											1013.25);
	bool measuring() const;			 // /< true if currently measuring
	void triggerMeasurement() const; // /< trigger a measurement

	uint8_t _H6, _P10, _res_heat_range;
	int8_t _H3, _H4, _H5, _H7, _G1, _G3, _T3, _P3, _P6, _P7, _res_heat, _rng_sw_err;
	uint16_t _H1, _H2, _T1, _P1;
	int16_t _G2, _T2, _P2, _P4, _P5, _P8, _P9;
	int32_t _tfine, _Temperature, _Pressure, _Humidity, _Gas;

	// getData
	template <typename T>
	uint8_t &getData(const uint8_t addr, T &value) const
	{
		uint8_t *bytePtr = (uint8_t *)&value;		  // Pointer to structure beginning
		static uint8_t structSize = sizeof(T);		  // Number of bytes in structure
		Wire.beginTransmission(RAK1906_ADDRESS);	  // Address the I2C device
		Wire.write(addr);							  // Send register address to read
		Wire.endTransmission();						  // Close transmission
		Wire.requestFrom(RAK1906_ADDRESS, sizeof(T)); // Request 1 byte of data
		structSize = Wire.available();				  // Use the actual number of bytes
		for (uint8_t i = 0; i < structSize; i++)
			*bytePtr++ = Wire.read(); // loop for each byte to be read
		return (structSize);
	} // end getData()

	template <typename T>
	uint8_t &putData(const uint8_t addr, const T &value) const
	{
		const uint8_t *bytePtr = (const uint8_t *)&value; // Pointer to structure beginning
		static uint8_t structSize = sizeof(T);			  // Number of bytes in structure
		Wire.beginTransmission(RAK1906_ADDRESS);		  // Address the I2C device
		Wire.write(addr);								  // Send register address to write
		for (uint8_t i = 0; i < sizeof(T); i++)
			Wire.write(*bytePtr++); // loop for each byte to be written
		Wire.endTransmission();		// Close transmission
		return (structSize);
	}
};

#endif // end RAK1906_
