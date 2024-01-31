/**
 * @file RAK1906_env.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief BME680 sensor functions
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

#include "rak1906.h" // Install manually from https://downloads.rakwireless.com/#RUI/RUI3/Library/ ==> RAKwireless_RAK1906_Environment_BME680.zip

/** BME680 instance for Wire */
rak1906 bme;

// /** Mean Sea Level Pressure */
// float mean_seal_level_press = 1013.25;

/**
 * @brief Initialize the BME680 sensor
 *
 * @return true if sensor was found
 * @return false if sensor was not found
 */
bool init_rak1906(void)
{
	Wire.begin();

	if (!bme.init())
	{
		// MYLOG("BME", "Could not find a valid BME680 sensor, check wiring!");
		return false;
	}
	// MYLOG("BME", "BME680 sensor ok");

	bme.setOversampling(TemperatureSensor, Oversample2); // Set to low oversampling rate for lower power consumption
	bme.setOversampling(HumiditySensor, Oversample2);	  // Set to low oversampling rate for lower power consumption
	bme.setOversampling(PressureSensor, Oversample2);	  // Set to low oversampling rate for lower power consumption
	bme.setIIRFilter(IIR4);								  // Set IIR filter
	bme.setGas(0, 0);									  // disable gas sensor for lower power consumption
	return true;
}

/**
 * @brief Read environment data from BME680
 *     Data is added to Cayenne LPP payload as channels
 *     LPP_CHANNEL_HUMID_2, LPP_CHANNEL_TEMP_2,
 *     LPP_CHANNEL_PRESS_2 and LPP_CHANNEL_GAS_2
 *
 *
 * @return true if reading was successful
 * @return false if reading failed
 */
bool read_rak1906()
{
	// MYLOG("BME", "Reading BME680");

	if (bme.update())
	{
		g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID_2, bme.humidity());
		g_solution_data.addTemperature(LPP_CHANNEL_TEMP_2, bme.temperature());
		g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS_2, bme.pressure());
		// g_solution_data.addAnalogInput(LPP_CHANNEL_GAS_2, (float)(bme.gas()) / 1000.0);
		// MYLOG("BME", "Barometric pressure: %.2f", bme.pressure());

		set_humid_rak14000(bme.humidity());
		set_temp_rak14000(bme.temperature());
		set_baro_rak14000(bme.pressure());
		return true;
	}
	else
	{
		return false;
	}
}
