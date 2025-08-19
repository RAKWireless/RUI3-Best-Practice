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
#ifndef _MAIN_H_
#define _MAIN_H_

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

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1			   // Base Board
#define LPP_CHANNEL_HUMID 2			   // RAK1901
#define LPP_CHANNEL_TEMP 3			   // RAK1901
#define LPP_CHANNEL_PRESS 4			   // RAK1902
#define LPP_CHANNEL_LIGHT 5			   // RAK1903
#define LPP_CHANNEL_HUMID_2 6		   // RAK1906
#define LPP_CHANNEL_TEMP_2 7		   // RAK1906
#define LPP_CHANNEL_PRESS_2 8		   // RAK1906
#define LPP_CHANNEL_GAS_2 9			   // RAK1906
#define LPP_CHANNEL_GPS 10			   // RAK1910/RAK12500
#define LPP_CHANNEL_SOIL_TEMP 11	   // RAK12035
#define LPP_CHANNEL_SOIL_HUMID 12	   // RAK12035
#define LPP_CHANNEL_SOIL_HUMID_RAW 13  // RAK12035
#define LPP_CHANNEL_SOIL_VALID 14	   // RAK12035
#define LPP_CHANNEL_LIGHT2 15		   // RAK12010
#define LPP_CHANNEL_VOC 16			   // RAK12047
#define LPP_CHANNEL_GAS 17			   // RAK12004
#define LPP_CHANNEL_GAS_PERC 18		   // RAK12004
#define LPP_CHANNEL_CO2 19			   // RAK12008
#define LPP_CHANNEL_CO2_PERC 20		   // RAK12008
#define LPP_CHANNEL_ALC 21			   // RAK12009
#define LPP_CHANNEL_ALC_PERC 22		   // RAK12009
#define LPP_CHANNEL_TOF 23			   // RAK12014
#define LPP_CHANNEL_TOF_VALID 24	   // RAK12014
#define LPP_CHANNEL_GYRO 25			   // RAK12025
#define LPP_CHANNEL_GESTURE 26		   // RAK14008
#define LPP_CHANNEL_UVI 27			   // RAK12019
#define LPP_CHANNEL_UVS 28			   // RAK12019
#define LPP_CHANNEL_CURRENT_CURRENT 29 // RAK16000
#define LPP_CHANNEL_CURRENT_VOLTAGE 30 // RAK16000
#define LPP_CHANNEL_CURRENT_POWER 31   // RAK16000
#define LPP_CHANNEL_TOUCH_1 32		   // RAK14002
#define LPP_CHANNEL_TOUCH_2 33		   // RAK14002
#define LPP_CHANNEL_TOUCH_3 34		   // RAK14002
#define LPP_CHANNEL_CO2_2 35		   // RAK12037
#define LPP_CHANNEL_CO2_Temp_2 36	   // RAK12037
#define LPP_CHANNEL_CO2_HUMID_2 37	   // RAK12037
#define LPP_CHANNEL_TEMP_3 38		   // RAK12003
#define LPP_CHANNEL_TEMP_4 39		   // RAK12003

/** Module stuff */
#include <rak14000.h> //Click here to get the library: http://librarymanager/All#RAK14000

#define POWER_ENABLE WB_IO2
#define LEFT_BUTTON WB_IO6
#define MIDDLE_BUTTON WB_IO5
#define RIGHT_BUTTON WB_IO3

void init_rak14000(void);
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);
void clear_rak14000(void);
void refresh_rak14000(void);
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);

void handle_OTAA(void);
void handle_ABP(void);
void handle_SEND(void);

extern bool tx_finished;
extern bool tx_started;
extern bool is_joining;
extern uint8_t event;
extern uint16_t bg_color;
extern uint16_t txt_color;
extern WisCayenne g_solution_data;
extern bool has_rx;
extern uint8_t last_rx_buff[];

// Custom AT commands
void send_packet(void);
bool get_at_setting(void);
bool save_at_setting(void);
bool init_interval_at(void);
bool init_status_at(void);

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

#endif // _MAIN_H_
