/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Globals and Includes
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <Arduino.h>
#include "wisblock_cayenne.h"

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
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

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint8_t test_mode = 0;
};

/** Custom flash parameters */
extern custom_param_s g_custom_parameters;

// Forward declarations
void send_packet(void);

/** Send Interval offset in flash */
#define SEND_FREQ_OFFSET 0x00000002 // length 4 bytes

// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1			   // Base Board

// Globals
uint8_t get_min_dr(uint16_t region, uint16_t payload_size);
extern volatile bool tx_active;
extern WisCayenne g_solution_data;

// RAK13011 Functions
bool init_rak13011(void);
void handle_rak13011(void *);
#define LPP_CHANNEL_SWITCH 48 // RAK13011
extern volatile int switch_status;

// Custom AT commands
bool init_status_at(void);
bool init_interval_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
