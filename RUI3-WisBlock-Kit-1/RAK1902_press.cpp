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
	if (!lps.begin(&Wire))
	{
		MYLOG("PRESS", "Could not initialize LPS2X on Wire");
		return false;
	}

	lps.setLowPower(true);
	lps.setOutputRate(LPS35HW::OutputRate_75Hz);	   // 75 Hz sample rate
	lps.setLowPassFilter(LPS35HW::LowPassFilter_ODR9); // default is off

	shutdown_rak1902();
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

	// Give the sensor some time to read
	delay(100);
	float pressure = lps.readPressure(); // hPa

	// Switch back to low power mode
	lps.setLowPower(true);
	lps.setOutputRate(LPS35HW::OutputRate_OneShot); // 75 Hz sample rate

	MYLOG("PRESS", "P: %.2f", pressure);

	g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS, pressure);

#if HAS_EPD > 0
	set_baro_rak14000(pressure);
#endif
}

/**
 * @brief Wake up RAK1902 from sleep
 *
 */
void startup_rak1902(void)
{
	lps.setLowPower(false);						 // Disable low power mode
	lps.setOutputRate(LPS35HW::OutputRate_75Hz); // 75 Hz sample rate
}

/**
 * @brief Put the RAK1902 into sleep mode
 *
 */
void shutdown_rak1902(void)
{
	lps.setOutputRate(LPS35HW::OutputRate_OneShot); // Oneshot
	lps.setLowPower(true);
}
