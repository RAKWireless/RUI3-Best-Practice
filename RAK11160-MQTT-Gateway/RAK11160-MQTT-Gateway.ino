/**
 * @file RAK11160-MQTT-Gateway.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RAK11160 as LoRa P2P to MQTT gateway
 * @version 0.1
 * @date 2025-02-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app.h"

/** Flag if WiFi connection is established */
bool has_wifi_conn = false;
/** Flag if MQTT Broker connection is established */
bool has_mqtt_conn = false;

/** WiFi active flag */
bool wifi_sending = false;

/**
 * @brief Arduino setup function, called once
 *
 */
void setup(void)
{
	// Check if device is in LoRa P2P mode
	if (api.lorawan.nwm.get() == 1)
	{
		// Device is in LoRaWAN mode, force LoRa P2P
		api.lorawan.nwm.set();
		// Device will restart
	}

	// Setup LoRa P2P callbacks
	api.lora.registerPRecvCallback(recv_cb);
	api.lora.registerPSendCallback(send_cb);

	// Setup GPIO's
	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);

	api.system.firmwareVersion.set("RUI3-MQTT-WIFI-GW-V1.0.0");

	Serial.println("RUI3 P2P to MQTT WiFi Gateway");
	Serial.println("---------------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.println("---------------------------------------------------------------");

	// Get custom settings for WiFi and MQTT
	if (!init_wifi_at())
	{
		MYLOG("SETUP", "Failed to init ATC");
	}

	// Get WiFi and MQTT settings
	if (!get_at_setting())
	{
		MYLOG("SETUP", "Failed to read settings");
	}

	// Initialize ESP8684 connection
	if (!init_connection())
	{
		MYLOG("SETUP", "Connection failed");
	}
	else
	{
		digitalWrite(LED_GREEN, LOW);
	}
	digitalWrite(LED_BLUE, LOW);

	// Initialize timer for sending received LoRa P2P packet to MQTT broker
	api.system.timer.create(RAK_TIMER_0, send_handler, RAK_TIMER_ONESHOT);

	api.lora.precv(65534);
}

/**
 * @brief Arduino loop. Not used
 *
 */
void loop(void)
{
	api.system.sleep.all();
}

/**
 * @brief Send handler
 * 		Called after LoRa P2P data was received
 *
 */
void send_handler(void *)
{
	if (has_wifi_conn && has_mqtt_conn)
	{
		digitalWrite(LED_BLUE, HIGH);
		MYLOG("SEND", "Publish packet to MQTT Broker");
		// Send as JSON
		size_t buff_len = parse(rcvd_buffer, rcvd_buffer_size);
		if (buff_len != 0)
		{
			if (!publish_raw_msg((char *)"Test", (uint8_t *)json_buffer, buff_len))
			{
				MYLOG("SEND", "Publish failed");
			}
			else
			{
				MYLOG("SEND", "Publish success");
			}
		}
		else
		{
			// MYLOG("SEND", "Parse to JSON failed");
		}
		digitalWrite(LED_BLUE, LOW);
	}
	else
	{
		// No connection to WiFi or Broker, retry to connect
		if (!init_connection())
		{
			MYLOG("SEND", "Reconnect failed");
		}
	}
}