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
		Serial.flush();                  \
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
bool init_iaq_interval_at(void);
void sensor_handler(void *);

extern uint8_t sync_time_status;

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint32_t iaq_interval = 10000;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1	  // Base Board
#define LPP_CHANNEL_HUMID_2 6 // RAK1906
#define LPP_CHANNEL_TEMP_2 7  // RAK1906
#define LPP_CHANNEL_PRESS_2 8 // RAK1906
#define LPP_CHANNEL_GAS_2 9	  // RAK1906

// RAK1906
bool init_rak1906(void);
void read_bme680(void *);
void GetGasReference();
extern bool has_rak1906;
extern volatile float g_last_temperature;
extern volatile float g_last_humidity;
extern volatile float g_last_pressure;
extern volatile float g_last_gas;
extern volatile float g_last_air_quality;
extern float volatile g_last_iaq;
