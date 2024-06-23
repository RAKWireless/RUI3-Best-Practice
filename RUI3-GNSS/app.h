/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Defines, includes, global definitions
 * @version 0.1
 * @date 2023-05-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <Arduino.h>
#include "wisblock_cayenne.h"

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif

// Forward declarations
void send_packet(void);
bool init_status_at(void);
bool init_interval_at(void);
bool get_at_setting(void);
bool save_at_setting(void);

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

// LoRaWAN stuff
extern WisCayenne g_solution_data;
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1			// Base Board
#define LPP_CHANNEL_GPS 10			// RAK1910/RAK12500
#define LPP_CHANNEL_GPS_ACCURACY 11 // RAK1910/RAK12500
#define LPP_CHANNEL_GPS_SAT 12		// RAK1910/RAK12500
#define LPP_CHANNEL_GPS_FIX 13		// RAK1910/RAK12500
// GNSS functions
// GNSS functions
#define NO_GNSS_INIT 0
#define RAK1910_GNSS 1
#define RAK12500_GNSS 2

bool init_gnss(void);
bool poll_gnss(void);
extern time_t check_gnss_max_try;
extern uint8_t g_gnss_option;
