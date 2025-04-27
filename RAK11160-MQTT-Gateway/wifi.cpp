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

/** WiFi communication buffer */
char esp_com_buff[1024];

// Forward declaration
void flush_RX(void);

/**
 * @brief Initialize WiFi and MQTT connections
 *
 * @return true connected
 * @return false no response from ESP8684 or no WiFi connection or no MQTT Broker connection
 */
bool init_connection(bool restart)
{
	if (!init_wifi(restart))
	{
		MYLOG("WIFI", "Init ESP8684 failed");
		return false;
	}
	else
	{
		MYLOG("WIFI", "Init ESP8684 ok");
		// Initialize WiFi connection
		has_wifi_conn = connect_wifi();
		if (!has_wifi_conn)
		{
			MYLOG("WIFI", "WiFi connection failed");
			return false;
		}
		else
		{
			MYLOG("WIFI", "WiFi connected");
			// Initialize MQTT Broker connection
			has_mqtt_conn = connect_mqtt(restart);
			if (!has_mqtt_conn)
			{
				MYLOG("WIFI", "MQTT Broker connection failed");
				return false;
			}
			else
			{
				MYLOG("WIFI", "MQTT Broker connected");
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
bool init_wifi(bool restart)
{
	// Initialize Serial to ESP8684
	Serial1.begin(115200);
	pinMode(WB_ESP8684, OUTPUT);
	if (restart)
	{
		digitalWrite(WB_ESP8684, LOW);
		delay(1000);
	}
	// Enable ESP8684
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
		if (wait_ok_response(10000, LED_WIFI))
		{
			// MYLOG("WIFI", "ESP8684 respond to AT: ==>\n%s\n<==\r\n", esp_com_buff);
			// MYLOG("WIFI", "ESP8684 found");
			return true;
		}
		delay(500);
	}
	digitalWrite(LED_WIFI, LOW);

	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// if analog input pin 1 is unconnected, random analog
	// noise will cause the call to randomSeed() to generate
	// different seed numbers each time the sketch runs.
	// randomSeed() will then shuffle the random function.
	randomSeed(analogRead(WB_A1));
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
	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Set connection mode to Station
	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+CWMODE=1,1\r\n");
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+CWMODE=1,1

	OK
	*****************************************/
	if (!wait_ok_response(10000, LED_WIFI))
	{
		MYLOG("WIFI", "WiFi station mode failed: %s", esp_com_buff);
		return false;
	}
	digitalWrite(LED_WIFI, LOW);

	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Set AP name and password
	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+CWJAP=\"%s\",\"%s\"\r\n", custom_parameters.MQTT_WIFI_APN, custom_parameters.MQTT_WIFI_PW);
	// MYLOG("WIFI", "Connect with ==>%s<==", esp_com_buff);
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+cwjap="<MQTT_WIFI_APN>","<MQTT_WIFI_PW>"
	WIFI DISCONNECT
	WIFI CONNECTED
	WIFI GOT IP

	OK
	*****************************************/
	if (!wait_ok_response(10000, LED_WIFI))
	{
		MYLOG("WIFI", "ESP8684 not connected: ==>%s<==\r\n", esp_com_buff);
		return false;
	}
	digitalWrite(LED_WIFI, LOW);

	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Set reconnection configuration (1 second interval, try forever)
	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+CWRECONNCFG=1,0\r\n");
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	+CWRECONNCFG:1,5000>

	OK
	*****************************************/
	if (!wait_ok_response(10000, LED_WIFI))
	{
		MYLOG("WIFI", "ESP8684 not connected: ==>%s<==\r\n", esp_com_buff);
		return false;
	}

	// Check protocol
	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+CWSTAPROTO?\r\n");
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	+RFPOWER:1,5000>

	OK
	*****************************************/
	wait_ok_response(10000, LED_WIFI);
	MYLOG("WIFI", "WiFi protocol: ==>%s<==\r\n", esp_com_buff);

	// Set RF power
	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+RFPOWER=84\r\n");
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	+RFPOWER:1,5000>

	OK
	*****************************************/
	if (wait_ok_response(10000, LED_WIFI))
	{
		MYLOG("WIFI", "WiFi station RF power set: ==>%s<==\r\n", esp_com_buff);
		return true;
	}
	digitalWrite(LED_WIFI, LOW);

	MYLOG("WIFI", "ESP8684 not connected: ==>%s<==\r\n", esp_com_buff);
	return false;
}

/**
 * @brief Connect to MQTT Broker
 *
 * @return true Connection success
 * @return false Connection failed
 */
bool connect_mqtt(bool restart)
{
	if (restart)
	{
		// Try to flush the RX buffer in case there is some ESP8684 stuff in it
		flush_RX();

		// Clear send buffer
		memset(esp_com_buff, 0, 1024);
		snprintf(esp_com_buff, 511, "AT+MQTTCLEAN=0\r\n");
		// MYLOG("WIFI", "MQTT Clean with ==>%s<==", esp_com_buff);
		Serial1.printf("%s", esp_com_buff);
		Serial1.flush();
		/** Expected response ********************
		AT+MQTTCLEAN=0

		OK
		*****************************************/
		if (wait_ok_response(10000, LED_MQTT) == false)
		{
			MYLOG("WIFI", "MQTT CLEAN failed: ==>\n%s\n<==\r\n", esp_com_buff);
		}
		digitalWrite(LED_MQTT, LOW);
	}
	// Create random user
	uint16_t id = random(0, 65535);
	char mqtt_user[64];
	sprintf(mqtt_user, "%s%04X", custom_parameters.MQTT_USER, id);

	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
			 mqtt_user, custom_parameters.MQTT_USERNAME, custom_parameters.MQTT_PASSWORD);
	// MYLOG("WIFI", "MQTT USR setup with ==>%s<==", esp_com_buff);
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+MQTTUSERCFG=0,1,"<MQTT_USER>","<MQTT_USERNAME>","<MQTT_PASSWORD>",0,0,""

	OK
	*****************************************/
	if (wait_ok_response(10000, LED_MQTT) == false)
	{
		MYLOG("WIFI", "MQTT USR config failed: ==>\n%s\n<==\r\n", esp_com_buff);
		return false;
	}
	digitalWrite(LED_MQTT, LOW);
	// MYLOG("WIFI", "MQTT USR config ok: ==>%s<==\r\n", esp_com_buff);

	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+MQTTCONN=0,\"%s\",%s,0\r\n",
			 custom_parameters.MQTT_URL, custom_parameters.MQTT_PORT);
	// MYLOG("WIFI", "MQTT Connect with ==>%s<==", esp_com_buff);
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+MQTTCONN=0,"<MQTT_URL>",<MQTT_PORT>,0
	+MQTTCONNECTED:0,1,"<MQTT_URL>","<MQTT_PORT>","",0

	OK
	*****************************************/
	if (wait_ok_response(10000, LED_MQTT) == false)
	{
		MYLOG("WIFI", "MQTT connect failed: ==>\n%s\n<==\r\n", esp_com_buff);
		return false;
	}
	digitalWrite(LED_MQTT, LOW);
	// MYLOG("WIFI", "MQTT connect ok: ==>%s<==\r\n", esp_com_buff);
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
	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+MQTTPUB=0,\"%s%s\",\'%s\',0,0\r\n",
			 custom_parameters.MQTT_PUB, sub_topic, message);
	// MYLOG("WIFI", "MQTT Publish ==>%s<==", esp_com_buff);
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	AT+MQTTPUB=0,"<MQTT_PUB>","<data>",0,0

	OK
	*****************************************/
	if (wait_ok_response(10000, LED_MQTT) == false)
	{
		MYLOG("WIFI", "MQTT PUB failed: ==>%s<==\r\n", esp_com_buff);
		return false;
	}
	digitalWrite(LED_MQTT, LOW);
	// MYLOG("WIFI", "MQTT PUB ok: ==>%s<==\r\n", esp_com_buff);
	MYLOG("WIFI", "MQTT PUB ok");
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
	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	flush_RX();

	// Clear send buffer
	memset(esp_com_buff, 0, 1024);
	snprintf(esp_com_buff, 511, "AT+MQTTPUBRAW=0,\"%s%s\",%d,0,0\r\n",
			 custom_parameters.MQTT_PUB, sub_topic, msg_len);
	// MYLOG("WIFI", "MQTT Publish Raw ==>%s<==", esp_com_buff);
	Serial1.printf("%s", esp_com_buff);
	Serial1.flush();
	/** Expected response ********************
	OK
	>
	*****************************************/
	if (wait_ok_response(10000, LED_MQTT, ">") == false)
	{
		MYLOG("WIFI", "MQTT PUB RAW failed waiting for '>': ==>%s<==\r\n", esp_com_buff);
		return false;
	}
	digitalWrite(LED_MQTT, LOW);
	// Start sending data
	for (int idx = 0; idx < msg_len; idx++)
	{
		Serial1.write(message[idx]);
		delay(5);
	}
	if (wait_ok_response(60000, LED_MQTT) == false)
	{
		MYLOG("WIFI", "MQTT PUB RAW failed waiting for 'OK': ==>\n%s\n<==\r\n", esp_com_buff);
		return false;
	}
	digitalWrite(LED_MQTT, LOW);
	// MYLOG("WIFI", "MQTT PUB RAW ok: ==>%s<==\r\n", esp_com_buff);
	MYLOG("WIFI", "MQTT PUB RAW ok");
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
bool wait_ok_response(time_t timeout, uint8_t pin, char *wait_for)
{
	time_t start = millis();
	int buff_idx = 0;
	bool got_ok = false;

	// Clear TX buffer
	memset(esp_com_buff, 0, 1024);

	while ((millis() - start) < timeout)
	{
		if (Serial1.available() != 0)
		{
			char rcvd = Serial1.read();
			// Serial.write(rcvd);
			// Serial.flush();
			esp_com_buff[buff_idx] = rcvd;
			buff_idx++;
			if (buff_idx == 1024)
			{
				esp_com_buff[1023] = 0;
				digitalWrite(pin, LOW);
				// Buffer overflow, return false
				return false;
			}
			esp_com_buff[buff_idx] = 0;
			if (strstr(esp_com_buff, wait_for) != NULL)
			{
				// Serial.println("RX OK");
				digitalWrite(pin, LOW);
				return true;
			}
		}
		digitalWrite(pin, !digitalRead(pin));
		delay(10);
	}
	digitalWrite(pin, LOW);
	return false;
}

/**
 * @brief Flush RX buffer from left-over ESP8684 data
 * 
 */
void flush_RX(void)
{
	// Try to flush the RX buffer in case there is some ESP8684 stuff in it
	if (Serial1.available())
	{
		while (Serial1.available())
		{
			Serial1.read();
			delay(10);
		}
	}
}