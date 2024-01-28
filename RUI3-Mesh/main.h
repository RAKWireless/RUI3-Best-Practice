/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Defines, globals and includes
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// For Watchdog
#include <uhal_wdt.h>

#ifdef SUPPORT_LORA_P2P
#pragma message(Support LoRa P2P on)
#else
#pragma message(Support LoRa P2P is off)
#endif
#ifdef SUPPORT_LORA
#pragma message(Support LoRaWAN is on)
#else
#pragma message(Support LoRaWAN is off)
#endif

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 0
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0)
#else
#define MYLOG(...)
#endif

// Generic data buffer
extern char data_buffer[];

/** Union to convert uint64_t into a byte array */
union longlong_byte_u
{
	uint64_t l_value = 0;
	uint8_t b_values[8];
};

// Mesh Network
#include "mesh.h"
void on_mesh_data(uint32_t fromID, uint8_t *rxPayload, uint16_t rxSize, int16_t rxRssi, int8_t rxSnr);
void map_changed_cb(void);
void timed_loop(void *);
void mesh_task(void *unused);
void print_mesh_map_oled(void);
extern uint8_t rx_temp_buffer[];
extern uint8_t rx_temp_size;
extern int16_t rx_temp_rssi;
extern int8_t rx_temp_snr;

// Mesh events to be handled in the app_event_handler
#define NO_EVENT 0
#define STATUS             0b0000000000000001
#define N_STATUS           0b1111111111111110
#define MESH_MAP_CHANGED   0b1000000000000000
#define N_MESH_MAP_CHANGED 0b0111111111111111
extern volatile uint16_t mesh_event;
extern volatile uint16_t g_task_event_type;

// User AT commands
bool init_interval_at(void);
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param);
bool init_status_at(void);
int status_handler(SERIAL_PORT port, char *cmd, stParam *param);
bool init_map_at(void);
int mesh_map_handler(SERIAL_PORT port, char *cmd, stParam *param);
bool init_master_at(void);
int mesh_master_handler(SERIAL_PORT port, char *cmd, stParam *param);
bool get_at_setting(void);
bool save_at_setting(void);

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint32_t master_address = 0;
};

/** Custom flash parameters */
extern custom_param_s g_custom_parameters;

/////////////////////////////
int at_set_master_node(char *str);
int at_query_master_node(void);
int at_query_node(void);
int at_query_mesh(void);
void read_master_node(void);
void save_master_node(void);
void init_user_at(void);

// OLED
#include <nRF_SSD1306Wire.h>
bool init_rak1921(void);
void rak1921_add_line(char *line);
void rak1921_show(void);
void rak1921_write_header(char *header_line);
void rak1921_clear(void);
void rak1921_write_line(int16_t line, int16_t y_pos, String text);
void rak1921_display(void);
extern char line_str[];
extern bool has_rak1921;

// Use to enable showing the mesh map instead of the received packets (comment out the define)
#define SHOW_MAP

#endif /* MAIN_H */
