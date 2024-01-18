/**
 * @file RAK1902_press.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialize and read values from the LPS22HB sensor
 * @version 0.1
 * @date 2022-04-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include <LPS35HW.h>

/** Sensor instance */
LPS35HW lps;

/**
 * @brief Initialize barometric pressure sensor
 *
 * @return true if sensor was found
 * @return false if initialization failed
 */
bool init_rak1902(void)
{
	Wire.begin();
	if (!lps.begin())
	{
		MYLOG("PRESS", "Could not initialize LPS2X on Wire");
		return false;
	}

	lps.setLowPower(true);
	lps.setOutputRate(LPS35HW::OutputRate_OneShot);	   // get results on demand
	lps.setLowPassFilter(LPS35HW::LowPassFilter_ODR9); // default is off
	return true;
}

/**
 * @brief Read the barometric pressure
 *     Data is added to Cayenne LPP payload as channel
 *     LPP_CHANNEL_PRESS
 *
 */
void read_rak1902(void)
{
	MYLOG("PRESS", "Reading LPS22HB");

	lps.requestOneShot(); // important to request new data before reading
	delay(1000);		  // Give the sensor some time

	float pressure = lps.readPressure(); // hPa

	MYLOG("PRESS", "P: %.2f", pressure);

	g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS, pressure);
}
