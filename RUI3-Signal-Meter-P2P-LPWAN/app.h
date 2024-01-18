/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and defines
 * @version 0.1
 * @date 2023-12-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef _APP_H_
#define _APP_H_
#include <Arduino.h>

#include "RAK1921_oled.h"

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

// Set firmware version
#define SW_VERSION_0 1
#define SW_VERSION_1 0
#define SW_VERSION_2 1

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint8_t test_mode = 0;
};

typedef enum test_mode_num
{
	MODE_LINKCHECK = 0,
	MODE_CFM = 1,
	MODE_P2P = 2
} test_mode_num_t;

/** Custom flash parameters */
extern custom_param_s g_custom_parameters;

// Forward declarations
bool init_status_at(void);
bool init_interval_at(void);
bool init_test_mode_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
void set_cfm(void);
void set_linkcheck(void);
void set_p2p(void);
 extern uint32_t g_send_repeat_time;
extern bool lorawan_mode;
extern bool use_link_check;

#endif // _APP_H_
