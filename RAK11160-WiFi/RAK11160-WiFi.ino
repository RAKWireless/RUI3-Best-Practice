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
#include <Arduino.h>

// Redefine LED1 pin (Only needed until RAK11160 is officially supported by RUI3)
#ifdef WB_LED1
#undef WB_LED1
#undef LED_GREEN
#define WB_LED1 PA10
#define LED_GREEN PIN_LED1
#endif

// Define enable pin for ESP8684
#define WB_ESP8684 PA0

/** WiFi TX buffer */
char wtx_buf[512];
/** WiFi RX buffer */
char wrx_buf[512];

/** WiFi AP name */
char MQTT_WIFI_APN[32] = "RAKwireless";
/** WiFi AP password */
char MQTT_WIFI_PW[32] = "RAKwireless";

/**
 * @brief Wait for response from ESP8684
 *
 * @param timeout time to wait in milliseconds
 * @param wait_for character array to wait for
 * @return true "OK" string received
 * @return false "OK" string not received, timeout
 */
bool wait_ok_response(time_t timeout)
{
	time_t start = millis();
	int buff_idx = 0;
	bool got_ok = false;

	// Clear TX buffer
	memset(wtx_buf, 0, 512);

	while ((millis() - start) < timeout)
	{
		if (Serial1.available() != 0)
		{
			char rcvd = Serial1.read();
			wtx_buf[buff_idx] = rcvd;
			buff_idx++;
			if (buff_idx == 512)
			{
				// Buffer overflow, return false
				return false;
			}

			if (strstr(wtx_buf, "OK") != NULL)
			{
				return true;
			}
		}
		delay(5);
	}
	return false;
}

/**
 * @brief Arduino setup function, called once
 *
 */
void setup(void)
{
	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);

	api.system.firmwareVersion.set("RAK11160-WIFI");

	Serial.println("RAK11160 WiFi");
	Serial.println("---------------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.println("---------------------------------------------------------------");

	// Initialize interface to ESP8684
	Serial1.begin(115200);
	// Enable ESP8684
	pinMode(WB_ESP8684, OUTPUT);
	digitalWrite(WB_ESP8684, HIGH);
	// Wait for ESP8684 bootup
	bool found_esp8684 = false;
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
			Serial.println("ESP8684 found");
			found_esp8684 = true;
			break;
		}
		delay(500);
	}

	// Set ESP8684 into station mode and enable auto connect AT+CWMODE=1,1
	// Clear send buffer
	memset(wrx_buf, 0, 512);
	snprintf(wrx_buf, 511, "AT+CWMODE=1,1\r\n");
	Serial1.printf("%s", wrx_buf);
	Serial1.flush();
	/** Expected response ********************
	AT+CWMODE=1,1

	OK
	*****************************************/
	if (wait_ok_response(10000))
	{
		Serial.printf("\r\nESP8684 mode set: ==>%s<==\r\n", wtx_buf);
	}
	else
	{
		Serial.printf("\r\nESP8684 mode failed: ==>%s<==\r\n", wtx_buf);
	}

	// Connect ESP8684 to WiFi AP
	// Clear send buffer
	memset(wrx_buf, 0, 512);
	snprintf(wrx_buf, 511, "AT+CWJAP=\"%s\",\"%s\"\r\n", MQTT_WIFI_APN, MQTT_WIFI_PW);
	Serial1.printf("%s", wrx_buf);
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
		Serial.printf("\r\nESP8684 connected: ==>%s<==\r\n", wtx_buf);
	}
	else
	{
		Serial.printf("\r\nESP8684 not connected: ==>%s<==\r\n", wtx_buf);
	}
}

/**
 * @brief Arduino loop. Not used
 *
 */
void loop(void)
{
	api.system.sleep.all();
}
