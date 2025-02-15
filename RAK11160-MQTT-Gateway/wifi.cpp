/**
 * @file wifi.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Communication functions for ESP8684
 * @version 0.1
 * @date 2025-02-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app.h"

/** WiFi TX buffer */
char esp_tx_buff[512];
/** WiFi RX buffer */
char esp_rx_buff[512];

/**
 * @brief Initialize WiFi and MQTT connections
 *
 * @return true connected
 * @return false no response from ESP8684 or no WiFi connection or no MQTT Broker connection
 */
bool init_connection(void)
{
	if (!init_wifi())
	{
		MYLOG("SETUP", "Init ESP8684 failed");
		return false;
	}
	else
	{
		// Initialize WiFi connection
		has_wifi_conn = connect_wifi();
		if (!has_wifi_conn)
		{
			MYLOG("SETUP", "WiFi connection failed");
			return false;
		}
		else
		{
			// Initialize MQTT Broker connection
			has_mqtt_conn = connect_mqtt();
			if (!has_mqtt_conn)
			{
				MYLOG("SETUP", "MQTT Broker connection failed");
				return false;
			}
			else
			{
				MYLOG("SETUP", "WiFi and MQTT Broker connected");
			}
		}
	}
	return true;
}

/**
 * @brief Initialize ESP8684 connection
 *
 * @return true ESP8684 connected
 * @return false ESP8684 not found
 */
bool init_wifi(void)
{
	// Initialize Serial to ESP8684
	Serial1.begin(115200);
	// Enable ESP8684
	pinMode(WB_ESP8684, OUTPUT);
	digitalWrite(WB_ESP8684, HIGH);
	// Wait for ESP8684 bootup
	time_t start = millis();
	while ((millis() - start) < 30000)
	{
		Serial1.println("AT");
		Serial1.flush();
		/** Expected response ********************
		AT

		OK
		*****************************************/
		if (wait_ok_response(10000))
		{
			// MYLOG("WIFI", "ESP8684 respond to AT: ==>\n%s\n<==\r\n", esp_rx_buff);
			MYLOG("WIFI", "ESP8684 found");
			return true;
		}
		delay(500);
	}
	return false;
}

/**
 * @brief Connect to WiFi network
 * 
 * @return true WiFi connected
 * @return false WiFi connection failed
 */
bool connect_wifi(void)
{
	// Clear send buffer
	memset(esp_tx_buff, 0, 512);
	snprintf(esp_tx_buff, 511, "AT+CWMODE=1,1\r\n");
	Serial1.printf("%s", esp_tx_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+CWMODE=1,1

	OK
	*****************************************/
	if (!wait_ok_response(10000))
	{
		MYLOG("WIFI", "WiFi station mode failed", esp_rx_buff);
		return false;
	}

	// Clear send buffer
	memset(esp_tx_buff, 0, 512);
	snprintf(esp_tx_buff, 511, "AT+CWJAP=\"%s\",\"%s\"\r\n", custom_parameters.MQTT_WIFI_APN, custom_parameters.MQTT_WIFI_PW);
	// MYLOG("WIFI", "Connect with ==>%s<==", esp_tx_buff);
	Serial1.printf("%s", esp_tx_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+cwjap="<MQTT_WIFI_APN>","<MQTT_WIFI_PW>"
	WIFI DISCONNECT
	WIFI CONNECTED
	WIFI GOT IP

	OK
	*****************************************/
	if (wait_ok_response(10000))
	{
		// MYLOG("WIFI", "ESP8684 connected: ==>%s<==\r\n", esp_rx_buff);
		MYLOG("WIFI", "ESP8684 connected");
		return true;
	}
	MYLOG("WIFI", "ESP8684 not connected: ==>%s<==\r\n", esp_rx_buff);
	return false;
}

/**
 * @brief Connect to MQTT Broker
 * 
 * @return true Connection success
 * @return false Connection failed
 */
bool connect_mqtt(void)
{
	// Clear send buffer
	memset(esp_tx_buff, 0, 512);
	snprintf(esp_tx_buff, 511, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", 
		custom_parameters.MQTT_USER, custom_parameters.MQTT_USERNAME, custom_parameters.MQTT_PASSWORD);
	// MYLOG("WIFI", "MQTT USR setup with ==>%s<==", esp_tx_buff);
	Serial1.printf("%s", esp_tx_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+MQTTUSERCFG=0,1,"<MQTT_USER>","<MQTT_USERNAME>","<MQTT_PASSWORD>",0,0,""

	OK
	*****************************************/
	if (wait_ok_response(10000) == false)
	{
		MYLOG("WIFI", "MQTT USR config failed: ==>\n%s\n<==\r\n", esp_rx_buff);
		return false;
	}
	// MYLOG("WIFI", "MQTT USR config ok: ==>%s<==\r\n", esp_rx_buff);

	// Clear send buffer
	memset(esp_tx_buff, 0, 512);
	snprintf(esp_tx_buff, 511, "AT+MQTTCONN=0,\"%s\",%s,0\r\n", 
		custom_parameters.MQTT_URL, custom_parameters.MQTT_PORT);
	// MYLOG("WIFI", "MQTT Connect with ==>%s<==", esp_tx_buff);
	Serial1.printf("%s", esp_tx_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+MQTTCONN=0,"<MQTT_URL>",<MQTT_PORT>,0
	+MQTTCONNECTED:0,1,"<MQTT_URL>","<MQTT_PORT>","",0

	OK
	*****************************************/
	if (wait_ok_response(10000) == false)
	{
		MYLOG("WIFI", "MQTT connect failed: ==>\n%s\n<==\r\n", esp_rx_buff);
		return false;
	}
	// MYLOG("WIFI", "MQTT connect ok: ==>%s<==\r\n", esp_rx_buff);
	return true;
}

/**
 * @brief Publish small topic to MQTT broker
 * 		Limited size for publish package
 *
 * @param sub_topic sub topic maximum length: 128 bytes.
 * @param message to publish maximum length: 124 bytes
 * @return true topic published
 * @return false topic publishing failed
 */
bool publish_msg(char *sub_topic, char *message)
{
	// Clear send buffer
	memset(esp_tx_buff, 0, 512);
	snprintf(esp_tx_buff, 511, "AT+MQTTPUB=0,\"%s%s\",\'%s\',0,0\r\n", 
		custom_parameters.MQTT_PUB, sub_topic, message);
	// MYLOG("WIFI", "MQTT Publish ==>%s<==", esp_tx_buff);
	Serial1.printf("%s", esp_tx_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+MQTTPUB=0,"<MQTT_PUB>","<data>",0,0

	OK
	*****************************************/
	if (wait_ok_response(10000) == false)
	{
		MYLOG("WIFI", "MQTT PUB failed: ==>%s<==\r\n", esp_rx_buff);
		wifi_sending = false;
		return false;
	}
	// MYLOG("WIFI", "MQTT PUB ok: ==>%s<==\r\n", esp_rx_buff);
	MYLOG("WIFI", "MQTT PUB ok");
	wifi_sending = false;
	return true;
}

/**
 * @brief Publish to MQTT broker
 *
 * @param sub_topic sub topic
 * @param message to publish
 * @return true topic published
 * @return false topic publishing failed
 */

bool publish_raw_msg(char *sub_topic, uint8_t *message, size_t msg_len)
{
	// Clear send buffer
	memset(esp_tx_buff, 0, 512);
	snprintf(esp_tx_buff, 511, "AT+MQTTPUBRAW=0,\"%s%s\",%d,0,0\r\n", 
		custom_parameters.MQTT_PUB, sub_topic, msg_len);
	// MYLOG("WIFI", "MQTT Publish Raw ==>%s<==", esp_tx_buff);
	Serial1.printf("%s", esp_tx_buff);
	Serial1.flush();
	/** Expected response ********************
	OK
	>
	*****************************************/
	if (wait_ok_response(10000,">") == false)
	{
		MYLOG("WIFI", "MQTT PUB RAW failed: ==>%s<==\r\n", esp_rx_buff);
		return false;
	}
	// Start sending data
	for (int idx = 0; idx < msg_len; idx++)
	{
		Serial1.write(message[idx]);
	}
	if (wait_ok_response(60000) == false)
	{
		MYLOG("WIFI", "MQTT PUB RAW failed: ==>\n%s\n<==\r\n", esp_rx_buff);
		wifi_sending = false;
		return false;
	}
	// MYLOG("WIFI", "MQTT PUB RAW ok: ==>%s<==\r\n", esp_rx_buff);
	MYLOG("WIFI", "MQTT PUB RAW ok");
	wifi_sending = false;
	return true;
}

/**
 * @brief Wait for response from ESP8684
 *
 * @param timeout time to wait in milliseconds
 * @param wait_for character array to wait for
 * @return true "wait_for" string received
 * @return false "wait_for" string not received, timeout
 */
bool wait_ok_response(time_t timeout, char *wait_for)
{
	time_t start = millis();
	int buff_idx = 0;
	bool got_ok = false;

	// Clear TX buffer
	memset(esp_rx_buff, 0, 512);

	while ((millis() - start) < timeout)
	{
		if (Serial1.available() != 0)
		{
			char rcvd = Serial1.read();
			// Serial.write(rcvd);
			// Serial.flush();
			esp_rx_buff[buff_idx] = rcvd;
			buff_idx++;
			if (buff_idx == 512)
			{
				// Buffer overflow, return false
				return false;
			}

			if (strstr(esp_rx_buff, wait_for) != NULL)
			{
				return true;
			}
		}
		delay(5);
	}
	return false;
}