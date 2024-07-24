/**
 * @file gnss.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief GNSS functions
 * @version 0.1
 * @date 2024-06-22
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"

#include "SparkFun_Ublox_Arduino_Library_Series_6_7.h" // Included in Source Code
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>	   // Click here to get the library: http://librarymanager/All#SparkFun_u-blox_GNSS

// /** Instance for RAK1910 GNSS sensor */
SFE_UBLOX_GNSS_M7 my_rak1910_gnss;
/** Instance for RAK12500 GNSS sensor */
SFE_UBLOX_GNSS my_gnss;

// Fake GPS Enable (1) Disable (0)
#define FAKE_GPS 0

// Enable Serial debug output
#define GNSS_DEBUG

/** GNSS polling function */
bool poll_gnss(void);

/** Flag if location was found */
volatile bool last_read_ok = false;

/** The GPS module to use */
uint8_t g_gnss_option = 0;

/**
 * @brief Initialize GNSS module
 *
 * @return true if GNSS module was found
 * @return false if no GNSS module was found
 */
bool init_gnss(void)
{
	// Power on the GNSS module
	digitalWrite(WB_IO2, HIGH);

	// Give the module some time to power up
	delay(500);

	if (g_gnss_option == NO_GNSS_INIT)
	{
		// Check for I2C GNSS
		Wire.begin();
		if (!my_gnss.begin(Wire))
		{
			MYLOG("GNSS", "RAK12500 fail");
		}
		else
		{
			MYLOG("GNSS", "RAK12500 ok");
			my_gnss.setI2COutput(COM_TYPE_UBX_M7); // Set the I2C port to output UBX only (turn off NMEA noise)
			g_gnss_option = RAK12500_GNSS;

			my_gnss.saveConfiguration(); // Save the current settings to flash and BBR

			my_gnss.setMeasurementRate(500);

			return true;
		}

		// No RAK12500 found, check if RAK1910 is plugged in
		// my_rak1910_gnss.enableDebugging(); // Uncomment this line to enable debug messages

		Serial1.begin(9600);
		if (!my_rak1910_gnss.begin(Serial1))
		{
			MYLOG("GNSS", "RAK1910 fail");
		}
		else
		{
			MYLOG("GNSS", "RAK1910 ok");
			if (!my_rak1910_gnss.setUART1Output(COM_TYPE_UBX_M7)) // Set the UART port to output UBX only (turn off NMEA noise)
			{
				MYLOG("GNSS", "Can't force UBX");
			}
			g_gnss_option = RAK1910_GNSS;

			my_rak1910_gnss.saveConfiguration(); // Save the current settings to flash and BBR

			my_rak1910_gnss.setNavigationFrequency(500);

			return true;
		}
		Serial1.end();
		return false;
	}
	else // Already searched and found GNSS module
	{
		// RAK12500 module
		if (g_gnss_option == RAK12500_GNSS)
		{
			my_gnss.begin(Wire);

			my_gnss.setI2COutput(COM_TYPE_UBX_M7); // Set the I2C port to output UBX only (turn off NMEA noise)

			my_gnss.saveConfiguration(); // Save the current settings to flash and BBR

			my_gnss.setMeasurementRate(500);
		}
		// RAK1910 module
		else
		{
			Serial1.begin(9600);

			my_rak1910_gnss.begin(Serial1);

			my_rak1910_gnss.setUART1Output(COM_TYPE_UBX_M7); // Set the UART port to output UBX only (turn off NMEA noise)

			my_rak1910_gnss.saveConfiguration(); // Save the current settings to flash and BBR

			my_rak1910_gnss.setNavigationFrequency(500);
		}
		return true;
	}
	return false;
}

/**
 * @brief Check GNSS module for position
 *
 * @return true Valid position found
 * @return false No valid position
 */
bool poll_gnss(void)
{
	last_read_ok = false;

	time_t time_out = millis();
	int64_t latitude = 0;
	int64_t longitude = 0;
	int32_t altitude = 0;
	int16_t accuracy = 0;
	uint8_t satellites = 0;

	time_t check_limit = custom_parameters.send_interval / 2;

	bool has_pos = false;
	bool has_alt = false;
	byte fix_type;

	// RAK12500 module
	if (g_gnss_option == RAK12500_GNSS)
	{
		if (my_gnss.getGnssFixOk())
		{
			fix_type = my_gnss.getFixType(); // Get the fix type
			char fix_type_str[32] = {0};
			if (fix_type == 0)
				sprintf(fix_type_str, "No Fix");
			else if (fix_type == 1)
				sprintf(fix_type_str, "Dead reck");
			else if (fix_type == 2)
				sprintf(fix_type_str, "Fix 2D");
			else if (fix_type == 3)
				sprintf(fix_type_str, "Fix 3D");
			else if (fix_type == 4)
				sprintf(fix_type_str, "GNSS fix");
			else if (fix_type == 5)
				sprintf(fix_type_str, "Time fix");

			satellites = my_gnss.getSIV();
			if ((fix_type >= 3) && (satellites >= 5)) /** Fix type 3D and at least 5 satellites */
													  // if (fix_type >= 3) /** Fix type 3D */
			{
				last_read_ok = true;
				latitude = my_gnss.getLatitude();
				longitude = my_gnss.getLongitude();
				altitude = my_gnss.getAltitude();
				accuracy = my_gnss.getHorizontalDOP();

				// MYLOG("GNSS", "Fixtype: %d %s", my_gnss.getFixType(), fix_type_str);
				// MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
				// MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
				// MYLOG("GNSS", "Acy: %.2f ", accuracy / 100.0);
				// MYLOG("GNSS", "Sat: %d ", satellites);
			}
		}
	}
	// RAK1910 module
	else
	{
		// if (my_rak1910_gnss.getGnssFixOk())
		{
			fix_type = my_rak1910_gnss.getFixType(); // Get the fix type
			char fix_type_str[32] = {0};
			if (fix_type == 0)
				sprintf(fix_type_str, "No Fix");
			else if (fix_type == 1)
				sprintf(fix_type_str, "Dead reck");
			else if (fix_type == 2)
				sprintf(fix_type_str, "Fix 2D");
			else if (fix_type == 3)
				sprintf(fix_type_str, "Fix 3D");
			else if (fix_type == 4)
				sprintf(fix_type_str, "GNSS fix");
			else if (fix_type == 5)
				sprintf(fix_type_str, "Time fix");

			satellites = my_rak1910_gnss.getSIV();
			if ((fix_type >= 3) && (satellites >= 5)) /** Fix type 3D and at least 5 satellites */
													  // if (fix_type >= 3) /** Fix type 3D */
			{
				last_read_ok = true;
				latitude = my_rak1910_gnss.getLatitude();
				longitude = my_rak1910_gnss.getLongitude();
				altitude = my_rak1910_gnss.getAltitude();
				accuracy = my_rak1910_gnss.getPDOP();

				// MYLOG("GNSS", "Fixtype: %d %s", my_rak1910_gnss.getFixType(), fix_type_str);
				// MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
				// MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
				// MYLOG("GNSS", "Acy: %.2f ", accuracy / 100.0);
				// MYLOG("GNSS", "Sat: %d ", satellites);
			}
		}
	}

	char disp_str[255];
	if (last_read_ok)
	{
		if ((latitude == 0) && (longitude == 0))
		{
			last_read_ok = false;
			return false;
		}

		g_solution_data.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
		g_solution_data.addDigitalInput(LPP_CHANNEL_GPS_ACCURACY, accuracy);
		g_solution_data.addDigitalInput(LPP_CHANNEL_GPS_SAT, satellites);
		g_solution_data.addDigitalInput(LPP_CHANNEL_GPS_FIX, fix_type);

		return true;
	}
	else
	{
		// No location found
#if FAKE_GPS > 0
		// MYLOG("GNSS", "Faking GPS");
		// 14.4213730, 121.0069140, 35.000
		latitude = 144213730;
		longitude = 1210069140;
		altitude = 35000;
		accuracy = 1;
		satellites = 5;

		g_solution_data.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
		g_solution_data.addDigitalInput(LPP_CHANNEL_GPS_ACCURACY, accuracy);
		g_solution_data.addDigitalInput(LPP_CHANNEL_GPS_SAT, satellites);
		last_read_ok = true;
		return true;
#endif
	}
	// MYLOG("GNSS", "No valid location found");
	last_read_ok = false;
	return false;
}
