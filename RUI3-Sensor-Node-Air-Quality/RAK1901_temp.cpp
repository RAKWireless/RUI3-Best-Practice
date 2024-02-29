/**
 * @file RAK1901_temp.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialize and read data from SHTC3 sensor
 * @version 0.1
 * @date 2022-04-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include <SparkFun_SHTC3.h>

/** Sensor instance */
SHTC3 shtc3;

/**
 * @brief Initialize the temperature and humidity sensor
 *
 * @return true if initialization is ok
 * @return false if sensor could not be found
 */
bool init_rak1901(void)
{
	if (shtc3.begin(Wire) != SHTC3_Status_Nominal)
	{
		MYLOG("T_H", "Could not initialize SHTC3");
		return false;
	}
	shtc3.sleep(true);

	return true;
}

/**
 * @brief Read the temperature and humidity values
 *     Data is added to Cayenne LPP payload as channel
 *     LPP_CHANNEL_HUMID, LPP_CHANNEL_TEMP
 *
 */
void read_rak1901(void)
{
	MYLOG("T_H", "Reading SHTC3");
	shtc3.wake(false);

	shtc3.update();

	if (shtc3.lastStatus == SHTC3_Status_Nominal)
	{
		int16_t temp_int = (int16_t)(shtc3.toDegC() * 10.0);
		uint16_t humid_int = (uint16_t)(shtc3.toPercent() * 2);

		MYLOG("T_H", "T: %.2f H: %.2f", (float)temp_int / 10.0, (float)humid_int / 2.0);

		g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID, shtc3.toPercent());
		g_solution_data.addTemperature(LPP_CHANNEL_TEMP, shtc3.toDegC());
		g_last_temp = shtc3.toDegC();
		g_last_humid = shtc3.toPercent();
		g_has_last_values = true;

#if HAS_EPD > 0
		set_humid_rak14000(shtc3.toPercent());
		set_temp_rak14000(shtc3.toDegC());
#endif
	}
	else
	{
		MYLOG("T_H", "Reading SHTC3 failed");
		g_last_temp = 0.0;
		g_last_humid = 0.0;
	}

	shtc3.sleep(true);
}

/**
 * @brief Returns the latest values from the sensor
 *        or starts a new reading
 *
 * @param values array for temperature [0] and humidity [1]
 */
void get_rak1901_values(float *values)
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
		read_rak1901();
		values[0] = g_last_temp;
		values[1] = g_last_humid;
	}
	return;
}
