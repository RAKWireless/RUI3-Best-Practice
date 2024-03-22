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

#ifndef LPP_CHANNEL_BATT
// Cayenne LPP Channel numbers per sensor value used in WisBlock API examples
#define LPP_CHANNEL_DEVID 0	  // Device ID, only used in LoRa P2P
#define LPP_CHANNEL_BATT 1	  // Base Board
#define LPP_CHANNEL_HUMID 2	  // RAK1901
#define LPP_CHANNEL_TEMP 3	  // RAK1901
#define LPP_CHANNEL_PRESS 4	  // RAK1902
#define LPP_CHANNEL_LIGHT 5	  // RAK1903
#define LPP_CHANNEL_HUMID_2 6 // RAK1906
#define LPP_CHANNEL_TEMP_2 7  // RAK1906
#define LPP_CHANNEL_PRESS_2 8 // RAK1906
#define LPP_CHANNEL_GAS_2 9	  // RAK1906
#define LPP_CHANNEL_GPS 10	  // RAK1910/RAK12500
/// \todo double used channel !!!!!!!!!!!!!!!!!!!!!!
#define LPP_CHANNEL_SOIL_TEMP 11 // RAK12035
#define LPP_CHANNEL_GPS_TOWER 11 // RAK13102

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
#define LPP_CHANNEL_PM_1_0 40		   // RAK12039
#define LPP_CHANNEL_PM_2_5 41		   // RAK12039
#define LPP_CHANNEL_PM_10_0 42		   // RAK12039
#define LPP_CHANNEL_EQ_EVENT 43		   // RAK12027
#define LPP_CHANNEL_EQ_SI 44		   // RAK12027
#define LPP_CHANNEL_EQ_PGA 45		   // RAK12027
#define LPP_CHANNEL_EQ_SHUTOFF 46	   // RAK12027
#define LPP_CHANNEL_EQ_COLLAPSE 47	   // RAK12027
#define LPP_CHANNEL_SWITCH 48		   // RAK13011
#define LPP_SH_WS 49				   // SensorHub RK900-09
#define LPP_SH_WD 50				   // SensorHub RK900-09
#define LPP_AUDIO_L_A 51			   // WisBlock Audio
#define LPP_AUDIO_L_C 52			   // WisBlock Audio
#define LPP_AUDIO_ALARM 53			   // WisBlock Audio
#define LPP_AUDIO_TYPE 54			   // WisBlock Audio
#define LPP_SH_SOIL_MOIST 55		   // SensorHub RK500-02
#define LPP_SH_EC 56				   // SensorHub RK500-02
#define LPP_SH_PH_H 57				   // SensorHub RK500-02
#define LPP_SH_PH_L 58				   // SensorHub RK500-02
#define LPP_SH_PYRANO 59			   // SensorHub RK200-03
#define LPP_SH_HUMID_PREC 60		   // SensorHub RK520-02
#define LPP_CHANNEL_WLEVEL 61		   // RAK12059
#define LPP_CHANNEL_WL_LOW 62		   // RAK12059
#define LPP_CHANNEL_WL_HIGH 63		   // RAK12059
#endif

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint32_t tank_depth_mm = 1100;
	uint32_t node_id = 0;
};

// Forward declarations
void send_packet(void);
void resend_packet(void *);
bool init_status_at(void);
bool init_interval_at(void);
bool init_tank_depth_at(void);
bool init_node_id_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
extern custom_param_s g_custom_parameters;
extern WisCayenne g_solution_data;

// RAK12007
#define TRIG WB_IO6
#define ECHO WB_IO4
#define PD WB_IO5 // power done control （=1 power done，=0 power on）

bool init_rak12007(void);
bool read_rak12007(bool add_payload = true);

extern bool has_rak12007;
