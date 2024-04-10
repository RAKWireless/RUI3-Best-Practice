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
		Serial.printf("\r\n");        \
	} while (0);                    \
	delay(100)
#else // RAK4630 || RAK11720
#define AT_PRINTF(...)               \
	do                               \
	{                                \
		Serial.printf(__VA_ARGS__);  \
		Serial.printf("\r\n");         \
		Serial6.printf(__VA_ARGS__); \
		Serial6.printf("\r\n");        \
	} while (0);                     \
	delay(100)
#endif

/** Custom flash parameters */
custom_param_s g_custom_parameters;


// Forward declarations
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param);
int status_handler(SERIAL_PORT port, char *cmd, stParam *param);
int tank_depth_handler(SERIAL_PORT port, char *cmd, stParam *param);
int node_id_handler(SERIAL_PORT port, char *cmd, stParam *param);

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_interval_at(void)
{
	return api.system.atMode.add((char *)"SENDINT",
								 (char *)"Set/Get the interval sending time values in seconds 0 = off, max 2,147,483 seconds",
								 (char *)"SENDINT", interval_send_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for send interval AT commands
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int interval_send_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%ld", cmd, g_custom_parameters.send_interval / 1000);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_send_freq = strtoul(param->argv[0], NULL, 10);

		MYLOG("AT_CMD", "Requested interval %ld", new_send_freq);

		g_custom_parameters.send_interval = new_send_freq * 1000;

		MYLOG("AT_CMD", "New interval %ld", g_custom_parameters.send_interval);
		// Stop the timer
		api.system.timer.stop(RAK_TIMER_0);
		if (g_custom_parameters.send_interval != 0)
		{
			// Restart the timer
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}
		// Save custom settings
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add send interval AT command
 *
 * @return true if success
 * @return false if failed
 */
bool init_node_id_at(void)
{
	return api.system.atMode.add((char *)"NODE_ID",
								 (char *)"Set/Get the node ID",
								 (char *)"NODE_ID", node_id_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

/**
 * @brief Handler for node ID AT commands
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int node_id_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%08X", cmd, g_custom_parameters.node_id);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_node_id = strtoul(param->argv[0], NULL, 10);

		MYLOG("AT_CMD", "Requested node ID %08X", new_node_id);

		g_custom_parameters.node_id = new_node_id;

		MYLOG("AT_CMD", "New node ID %08X", g_custom_parameters.node_id);
		// Save custom settings
		save_at_setting();
	}
	else
	{
		return AT_PARAM_ERROR;
	}

	return AT_OK;
}

/**
 * @brief Add custom Status AT commands
 *
 * @return true AT commands were added
 * @return false AT commands couldn't be added
 */
bool init_status_at(void)
{
	return api.system.atMode.add((char *)"STATUS",
								 (char *)"Get device information",
								 (char *)"STATUS", status_handler,
								 RAK_ATCMD_PERM_READ);
}

/** Regions as text array */
char *regions_list[] = {"EU433", "CN470", "RU864", "IN865", "EU868", "US915", "AU915", "KR920", "AS923", "AS923-2", "AS923-3", "AS923-4"};
/** Network modes as text array*/
char *nwm_list[] = {"P2P", "LoRaWAN", "FSK"};

/**
 * @brief Print device status over Serial
 *
 * @param port Serial port used
 * @param cmd char array with the received AT command
 * @param param char array with the received AT command parameters
 * @return int result of command parsing
 * 			AT_OK AT command & parameters valid
 * 			AT_PARAM_ERROR command or parameters invalid
 */
int status_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	String value_str = "";
	int nw_mode = 0;
	int region_set = 0;
	uint8_t key_eui[16] = {0}; // efadff29c77b4829acf71e1a6e76f713

	if ((param->argc == 1 && !strcmp(param->argv[0], "?")) || (param->argc == 0))
	{
		AT_PRINTF("Device Status:");
		value_str = api.system.hwModel.get();
		value_str.toUpperCase();
		AT_PRINTF("Module: %s", value_str.c_str());
		AT_PRINTF("Version: %s", api.system.firmwareVer.get().c_str());
		AT_PRINTF("Send time: %d s", g_custom_parameters.send_interval / 1000);
		/// \todo
		nw_mode = api.lorawan.nwm.get();
		AT_PRINTF("Network mode %s", nwm_list[nw_mode]);
		if (nw_mode == 1)
		{
			AT_PRINTF("Network %s", api.lorawan.njs.get() ? "joined" : "not joined");
			region_set = api.lorawan.band.get();
			AT_PRINTF("Region: %d", region_set);
			AT_PRINTF("Region: %s", regions_list[region_set]);
			if (api.lorawan.njm.get())
			{
				AT_PRINTF("OTAA mode");
				api.lorawan.deui.get(key_eui, 8);
				AT_PRINTF("DevEUI=%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appeui.get(key_eui, 8);
				AT_PRINTF("AppEUI=%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7]);
				api.lorawan.appkey.get(key_eui, 16);
				AT_PRINTF("AppKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
			}
			else
			{
				AT_PRINTF("ABP mode");
				api.lorawan.appskey.get(key_eui, 16);
				AT_PRINTF("AppsKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.nwkskey.get(key_eui, 16);
				AT_PRINTF("NwsKey=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3],
						  key_eui[4], key_eui[5], key_eui[6], key_eui[7],
						  key_eui[8], key_eui[9], key_eui[10], key_eui[11],
						  key_eui[12], key_eui[13], key_eui[14], key_eui[15]);
				api.lorawan.daddr.get(key_eui, 4);
				AT_PRINTF("DevAddr=%02X%02X%02X%02X",
						  key_eui[0], key_eui[1], key_eui[2], key_eui[3]);
			}
		}
		else if (nw_mode == 0)
		{
			AT_PRINTF("Frequency = %d", api.lorawan.pfreq.get());
			AT_PRINTF("SF = %d", api.lorawan.psf.get());
			AT_PRINTF("BW = %d", api.lorawan.pbw.get());
			AT_PRINTF("CR = %d", api.lorawan.pcr.get());
			AT_PRINTF("Preamble length = %d", api.lorawan.ppl.get());
			AT_PRINTF("TX power = %d", api.lorawan.ptp.get());
		}
		else
		{
			AT_PRINTF("Frequency = %d", api.lorawan.pfreq.get());
			AT_PRINTF("Bitrate = %d", api.lorawan.pbr.get());
			AT_PRINTF("Deviaton = %d", api.lorawan.pfdev.get());
		}
	}
	else
	{
		return AT_PARAM_ERROR;
	}
	return AT_OK;
}

bool init_tank_depth_at(void)
{
	return api.system.atMode.add((char *)"DEPTH",
								 (char *)"Set/Get the tank depth in mm (50mm to 3000mm)",
								 (char *)"DEPTH", tank_depth_handler,
								 RAK_ATCMD_PERM_WRITE | RAK_ATCMD_PERM_READ);
}

int tank_depth_handler(SERIAL_PORT port, char *cmd, stParam *param)
{
	if (param->argc == 1 && !strcmp(param->argv[0], "?"))
	{
		AT_PRINTF("%s=%d", cmd, g_custom_parameters.tank_depth_mm);
	}
	else if (param->argc == 1)
	{
		MYLOG("AT_CMD", "param->argv[0] >> %s", param->argv[0]);
		for (int i = 0; i < strlen(param->argv[0]); i++)
		{
			if (!isdigit(*(param->argv[0] + i)))
			{
				MYLOG("AT_CMD", "%d is no digit", i);
				return AT_PARAM_ERROR;
			}
		}

		uint32_t new_depth = strtoul(param->argv[0], NULL, 10);
		uint8_t old_depth = g_custom_parameters.tank_depth_mm;

		MYLOG("AT_CMD", "Requested depth %ld", new_depth);

		if ((new_depth < 50) || (new_depth > 3000))
		{
			return AT_PARAM_ERROR;
		}

		if (new_depth != old_depth)
		{
			g_custom_parameters.tank_depth_mm = new_depth;
			MYLOG("AT_CMD", "New tank depth %ld", g_custom_parameters.tank_depth_mm);

			// Save custom settings
			save_at_setting();
		}
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
	MYLOG("AT_CMD", "Got flag: %02X", temp_params.valid_flag);
	MYLOG("AT_CMD", "Got send interval: %08X", temp_params.send_interval);
	if (flash_value[0] != 0xAA)
	{
		MYLOG("AT_CMD", "No valid send interval found, set to default, read 0X%08X", temp_params.send_interval);
		g_custom_parameters.send_interval = 0;
		g_custom_parameters.tank_depth_mm = 1100;
		g_custom_parameters.node_id = 0x00;
		save_at_setting();
		return false;
	}
	MYLOG("AT_CMD", "Read send interval %ds", temp_params.send_interval/1000);
	g_custom_parameters.send_interval = temp_params.send_interval;

	MYLOG("AT_CMD", "Read tank depth %dmm", temp_params.tank_depth_mm);
	g_custom_parameters.tank_depth_mm = temp_params.tank_depth_mm;

	MYLOG("AT_CMD", "Read node ID %08X", temp_params.node_id);
	g_custom_parameters.node_id = temp_params.node_id;

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
	uint8_t *flash_value = (uint8_t *)&g_custom_parameters.valid_flag;

	bool wr_result = false;
	MYLOG("AT_CMD", "Writing flag: %02X", g_custom_parameters.valid_flag);
	MYLOG("AT_CMD", "Writing send interval 0X%08X ", g_custom_parameters.send_interval);
	MYLOG("AT_CMD", "Writing tank depth %d ", g_custom_parameters.tank_depth_mm);
	MYLOG("AT_CMD", "Writing node id %08X ", g_custom_parameters.node_id);
	wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	if (!wr_result)
	{
		// Retry
		wr_result = api.system.flash.set(0, flash_value, sizeof(custom_param_s));
	}
	wr_result = true;
	return wr_result;
}
