/**
 * @file RAK1903_light.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialize and read data from OPT3001 sensor
 * @version 0.2
 * @date 2022-01-30
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "../inc/app.h"
#include <rak1903.h> // Install manually from https://downloads.rakwireless.com/#RUI/RUI3/Library/ ==> RAKwireless_RAK1903_Optical_OPT3001.zip

/** Sensor instance */
rak1903 rak1903;

/**
 * @brief Initialize the Light sensor
 *
 * @return true if sensor found and configuration success
 * @return false if error occured
 */
bool init_rak1903(void)
{
	Wire.begin();
	if (!rak1903.init())
	{
		MYLOG("LIGHT", "Could not initialize OPT3001");
		return false;
	}
	return true;
}

/**
 * @brief Read value from light sensor
 *     Data is added to Cayenne LPP payload as channel
 *     LPP_CHANNEL_LIGHT
 *
 */
void read_rak1903()
{
	// MYLOG("LIGHT", "Reading OPT3001");
	if (rak1903.update())
	{
		uint16_t light_int = (uint16_t)(rak1903.lux());

		// MYLOG("LIGHT", "L: %.2f", (float)light_int / 1.0);

		g_solution_data.addLuminosity(LPP_CHANNEL_LIGHT, (uint32_t)(light_int));

		set_light_rak14000(light_int);
	}
	// else
	// {
	// 	MYLOG("LIGHT", "Error reading OPT3001");
	// 	g_solution_data.addLuminosity(LPP_CHANNEL_LIGHT, 0);
	// }
}
