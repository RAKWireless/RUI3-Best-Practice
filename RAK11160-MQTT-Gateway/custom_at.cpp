/**
 * @file custom_at.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief
 * @version 0.1
 * @date 2022-05-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
#define AT_PRINTF(...)              \
	do                              \
	{                               \
		Serial.printf(__VA_ARGS__); \
		Serial.printf("\r\n");      \
	} while (0);                    \
	delay(100)
#else // RAK4630 || RAK11720
#define AT_PRINTF(...)               \
	do                               \
	{                                \
		Serial.printf(__VA_ARGS__);  \
		Serial.printf("\r\n");       \
		Serial6.printf(__VA_ARGS__); \
		Serial6.printf("\r\n");      \
	} while (0);                     \
	delay(100)
#endif

/** Custom flash parameters */
custom_param_s custom_parameters;

// Forward declarations
int wifi_setup_handler(SERIAL_PORT port, char *cmd, stParam *param);

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_wifi_at(void)
{
	return api.system.atMode.add((char *)"WIFI",
								 (char *)"Set/Get the WiFi and MQTT settings",
								 (char *)"WIFI", wifi_setup_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for WiFi and MQTT AT commands
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int wifi_setup_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%s:%s:%s:%s:%s:%s:%s:%s", cmd,
				  custom_parameters.MQTT_WIFI_APN, custom_parameters.MQTT_WIFI_PW,
				  custom_parameters.MQTT_USER, custom_parameters.MQTT_USERNAME,
				  custom_parameters.MQTT_PASSWORD, custom_parameters.MQTT_URL,
				  custom_parameters.MQTT_PORT, custom_parameters.MQTT_PUB);
	}
	else if (param->argc == 8)
	{
		snprintf(custom_parameters.MQTT_WIFI_APN, 31, param->argv[0]);
		snprintf(custom_parameters.MQTT_WIFI_PW, 31, param->argv[1]);
		snprintf(custom_parameters.MQTT_USER, 31, param->argv[2]);
		snprintf(custom_parameters.MQTT_USERNAME, 31, param->argv[3]);
		snprintf(custom_parameters.MQTT_PASSWORD, 31, param->argv[4]);
		snprintf(custom_parameters.MQTT_URL, 31, param->argv[5]);
		snprintf(custom_parameters.MQTT_PORT, 31, param->argv[6]);
		snprintf(custom_parameters.MQTT_PUB, 31, param->argv[7]);

		// Save custom settings if needed
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Get setting from flash
 *
 * @return false read from flash failed or invalid settings type
 */
bool get_at_setting(void)
{
	custom_param_s temp_params;
	uint8_t *flash_value = (uint8_t *)&temp_params.valid_flag;
	if (!api.system.flash.get(0, flash_value, sizeof(custom_param_s)))
	{
		MYLOG("AT_CMD", "Failed to read send interval from Flash");
		return false;
	}
	// MYLOG("AT_CMD", "Got flag: %02X", temp_params.valid_flag);
	if (flash_value[0] != 0xAA)
	{
		MYLOG("AT_CMD", "No valid settings found, set to default");
		custom_parameters.valid_flag = 0xAA;
		snprintf(custom_parameters.MQTT_WIFI_APN, 32, "WIFI_APN");
		snprintf(custom_parameters.MQTT_WIFI_PW, 32, "WIFI_PW");
		snprintf(custom_parameters.MQTT_USER, 32, "MQTT_USER");
		snprintf(custom_parameters.MQTT_USERNAME, 32, "MQTT_USERNAME");
		snprintf(custom_parameters.MQTT_PASSWORD, 32, "MQTT_PASSWORD");
		snprintf(custom_parameters.MQTT_URL, 32, "MQTT_URL");
		snprintf(custom_parameters.MQTT_PORT, 32, "1883");
		snprintf(custom_parameters.MQTT_PUB, 32, "test/");
		save_at_setting();
		return false;
	}
	memcpy((uint8_t *)&custom_parameters.valid_flag, (uint8_t *)&temp_params.valid_flag, sizeof(custom_param_s));
	return true;
}

/**
 * @brief Save setting to flash
 *
 * @return true write to flash was successful
 * @return false write to flash failed or invalid settings type
 */
bool save_at_setting(void)
{
	uint8_t *flash_value = (uint8_t *)&custom_parameters.valid_flag;

	bool wr_result = false;

	// MYLOG("AT_CMD", "Writing flag: %02X", custom_parameters.valid_flag);
	// MYLOG("AT_CMD", "Writing send interval 0X%08X ", custom_parameters.send_interval);
	wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	if (!wr_result)
	{
		// Retry
		wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	}
	wr_result = true;
	return wr_result;
}
