/**
 * @file RAK12500_gnss.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization and usage of RAK12500
 * @version 0.1
 * @date 2024-01-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"

#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

/** Instance for RAK12500 GNSS sensor */
SFE_UBLOX_GNSS my_gnss;

// GNSS functions
#define NO_GNSS_INIT 0
#define RAK1910_GNSS 1
#define RAK12500_GNSS 2

// Fake GPS Enable (1) Disable (0)
#define FAKE_GPS 0

/** GNSS polling function */
bool poll_gnss(void);

/** Flag if location was found */
volatile bool last_read_ok = false;

/** Flag if GNSS is serial or I2C */
bool i2c_gnss = false;

/** The GPS module to use */
uint8_t g_gnss_option = NO_GNSS_INIT;

/** Start time of location acquisition */
time_t gnss_start;
/** End time of location acquisition */
time_t gnss_finished;

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

	if (!my_gnss.begin(Wire))
	{
		MYLOG("GNSS", "Could not initialize RAK12500 on Wire");
		return false;
	}

	MYLOG("GNSS", "Found RAK12500 on Wire");

	my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

	my_gnss.saveConfiguration(); // Save the current settings to flash and BBR

	my_gnss.setMeasurementRate(500);

	return true;
}

/**
 * @brief GNSS location acquisition
 * Called every 2.5 seconds by timer 1
 * Gives up after 1/2 of send interval
 * or when location was aquired
 *
 */
void gnss_handler(void *)
{
	digitalWrite(LED_GREEN, HIGH);
	if (poll_gnss())
	{
		// Power down the module
		digitalWrite(WB_IO2, LOW);
		delay(100);
		MYLOG("GNSS", "Got location");
		api.system.timer.stop(RAK_TIMER_1);
		gnss_finished = millis();
		digitalWrite(LED_BLUE, HIGH);
		send_packet();
	}
	else
	{
		if (check_gnss_counter >= check_gnss_max_try)
		{
			// Power down the module
			digitalWrite(WB_IO2, LOW);
			delay(100);
			MYLOG("GNSS", "Location timeout");
			api.system.timer.stop(RAK_TIMER_1);
			gnss_finished = millis();
			digitalWrite(LED_BLUE, HIGH);
			send_packet();
		}
	}
	check_gnss_counter++;
	digitalWrite(LED_GREEN, LOW);
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
	uint32_t unix_time = 0;

	bool has_pos = false;
	bool has_alt = false;

	if (my_gnss.getGnssFixOk())
	{
		byte fix_type = my_gnss.getFixType(); // Get the fix type
		char fix_type_str[32] = {0};
#if MY_DEBUG > 0
		if (fix_type == 0)
			sprintf(fix_type_str, "No Fix");
		else if (fix_type == 1)
			sprintf(fix_type_str, "Dead reckoning");
		else if (fix_type == 2)
			sprintf(fix_type_str, "Fix type 2D");
		else if (fix_type == 3)
			sprintf(fix_type_str, "Fix type 3D");
		else if (fix_type == 4)
			sprintf(fix_type_str, "GNSS fix");
		else if (fix_type == 5)
			sprintf(fix_type_str, "Time fix");
#endif
		if ((fix_type >= 3) && (my_gnss.getSIV() >= 5)) /** Fix type 3D and at least 5 satellites */
														// if (fix_type >= 3) /** Fix type 3D */
		{
			last_read_ok = true;
			latitude = my_gnss.getLatitude();
			longitude = my_gnss.getLongitude();
			altitude = my_gnss.getAltitude();
			accuracy = my_gnss.getHorizontalDOP();
			satellites = my_gnss.getSIV();
			unix_time = my_gnss.getUnixEpoch();
			// MYLOG("GNSS", "Fixtype: %d %s", my_gnss.getFixType(), fix_type_str);
			// MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
			// MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
			// MYLOG("GNSS", "Acy: %.2f ", accuracy / 100.0);
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

		// Add the location to the payload
#if USE_GNSS6_ACCURACY
		g_solution_data.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
#else
        g_solution_data.addGNSS_4(LPP_CHANNEL_GPS, latitude, longitude, altitude);
#endif

#if ADD_UNIX_TIMESTAMP
		g_solution_data.addUnixTime(LPP_CHANNEL_TIME, unix_time);
#endif
		return true;
	}
	else
	{
		// No location found
#if FAKE_GPS > 0
		MYLOG("GNSS", "Faking GPS");
		// 14.4213730, 121.0069140, 35.000
		latitude = 144213730;
		longitude = 1210069140;
		altitude = 35000;
		accuracy = 1;
		satellites = 18;

#if USE_GNSS6_ACCURACY
		g_solution_data.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
#else
		g_solution_data.addGNSS_4(LPP_CHANNEL_GPS, latitude, longitude, altitude);
#endif

#if ADD_UNIX_TIMESTAMP
		g_solution_data.addUnixTime(LPP_CHANNEL_TIME, millis() / 1000);
#endif
		last_read_ok = true;
		return true;
#endif
	}

	// MYLOG("GNSS", "No valid location found");
	last_read_ok = false;
	return false;
}
