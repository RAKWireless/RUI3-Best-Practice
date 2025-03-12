/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Includes and defines
 * @version 0.1
 * @date 2025-02-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <Arduino.h>
#include "ArrayQueue.h"

// Redefine LED1 pin (Only needed until RAK11160 is officially supported by RUI3)
#ifdef WB_LED1
#undef WB_LED1
#undef LED_BLUE
#define WB_LED1 PA10
#define LED_BLUE PIN_LED1
#endif

#define LED_MQTT PA1
#define LED_WIFI PA10

// Define enable pin for ESP8684
#define WB_ESP8684 PA0

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
		Serial.flush();                  \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif

// Forward declarations
void recv_cb(rui_lora_p2p_recv_t data);
void send_cb(void);
bool init_connection(bool restart = false);
bool init_wifi(bool restart);
bool connect_wifi(void);
bool connect_mqtt(bool restart = false);
bool publish_msg(char *sub_topic, char *message);
bool publish_raw_msg(char *sub_topic, uint8_t *message, size_t msg_len);
bool wait_ok_response(time_t timeout, uint8_t pin, char *wait_for = "OK");
void send_handler(void *);
size_t parse(uint8_t *data, uint16_t data_len);
extern uint8_t rcvd_buffer[];
extern uint16_t rcvd_buffer_size;
extern bool has_wifi_conn;
extern bool has_mqtt_conn;
extern volatile bool wifi_sending;
extern char json_buffer[];
extern ArrayQueue Fifo;

// Custom AT commands
bool init_wifi_at(void);
int wifi_setup_handler(SERIAL_PORT port, char *cmd, stParam *param);
bool get_at_setting(void);
bool save_at_setting(void);
/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	char MQTT_WIFI_APN[32] = "RAKwireless";
	char MQTT_WIFI_PW[32] = "RAKwireless";
	char MQTT_USER[32] = "ESP8684";
	char MQTT_USERNAME[32] = "RAKwireless";
	char MQTT_PASSWORD[32] = "RAKwireless";
	char MQTT_URL[32] = "127.0.0.1";
	char MQTT_PORT[32] = "1883";
	char MQTT_PUB[32] = "RAKwireless/";
};

// Custom flash parameters
extern custom_param_s custom_parameters;
