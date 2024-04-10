#include "rak1906.h"

rak1906::rak1906()
{
}

uint8_t rak1906::readByte(const uint8_t addr) const
{
	uint8_t returnValue;		// Storage for returned value
	getData(addr, returnValue); // Read just one byte
	return (returnValue);		// Return byte just read
}

bool rak1906::init()
{
	Wire.beginTransmission(RAK1906_ADDRESS);
	if (!Wire.endTransmission() == 0)
		return false;

	getCalibration();														// get the calibration values
	uint8_t workRegister = readByte(RAK1906_CONTROL_MEASURE_REGISTER);		// Read the control measure
	putData(RAK1906_CONTROL_MEASURE_REGISTER, (uint8_t)(workRegister | 1)); // Trigger 1st measurement
	setOversampling(TemperatureSensor, Oversample16);						// Use enumerated type values
	setOversampling(HumiditySensor, Oversample16);							// Use enumerated type values
	setOversampling(PressureSensor, Oversample16);							// Use enumerated type values
	setIIRFilter(IIR4);														// Use enumerated type values 320 C for 150 milliseconds

	return true;
}

bool rak1906::update()
{
	Wire.beginTransmission(RAK1906_ADDRESS);
	if (!Wire.endTransmission() == 0)
		return false;

	const uint32_t lookupTable1[16] = {
		UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2147483647),
		UINT32_C(2147483647),
		UINT32_C(2147483647), UINT32_C(2126008810), UINT32_C(2147483647),
		UINT32_C(2130303777),
		UINT32_C(2147483647), UINT32_C(2147483647), UINT32_C(2143188679),
		UINT32_C(2136746228),
		UINT32_C(2147483647), UINT32_C(2126008810), UINT32_C(2147483647),
		UINT32_C(2147483647)};
	const uint32_t lookupTable2[16] = {
		UINT32_C(4096000000), UINT32_C(2048000000), UINT32_C(1024000000),
		UINT32_C(512000000),
		UINT32_C(255744255), UINT32_C(127110228), UINT32_C(64000000),
		UINT32_C(32258064),
		UINT32_C(16016016), UINT32_C(8000000), UINT32_C(4000000),
		UINT32_C(2000000),
		UINT32_C(1000000), UINT32_C(500000), UINT32_C(250000),
		UINT32_C(125000)};

	uint8_t buff[15],
		gas_range = 0;										 // declare array for registers
	int64_t var1, var2, var3, var4, var5, var6, temp_scaled; // Work variables
	uint32_t adc_temp, adc_pres;							 // Raw ADC temperature and pressure
	uint16_t adc_hum, adc_gas_res;							 // Raw ADC humidity and gas
	waitForReadings();
	getData(RAK1906_STATUS_REGISTER, buff);																	// read all 15 bytes in one go
	adc_pres = (uint32_t)(((uint32_t)buff[2] << 12) | ((uint32_t)buff[3] << 4) | ((uint32_t)buff[4] >> 4)); // put the 3 bytes of Pressure
	adc_temp = (uint32_t)(((uint32_t)buff[5] << 12) | ((uint32_t)buff[6] << 4) | ((uint32_t)buff[7] >> 4)); // put the 3 bytes of Temperature
	adc_hum = (uint16_t)(((uint32_t)buff[8] << 8) | (uint32_t)buff[9]);										// put the 2 bytes of Humidity
	adc_gas_res = (uint16_t)((uint32_t)buff[13] << 2 | (((uint32_t)buff[14]) >> 6));						// put the 2 bytes of Gas
	gas_range = buff[14] & 0X0F;																			// Retrieve the range

	/*******************************
	 First compute the temperature
	*******************************/
	var1 = ((int32_t)adc_temp >> 3) - ((int32_t)_T1 << 1); // Perform calibration/adjustment
	var2 = (var1 * (int32_t)_T2) >> 11;					   // of Temperature values according
	var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;			   // to formula defined by Bosch
	var3 = ((var3) * ((int32_t)_T3 << 4)) >> 14;
	_tfine = (int32_t)(var2 + var3);
	_Temperature = (int16_t)(((_tfine * 5) + 128) >> 8);
	/******************************
	 Now compute the pressure
	*******************************/
	var1 = (((int32_t)_tfine) >> 1) - 64000;
	var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)_P6) >> 2;
	var2 = var2 + ((var1 * (int32_t)_P5) << 1);
	var2 = (var2 >> 2) + ((int32_t)_P4 << 16);
	var1 =
		(((((var1 >> 2) * (var1 >> 2)) >> 13) *
		  ((int32_t)_P3 << 5)) >>
		 3) +
		(((int32_t)_P2 * var1) >> 1);
	var1 = var1 >> 18;
	var1 = ((32768 + var1) * (int32_t)_P1) >> 15;
	_Pressure = 1048576 - adc_pres;
	_Pressure = (int32_t)((_Pressure - (var2 >> 12)) * ((uint32_t)3125));

	if (_Pressure >= INT32_C(0x40000000)) // Issue #26
		_Pressure = ((_Pressure / (uint32_t)var1) << 1);
	else
		_Pressure = ((_Pressure << 1) / (uint32_t)var1);
	var1 =
		((int32_t)_P9 *
		 (int32_t)(((_Pressure >> 3) * (_Pressure >> 3)) >> 13)) >>
		12;
	var2 = ((int32_t)(_Pressure >> 2) * (int32_t)_P8) >> 13;
	var3 =
		((int32_t)(_Pressure >> 8) * (int32_t)(_Pressure >> 8) *
		 (int32_t)(_Pressure >> 8) * (int32_t)_P10) >>
		17;
	_Pressure =
		(int32_t)(_Pressure) +
		((var1 + var2 + var3 + ((int32_t)_P7 << 7)) >> 4);
	/*********************
	 Compute the humidity
	**********************/
	temp_scaled = (((int32_t)_tfine * 5) + 128) >> 8;
	var1 = (int32_t)(adc_hum - ((int32_t)((int32_t)_H1 << 4))) -
		   (((temp_scaled * (int32_t)_H3) / ((int32_t)100)) >> 1);
	var2 =
		((int32_t)_H2 *
		 (((temp_scaled * (int32_t)_H4) / ((int32_t)100)) +
		  (((temp_scaled *
			 ((temp_scaled * (int32_t)_H5) / ((int32_t)100))) >>
			6) /
		   ((int32_t)100)) +
		  (int32_t)(1 << 14))) >>
		10;
	var3 = var1 * var2;
	var4 = (int32_t)_H6 << 7;
	var4 =
		((var4) + ((temp_scaled * (int32_t)_H7) / ((int32_t)100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	_Humidity = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
	if (_Humidity > 100000) /* Cap at 100%rH */
		_Humidity = 100000;
	else if (_Humidity < 0)
		_Humidity = 0;
	/**********************
	 Compute the Gas
	**********************/
	uint64_t uvar2;
	var1 =
		(int64_t)((1340 +
				   (5 * (int64_t)_rng_sw_err)) *
				  ((int64_t)lookupTable1[gas_range])) >>
		16;
	uvar2 =
		(((int64_t)((int64_t)adc_gas_res << 15) - (int64_t)(16777216)) +
		 var1);
	var3 = (((int64_t)lookupTable2[gas_range] * (int64_t)var1) >> 9);
	_Gas = (uint32_t)((var3 + ((int64_t)uvar2 >> 1)) / (int64_t)uvar2);
	triggerMeasurement(); // trigger the next measurement

	tmpTemperature = _Temperature;
	tmpHumidity = _Humidity;
	tmpPressure = _Pressure;
	tmpGas = _Gas;

	return true; // FIXME
}

float rak1906::temperature()
{
	return (float)tmpTemperature / 100;
}

float rak1906::humidity()
{
	return (float)tmpHumidity / 1000;
}

float rak1906::pressure()
{
	return (float)tmpPressure / 100;
}

float rak1906::gas()
{
	return (float)tmpGas / 100;
}

uint8_t rak1906::setOversampling(const uint8_t sensor, const uint8_t sampling)
{
	if (sensor >= UnknownSensor || (sampling != UINT8_MAX && sampling >= UnknownOversample))
	{
		// return an error if sensor or oversample is out of range
		return (UINT8_MAX);
	}
	uint8_t tempRegister;			// Temporary register variable
	uint8_t returnValue = sampling; // Return sampling value
	waitForReadings();				// Ensure any active reading is finished
	switch (sensor)
	{
	// Depending upon which sensor is chosen
	case HumiditySensor: // Set the humidity oversampling
	{
		tempRegister = readByte(RAK1906_CONTROL_HUMIDITY_REGISTER); // Read the register contents
		if (sampling == UINT8_MAX)
		{
			// If we just want to read values

			returnValue = tempRegister & ~RAK1906_HUMIDITY_MASK; // Set return value
		}
		else
		{
			tempRegister &= RAK1906_HUMIDITY_MASK;							   // Mask bits to 0
			tempRegister |= sampling;										   // Add in the sampling bits
			putData(RAK1906_CONTROL_HUMIDITY_REGISTER, (uint8_t)tempRegister); // Update humidity bits 0:2
		}																	   // if-then return current value or set new value of HumiditySensor
		break;
	}
	case PressureSensor: // Set the pressure oversampling
	{
		tempRegister = readByte(RAK1906_CONTROL_MEASURE_REGISTER); // Read the register contents
		if (sampling == UINT8_MAX)
		{
			// If we just want to read the values
			returnValue = (tempRegister & ~RAK1906_PRESSURE_MASK) >> 2; // Set return value
		}
		else
		{
			tempRegister &= RAK1906_PRESSURE_MASK;							  // Mask bits to 0
			tempRegister |= (sampling << 2);								  // Add in sampling bits at offset
			putData(RAK1906_CONTROL_MEASURE_REGISTER, (uint8_t)tempRegister); // Update register
		}																	  // if-then return current value or set new value of PressureSensor

		break;
	}
	case TemperatureSensor: // Set the temperature oversampling
	{
		tempRegister = readByte(RAK1906_CONTROL_MEASURE_REGISTER); // Read the register contents
		if (sampling == UINT8_MAX)
		{
			// If we just want to read the values
			returnValue = (tempRegister & ~RAK1906_TEMPERATURE_MASK) >> 5; // Set return value
		}
		else
		{
			tempRegister &= RAK1906_TEMPERATURE_MASK;						  // Mask bits to/ 0
			tempRegister |= (sampling << 5);								  // Add in the sampling bits at offset
			putData(RAK1906_CONTROL_MEASURE_REGISTER, (uint8_t)tempRegister); // Update humidity bits 5:7
		}																	  // if-then return current value or set new value of TemperatureSensor
		break;
	}
	default:
		return (UINT8_MAX); // Return an error if no match of switch the sensor type
	}
	return (returnValue); // Otherwise return current value
}

uint8_t rak1906::setIIRFilter(const uint8_t iirFilterSetting) const
{
	waitForReadings();										 // Ensure any active reading is finished
	uint8_t returnValue = readByte(RAK1906_CONFIG_REGISTER); // Get control register byte contents
	if (iirFilterSetting != UINT8_MAX)						 // If the value is to be changed
	{
		returnValue = returnValue & 0xE3;			   // mask IIR bits
		returnValue |= (iirFilterSetting & 0x07) << 2; // use 3 bits of iirFilterSetting
		putData(RAK1906_CONFIG_REGISTER, returnValue); // Write new control register value
	}												   // if the value is to be changed
	returnValue = (returnValue >> 2) & 0x07;		   // Extract IIR filter setting from register
	return (returnValue);
}

bool rak1906::setGas(uint16_t GasTemp, uint16_t GasMillis) const
{
	uint8_t gasRegister = readByte(RAK1906_CONTROL_GAS_REGISTER2); // Read current register values
	if (GasTemp == 0 || GasMillis == 0)
	{
		// If either input variable is zero //
		putData(RAK1906_CONTROL_GAS_REGISTER1, (uint8_t)0x08);				   // Turn off gas heater
		putData(RAK1906_CONTROL_GAS_REGISTER2, (uint8_t)(gasRegister & 0xEF)); // Turn off gas measurements
	}
	else
	{
		putData(RAK1906_CONTROL_GAS_REGISTER1, (uint8_t)0); // Turn off heater bit to turn on
		uint8_t heatr_res;
		int32_t var1, var2, var3, var4, var5, heatr_res_x100;
		if (GasTemp < 200)
			GasTemp = 200;
		else if (GasTemp > 400)
			GasTemp = 400; // Clamp temperature to min/max

		var1 = (((int32_t)(_Temperature / 100) * _H3) / 1000) << 8;
		var2 = (_G1 + 784) * (((((_G2 + 154009) * GasTemp * 5) / 100) + 3276800) / 10); // Issue #26
		var3 = var1 + (var2 / 2);
		var4 = (var3 / (_res_heat_range + 4));
		var5 = (131 * _res_heat) + 65536;
		heatr_res_x100 = (int32_t)(((var4 / var5) - 250) * 34);
		heatr_res = (uint8_t)((heatr_res_x100 + 50) / 100);
		putData(RAK1906_GAS_HEATER_REGISTER0, heatr_res);
		uint8_t factor = 0;
		uint8_t durval;
		if (GasMillis >= 0xfc0)
			durval = 0xff; // Max duration
		else
		{
			while (GasMillis > 0x3F)
			{
				GasMillis = GasMillis >> 2;
				factor += 1;
			} // of while loop
			durval = (uint8_t)(GasMillis + (factor * 64));
		}													// of if-then-else duration exceeds max
		putData(RAK1906_CONTROL_GAS_REGISTER1, (uint8_t)0); // then turn off gas heater
		putData(RAK1906_GAS_DURATION_REGISTER0, durval);
		putData(RAK1906_CONTROL_GAS_REGISTER2,
				(uint8_t)(gasRegister | 0x10));
	} // of if-then-else turn gas measurements on or off
	return true;
}

float altitude(const int32_t press, const float seaLevel)
{
	static float Altitude;
	Altitude = 44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903)); // Convert into meters
	return (Altitude);
} // of method altitude()

void rak1906::getCalibration()
{
	/*************************************
	 ** Temperature related coefficients **
	 *************************************/
	uint8_t coeff_arr1[RAK1906_COEFF_SIZE1] = {0};	   // Define temp array 1 and initialize with 0x
	uint8_t coeff_arr2[RAK1906_COEFF_SIZE2] = {0};	   // Define temp array 2 and initialize with 0s
	getData(RAK1906_COEFF_START_ADDRESS1, coeff_arr1); // Split reading registers into 2
	getData(RAK1906_COEFF_START_ADDRESS2, coeff_arr2); // one 25 bytes and the other 16
	_T1 =
		(uint16_t)(CONCAT_BYTES(coeff_arr2[RAK1906_T1_MSB_REG],
								coeff_arr2[RAK1906_T1_LSB_REG]));
	_T2 =
		(int16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_T2_MSB_REG],
							   coeff_arr1[RAK1906_T2_LSB_REG]));
	_T3 = (int8_t)(coeff_arr1[RAK1906_T3_REG]);
	/*************************************
	** Pressure related coefficients    **
	*************************************/
	_P1 =
		(uint16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_P1_MSB_REG],
								coeff_arr1[RAK1906_P1_LSB_REG]));
	_P2 =
		(int16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_P2_MSB_REG],
							   coeff_arr1[RAK1906_P2_LSB_REG]));
	_P3 = (int8_t)coeff_arr1[RAK1906_P3_REG];
	_P4 =
		(int16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_P4_MSB_REG],
							   coeff_arr1[RAK1906_P4_LSB_REG]));
	_P5 =
		(int16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_P5_MSB_REG],
							   coeff_arr1[RAK1906_P5_LSB_REG]));
	_P6 = (int8_t)(coeff_arr1[RAK1906_P6_REG]);
	_P7 = (int8_t)(coeff_arr1[RAK1906_P7_REG]);
	_P8 =
		(int16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_P8_MSB_REG],
							   coeff_arr1[RAK1906_P8_LSB_REG]));
	_P9 =
		(int16_t)(CONCAT_BYTES(coeff_arr1[RAK1906_P9_MSB_REG],
							   coeff_arr1[RAK1906_P9_LSB_REG]));
	_P10 = (uint8_t)(coeff_arr1[RAK1906_P10_REG]);
	/**********************************
	** Humidity related coefficients **
	**********************************/
	_H1 = (uint16_t)(((uint16_t)coeff_arr2[RAK1906_H1_MSB_REG] << RAK1906_HUM_REG_SHIFT_VAL) |
					 ((coeff_arr2[RAK1906_H1_LSB_REG] >>
					   RAK1906_HUM_REG_SHIFT_VAL) &
					  RAK1906_BIT_H1_DATA_MSK));
	_H2 =
		(uint16_t)(((uint16_t)coeff_arr2[RAK1906_H2_MSB_REG] << RAK1906_HUM_REG_SHIFT_VAL) |
				   ((coeff_arr2[RAK1906_H2_LSB_REG] >>
					 RAK1906_HUM_REG_SHIFT_VAL) &
					RAK1906_BIT_H1_DATA_MSK));
	_H3 = (int8_t)coeff_arr2[RAK1906_H3_REG];
	_H4 = (int8_t)coeff_arr2[RAK1906_H4_REG];
	_H5 = (int8_t)coeff_arr2[RAK1906_H5_REG];
	_H6 = (uint8_t)coeff_arr2[RAK1906_H6_REG];
	_H7 = (int8_t)coeff_arr2[RAK1906_H7_REG];
	/************************************
	** Gas heater related coefficients **
	************************************/
	_G1 = (int8_t)coeff_arr2[RAK1906_GH1_REG];
	_G2 =
		(int16_t)(CONCAT_BYTES(coeff_arr2[RAK1906_GH2_MSB_REG],
							   coeff_arr2[RAK1906_GH2_LSB_REG]));
	_G3 = (int8_t)coeff_arr2[RAK1906_GH3_REG];
	uint8_t temp_var = 0;
	getData(RAK1906_ADDR_RES_HEAT_RANGE_ADDR, temp_var);
	_res_heat_range = ((temp_var & RAK1906_RHRANGE_MSK) / 16);
	getData(RAK1906_ADDR_RES_HEAT_VAL_ADDR, temp_var);
	_res_heat = (int8_t)temp_var;
	getData(RAK1906_ADDR_RANGE_SW_ERR_ADDR, temp_var);
	_rng_sw_err = ((int8_t)temp_var & (int8_t)RAK1906_RSERROR_MSK) / 16;
}

void rak1906::waitForReadings() const
{
	while (measuring())
	{
	} // loop until any active measurment is complete
} // of method waitForReadings

bool rak1906::measuring() const
{
	bool result(false);
	if ((readByte(RAK1906_STATUS_REGISTER) &
		 _BV(RAK1906_MEASURING_BIT_POSITION)) != 0)
	{
		result = true;
	} // if-then device is currently measuring
	return result;
} // of method "measuring()"

void rak1906::triggerMeasurement() const
{
	uint8_t workRegister = readByte(RAK1906_CONTROL_MEASURE_REGISTER);		// Read the control measure
	putData(RAK1906_CONTROL_MEASURE_REGISTER, (uint8_t)(workRegister | 1)); // Trigger start of next measurement
} // of method "triggerMeasurement()"
