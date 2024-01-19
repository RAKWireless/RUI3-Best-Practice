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

// Sensor functions
bool init_rak1901(void);
void read_rak1901(void);
bool init_rak1902(void);
void read_rak1902(void);

struct sensor_data_s {
	int16_t temperature;
	int16_t humidity;
	int16_t pressure;
	int16_t battery;
};

union au16data_u
{
	sensor_data_s sensor_data; // Temperature * 100, Humidity * 100, Barometric Pressure * 10, Battery * 100
	int16_t data[4];
};

extern au16data_u au16data;