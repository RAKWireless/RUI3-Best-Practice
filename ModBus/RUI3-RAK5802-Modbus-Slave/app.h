/**
 * @file main.h
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
#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0);                         \
	delay(100)
#else // RAK4630 || RAK11720
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\r\n");           \
		Serial6.printf(__VA_ARGS__);     \
		Serial6.printf("\r\n");          \
	} while (0);                         \
	delay(100)
#endif
#else
#define MYLOG(...)
#endif

// Sensor functions
bool init_rak1901(void);
void read_rak1901(void);
bool init_rak1902(void);
void read_rak1902(void);

struct sensor_data_s
{
	int16_t coils;
	int16_t temperature;
	int16_t humidity;
	int16_t pressure;
	int16_t battery;
};

union coils_n_regs_u
{
	sensor_data_s sensor_data; // Temperature * 100, Humidity * 100, Barometric Pressure * 10, Battery * 100, 16 coils
	int16_t data[5];
};

extern coils_n_regs_u coils_n_regs;
