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

/** Queue with received data packets (max 20, each 128 bytes long) */
ArrayQueue Fifo;

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
	pinMode(LED_WIFI, OUTPUT);
	digitalWrite(LED_WIFI, HIGH);
	pinMode(LED_MQTT, OUTPUT);
	digitalWrite(LED_MQTT, LOW);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	time_t start_wait = millis();
	while ((millis() - start_wait) < 5000)
	{
		digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
		digitalWrite(LED_MQTT, !digitalRead(LED_MQTT));
		delay(100);
	}
	digitalWrite(LED_WIFI, HIGH);
	digitalWrite(LED_MQTT, HIGH);
	// delay(5000);

	api.system.firmwareVersion.set("RUI3-MQTT-WIFI-GW-V1.0.0");

	Serial.println("RUI3 P2P to MQTT WiFi Gateway");
	Serial.println("---------------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.println("---------------------------------------------------------------");
	Serial.flush();
	delay(100);

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
		digitalWrite(LED_WIFI, LOW);
		digitalWrite(LED_MQTT, LOW);
	}

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
	digitalWrite(LED_WIFI, HIGH);
	if (!has_wifi_conn || !has_mqtt_conn)
	{
		// No connection to WiFi or Broker, retry to connect
		if (!init_connection(true))
		{
			MYLOG("SEND", "Reconnect failed");
			digitalWrite(LED_WIFI, HIGH);
		}
	}

	if (has_wifi_conn && has_mqtt_conn)
	{
		digitalWrite(LED_MQTT, HIGH);
		while (!Fifo.isEmpty())
		{
			MYLOG("SEND", "%d FiFo entries ", Fifo.getSize());
			// Get last entry from the Fifo queue
			uint16_t buffer_size = Fifo.getPayloadSize();
			MYLOG("SEND", "Payload size %d", buffer_size);
			uint8_t *buffer = Fifo.getPayload();
#if MY_DEBUG > 0
			for (int i = 0; i < buffer_size; i++)
			{
				Serial.printf("%02X", buffer[i]);
			}
			Serial.print("\r\n");
#endif
			// Parse the data
			MYLOG("SEND", "Publish packet to MQTT Broker");
			// Send as JSON
			size_t buff_len = parse(buffer, buffer_size);

			if (buff_len != 0)
			{
				if (!publish_raw_msg((char *)"Test", (uint8_t *)json_buffer, buff_len))
				{
					MYLOG("SEND", "Publish failed");
					digitalWrite(LED_MQTT, HIGH);
					has_wifi_conn = false;
					has_mqtt_conn = false;
				}
				else
				{
					digitalWrite(LED_MQTT, LOW);
					MYLOG("SEND", "Publish success");
				}
			}
			else
			{
				// MYLOG("SEND", "Parse to JSON failed");
			}

			// Remove entry from queue
			Fifo.deQueue();
		}
		digitalWrite(LED_WIFI, LOW);
	}
	wifi_sending = false;
}