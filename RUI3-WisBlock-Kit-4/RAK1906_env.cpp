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
// #include <ClosedCube_BME680.h>

// // Enable/Disable gas resistance measurements 0 = off, 1 = on
// #define USE_GAS 0

// /** BME680 instance for Wire */
// ClosedCube_BME680 bme;

// /**
//  * @brief Initialize the BME680 sensor
//  *
//  * @return true if sensor was found
//  * @return false if sensor was not found
//  */
// bool init_rak1906(void)
// {
// 	Wire.begin();

// 	bme.init(0x76);
// 	bme.reset();

// 	MYLOG("BME", "ChipID %02X", bme.getChipID());

// 	if (bme.getChipID() == 0x00)
// 	{
// 		MYLOG("BME", "Could not find a valid BME680 sensor, check wiring!");
// 		return false;
// 	}

// 	// Set up oversampling and filter initialization
// 	/// \todo Needs to be implemented in the RUI3 RAK1906 library!!!!
// 	bme.setOversampling(BME680_OVERSAMPLING_X2, BME680_OVERSAMPLING_X8, BME680_OVERSAMPLING_X4);
// 	bme.setIIRFilter(BME680_FILTER_3);
// #if USE_GAS == 0
// 	bme.setGasOff(); // off
// #else
// 	bme.setGasOn(320, 150); // 320*C for 150 ms
// #endif

// 	bme.setSleepMode();
// 	return true;
// }

// /**
//  * @brief Read environment data from BME680
//  *     Data is added to Cayenne LPP payload as channels
//  *     LPP_CHANNEL_HUMID_2, LPP_CHANNEL_TEMP_2,
//  *     LPP_CHANNEL_PRESS_2 and LPP_CHANNEL_GAS_2
//  *
//  */
// void read_rak1906()
// {
// 	MYLOG("BME", "Reading BME680");
// 	time_t start_time = millis();
// 	bool got_data = false;
// 	ClosedCube_BME680_Status status;
// 	while (1)
// 	{
// 		status = bme.readStatus();
// 		if (status.newDataFlag)
// 		{
// 			double temp = bme.readTemperature();
// 			double pres = bme.readPressure();
// 			double hum = bme.readHumidity();
// #if USE_GAS == 1
// 			uint32_t gas = bme.readGasResistance();
// #endif
// 			g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID_2, hum);
// 			g_solution_data.addTemperature(LPP_CHANNEL_TEMP_2, temp);
// 			g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS_2, pres);
// #if USE_GAS == 1
// 			g_solution_data.addAnalogInput(LPP_CHANNEL_GAS_2, (float)(gas) / 1000.0);
// #endif

// #if USE_GAS == 0
// 			MYLOG("BME", "RH= %.2f T= %.2f P= %.2f", hum, temp, pres);
// #else
// 			MYLOG("BME", "RH= %.2f T= %.2f", hum, btemp);
// 			MYLOG("BME", "P= %.2f R= %.2f", pres, (float)(gas) / 1000.0);
// #endif
// 			got_data = true;
// 			break;
// 		}
// 		if ((millis() - start_time) > 5000)
// 		{
// 			break;
// 		}
// 	}

// 	if (!got_data)
// 	{
// 		MYLOG("BME", "BME reading timeout");
// 		return;
// 	}
// 	return;
// }

// void startup_rak1906(void)
// {
// 	bme.setForcedMode();
// }

// void shutdown_rak1906(void)
// {
// 	bme.setSleepMode();
// }

#include "rak1906.h"

// Enable/Disable gas resistance measurements 0 = off, 1 = on
#define USE_GAS 0

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
 */
void read_rak1906()
{
	MYLOG("BME", "Reading BME680");
	if (!bme.update())
	{
		MYLOG("BME", "BME reading timeout");
		return;
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
	return;
}

void startup_rak1906(void)
{
	// Handled in update()
}

void shutdown_rak1906(void)
{
	// Handled in update()
}
