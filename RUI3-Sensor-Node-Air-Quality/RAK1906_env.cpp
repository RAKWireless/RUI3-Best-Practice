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

// Enable/Disable gas resistance measurements 0 = off, 1 = on
#define USE_GAS 0

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)

// Smaller library used for RAK3172 versions to reduce code size
#include "rak1906.h"

#pragma message("RAK3172")

/** BME680 instance for Wire */
rak1906 bme;

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
		MYLOG("BME", "Could not find a valid BME680 sensor, check wiring!");
		return false;
	}

	// Set up oversampling and filter initialization
	/// \todo Needs to be implemented in the RUI3 RAK1906 library!!!!
	bme.setOversampling(TemperatureSensor, Oversample8);
	bme.setOversampling(HumiditySensor, Oversample2);
	bme.setOversampling(PressureSensor, Oversample4);
	bme.setIIRFilter(IIR4);
#if USE_GAS == 0
	bme.setGas(0, 0); // off
#else
	bme.setGas(320, 150); // 320*C for 150 ms
#endif

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
	MYLOG("BME", "Reading BME680");
	if (!bme.update())
	{
		MYLOG("BME", "BME reading timeout");
		return false;
	}

#if MY_DEBUG > 0
	int16_t temp_int = (int16_t)(bme.temperature() * 10.0);
	uint16_t humid_int = (uint16_t)(bme.humidity() * 2);
	uint16_t press_int = (uint16_t)(bme.pressure() * 100);
	uint16_t gasres_int = (uint16_t)(bme.gas() / 10);
#endif

	g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID_2, bme.humidity());
	g_solution_data.addTemperature(LPP_CHANNEL_TEMP_2, bme.temperature());
	g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS_2, bme.pressure());
#if USE_GAS == 1
	g_solution_data.addAnalogInput(LPP_CHANNEL_GAS_2, (float)(bme.gas()) / 1000.0);
#endif

#if MY_DEBUG > 0
#if USE_GAS == 0
	MYLOG("BME", "RH= %.2f T= %.2f P= %.2f", bme.humidity(), bme.temperature(), bme.pressure());
#else
	MYLOG("BME", "RH= %.2f T= %.2f", bme.humidity(), bme.temperature());
	MYLOG("BME", "P= %.2f R= %.2f", bme.pressure(), (float)(bme.gas()) / 1000.0);
#endif
#endif

	g_last_humid = bme.humidity();
	g_last_temp = bme.temperature();
	g_has_last_values = true;

	return true;
}

/**
 * @brief Returns the latest values from the sensor
 *        or starts a new reading
 *
 * @param values array for temperature [0] and humidity [1]
 */
void get_rak1906_values(float *values)
{
	if (g_has_last_values)
	{
		// Discard old values after 10 VOC readings
		t_h_discard_counter++;
		if (t_h_discard_counter == 9)
		{
			t_h_discard_counter = 0;
			g_has_last_values = false;
		}
		values[0] = g_last_temp;
		values[1] = g_last_humid;
		return;
	}
	else
	{
		// start_rak1906();
		// delay(100);
		read_rak1906();
		values[0] = g_last_temp;
		values[1] = g_last_humid;
	}
	return;
}

#else // _VARIANT_RAK3172_ || _VARIANT_RAK3172_SIP_

// Adafruit BME680 library used for other RUI3 modules
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

/** BME680 instance for Wire */
Adafruit_BME680 bme(&Wire);

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

	if (!bme.begin(0x76))
	{
		// MYLOG("BME", "Could not find a valid BME680 sensor, check wiring!");
		return false;
	}

	// Set up oversampling and filter initialization
	bme.setTemperatureOversampling(BME680_OS_8X);
	bme.setHumidityOversampling(BME680_OS_2X);
	bme.setPressureOversampling(BME680_OS_4X);
	bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
#if USE_GAS == 0
	// Switch off the Gas Heater
	bme.setGasHeater(0, 0); // off
#else
	// Switch on the Gas Heater
	bme.setGasHeater(320, 150); // 320*C for 150 ms
#endif
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
	bme.beginReading();
	time_t wait_start = millis();
	bool read_success = false;
	while ((millis() - wait_start) < 5000)
	{
		if (bme.endReading())
		{
			read_success = true;
			break;
		}
	}

	if (!read_success)
	{
		// MYLOG("BME", "BME reading timeout");
		return false;
	}

	MYLOG("BME", "Temperature: %.2f", bme.humidity);
	MYLOG("BME", "Humidity: %.2f", bme.humidity);
	MYLOG("BME", "Barometer: %.2f", bme.pressure / 100.0);
#if USE_GAS == 1
	MYLOG("BME", "Gas resistance: %.2f", (float)(bme.gas_resistance / 1000.0));
#endif
	g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID_2, bme.humidity);
	g_solution_data.addTemperature(LPP_CHANNEL_TEMP_2, bme.temperature);
	g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS_2, bme.pressure / 100.0);
#if USE_GAS == 1
	g_solution_data.addAnalogInput(LPP_CHANNEL_GAS_2, (float)(bme.gas_resistance) / 1000.0);
#endif
	return true;
}

/**
 * @brief Returns the latest values from the sensor
 *        or starts a new reading
 *
 * @param values array for temperature [0] and humidity [1]
 */
void get_rak1906_values(float *values)
{
	values[0] = bme.temperature;
	values[1] = bme.humidity;
	return;
}

#endif // _VARIANT_RAK3172_ || _VARIANT_RAK3172_SIP_