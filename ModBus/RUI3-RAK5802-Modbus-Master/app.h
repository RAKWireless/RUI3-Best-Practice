/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and defines
 * @version 0.1
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <Arduino.h>
#include "RUI3_ModbusRtu.h"

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

struct sensor_data_s
{
	int16_t temperature;
	int16_t humidity;
	int16_t pressure;
	int16_t battery;
};

// Modbus stuff
union au16data_u
{
	sensor_data_s sensor_data; // Temperature * 100, Humidity * 100, Barometric Pressure * 10, Battery * 100
	int16_t data[4];
};

extern au16data_u au16data;

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

// Forward declarations
void send_packet(void);
bool init_status_at(void);
bool init_interval_at(void);
bool get_at_setting(void);
bool save_at_setting(void);

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1	// Base Board
#define LPP_CHANNEL_HUMID 2 // RAK1901
#define LPP_CHANNEL_TEMP 3	// RAK1901
#define LPP_CHANNEL_PRESS 4 // RAK1902

extern WisCayenne g_solution_data;
