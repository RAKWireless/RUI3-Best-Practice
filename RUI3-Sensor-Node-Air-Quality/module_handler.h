/**
 * @file module_handler.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Globals and defines for module handling
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef MODULE_HANDLER_H
#define MODULE_HANDLER_H

// Module handler stuff
void find_modules(void);
void announce_modules(void);
void get_sensor_values(void);

// Forward declarations
void sensor_handler(void *);
void start_sensors(void *);

extern bool has_rak1906;
extern bool has_rak12037;
extern bool has_rak12047;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1 // Base Board
// #define LPP_CHANNEL_HUMID 2			   // RAK1901
// #define LPP_CHANNEL_TEMP 3			   // RAK1901
// #define LPP_CHANNEL_PRESS 4			   // RAK1902
// #define LPP_CHANNEL_LIGHT 5			   // RAK1903
#define LPP_CHANNEL_HUMID_2 6 // RAK1906
#define LPP_CHANNEL_TEMP_2 7  // RAK1906
#define LPP_CHANNEL_PRESS_2 8 // RAK1906
#define LPP_CHANNEL_GAS_2 9	  // RAK1906
// #define LPP_CHANNEL_GPS 10			   // RAK1910/RAK12500
// #define LPP_CHANNEL_SOIL_TEMP 11	   // RAK12035
// #define LPP_CHANNEL_SOIL_HUMID 12	   // RAK12035
// #define LPP_CHANNEL_SOIL_HUMID_RAW 13  // RAK12035
// #define LPP_CHANNEL_SOIL_VALID 14	   // RAK12035
// #define LPP_CHANNEL_LIGHT2 15		   // RAK12010
#define LPP_CHANNEL_VOC 16 // RAK12047
// #define LPP_CHANNEL_GAS 17			   // RAK12004
// #define LPP_CHANNEL_GAS_PERC 18		   // RAK12004
// #define LPP_CHANNEL_CO2 19			   // RAK12008
// #define LPP_CHANNEL_CO2_PERC 20		   // RAK12008
// #define LPP_CHANNEL_ALC 21			   // RAK12009
// #define LPP_CHANNEL_ALC_PERC 22		   // RAK12009
// #define LPP_CHANNEL_TOF 23			   // RAK12014
// #define LPP_CHANNEL_TOF_VALID 24	   // RAK12014
// #define LPP_CHANNEL_GYRO 25			   // RAK12025
// #define LPP_CHANNEL_GESTURE 26		   // RAK14008
// #define LPP_CHANNEL_UVI 27			   // RAK12019
// #define LPP_CHANNEL_UVS 28			   // RAK12019
// #define LPP_CHANNEL_CURRENT_CURRENT 29 // RAK16000
// #define LPP_CHANNEL_CURRENT_VOLTAGE 30 // RAK16000
// #define LPP_CHANNEL_CURRENT_POWER 31   // RAK16000
// #define LPP_CHANNEL_TOUCH_1 32		   // RAK14002
// #define LPP_CHANNEL_TOUCH_2 33		   // RAK14002
// #define LPP_CHANNEL_TOUCH_3 34		   // RAK14002
#define LPP_CHANNEL_CO2_2 35	   // RAK12037
#define LPP_CHANNEL_CO2_Temp_2 36  // RAK12037
#define LPP_CHANNEL_CO2_HUMID_2 37 // RAK12037
#define LPP_CHANNEL_TEMP_3 38	   // RAK12003
#define LPP_CHANNEL_TEMP_4 39	   // RAK12003

extern WisCayenne g_solution_data;

// Sensor functions
bool init_rak1906(void);
void start_rak1906(void);
bool read_rak1906(void);
void get_rak1906_values(float *values);
bool init_rak12037(void);
void read_rak12037(void);
void start_rak12037(void);
void get_rak12037_values(float *values);
bool init_rak12047(void);
void get_rak12047_value(void);
extern volatile bool sensor_reading_active;
extern float _last_bme_temp;
extern float _last_bme_humid;
extern bool _has_last_bme_values;
extern uint8_t t_h_discard_counter;

// Custom AT commands
bool get_at_setting(void);
bool save_at_setting(void);
bool init_interval_at(void);
bool init_status_at(void);

#endif