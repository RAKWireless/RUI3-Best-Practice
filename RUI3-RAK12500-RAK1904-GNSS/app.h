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
#include <ArduinoJson.h> // Click here to install http://librarymanager/All#ArduinoJson
#include <CayenneLPP.h>	 // Click here to install http://librarymanager/All#CayenneLPP
#include "wisblock_cayenne.h"

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#if MY_DEBUG > 0
#define MYLOG(tag, ...)                   \
	do                                    \
	{                                     \
		if (tag)                          \
			Serial.printf("[%s] ", tag);  \
		Serial.printf(__VA_ARGS__);       \
		Serial.printf("\n");              \
	} while (0);                          \
	delay(100)
#else
#define MYLOG(...)
#endif
#else
#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
		if (tag)                         \
			Serial6.printf("[%s] ", tag); \
		Serial6.printf(__VA_ARGS__);      \
		Serial6.printf("\r\n");             \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif
#endif

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint32_t min_interval = 0;
};
extern custom_param_s custom_parameters;

// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1 // Base Board
#define LPP_CHANNEL_GPS 10 // RAK1910/RAK12500
#define LPP_CHANNEL_TIME 10 // RAK1910/RAK12500

// Forward declarations
void sensor_handler(void *);
void send_packet(void);
bool init_status_at(void);
bool init_interval_at(void);
bool init_min_interval_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
extern WisCayenne g_solution_data;
extern volatile time_t last_trigger;
extern bool motion_detected;

// GNSS stuff
bool init_gnss(void);
bool poll_gnss(void);
void gnss_handler(void *);
extern uint16_t check_gnss_counter;
extern uint16_t check_gnss_max_try;
extern bool gnss_active;
extern time_t gnss_start;
extern time_t gnss_finished;

// Acceleration stuff
bool rak1904_writeRegister(uint8_t chip_reg, uint8_t dataToWrite);
bool rak1904_readRegister(uint8_t *outputPointer, uint8_t chip_reg);
bool init_rak1904(void);
void int_callback_rak1904(void);
void clear_int_rak1904(void);

// Automatic data rate adaption to the packet size
uint8_t get_min_dr(uint16_t region, uint16_t payload_size);
