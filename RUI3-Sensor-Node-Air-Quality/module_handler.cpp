/**
 * @file module_handler.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Find and handle WisBlock sensor modules
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "app.h"
#include "module_handler.h"

bool has_rak1901 = false;
bool has_rak1906 = false;
bool has_rak12037 = false;
bool has_rak12047 = false;

/**
 * @brief Scan both I2C bus for devices
 *
 */
void find_modules(void)
{
	// Scan the I2C interfaces for devices
	byte error;

	Wire.begin();
	Wire.setClock(400000);
	for (byte address = 1; address < 127; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			MYLOG("SCAN", "Found sensor on I2C1 0x%02X\n", address);
			switch (address)
			{
			case 0x70:
				has_rak1901 = true;
				break;
			case 0x76:
				has_rak1906 = true;
				break;
			case 0x61:
				has_rak12037 = true;
				break;
			case 0x59:
				has_rak12047 = true;
				break;
			}
		}
	}

	if (has_rak1901)
	{
		if (init_rak1901())
		{
			sprintf(g_dev_name, "RUI3 Env Sensor");
		}
		else
		{
			has_rak1901 = false;
		}
	}

	if (has_rak12037)
	{
		if (init_rak12037())
		{
			sprintf(g_dev_name, "RUI3 CO2 Sensor");
		}
		else
		{
			has_rak12037 = false;
		}
	}

	if (has_rak12047)
	{
		if (init_rak12047())
		{
			sprintf(g_dev_name, "RUI3 VOC Sensor");
		}
		else
		{
			has_rak12047 = false;
		}
	}

	if (has_rak1906 && has_rak12037 && has_rak12047)
	{
		sprintf(g_dev_name, "RUI3 IAQ Sensor");
	}
}

/**
 * @brief AT command feedback about found modules
 *
 */
void announce_modules(void)
{
	if (has_rak1901)
	{
		Serial.println("+EVT:RAK1901");
		// Reading sensor data
		read_rak1901();
	}

	if (has_rak1906)
	{
		Serial.println("+EVT:RAK1906");
		// Reading sensor data
		read_rak1906();
	}

	if (has_rak12037)
	{
		Serial.println("+EVT:RAK12037");
		// Reading sensor data
		read_rak12037();
	}

	if (has_rak12047)
	{
		Serial.println("+EVT:RAK12047");
		// Sensor needs 100 readings before valid data is available.
		// Makes no sense to read it already.
	}
}

/**
 * @brief Read values from the found modules
 *
 */
void get_sensor_values(void)
{
	if (has_rak1901)
	{
		// Reading sensor data
		read_rak1901();
	}

	if (has_rak12037)
	{
		// Read sensor data
		read_rak12037();
	}

	if (has_rak12047)
	{
		// Read sensor data
		get_rak12047_value();
	}
}