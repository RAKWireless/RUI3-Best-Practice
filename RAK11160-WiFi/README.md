| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# How to connect the RAK11160 WisDuo module to a WiFi AP

The WisDuo RAK11160 WisDuo provides LoRa, WiFi and BLE connectivity in one single module.    
This makes it the perfect choice to build a gateway between the long range LoRa world and the cloud, or specifically here to an MQTT broker.    

In this tutorial we will show how to connect the RAK11160 to WiFi AP.          

## Connect from external MCU with loop-through mode

To send AT commands from the console or an external host MCU, you have to change the UART mode of the STM32WLE5 to loop-through mode. This is done with the AT command AT+ESP=x

To enable loop-through mode:
```AT
AT+ESP=1
```

To disable loop-through mode:
```AT
AT+ESP=0
```

To connect to a WiFi AP, the ESP8684 must be set to station mode. Then the AP name and AP access password must be provided.
The AP name and AP access password are stored in the ESP8684 and will be reused on the next reboot/power up to automatically connect WiFi.    

Connect to a WiFi AP and enable auto-connect:
```AT
AT+ESP=1

AT+CWMODE=1,1
AT+CWJAP="<MQTT_WIFI_APN>","<MQTT_WIFI_PW>"
```
Replace `<MQTT_WIFI_APN>` and `<MQTT_WIFI_PW>` with the AP name and access credential of the WiFi access point.

If the ESP8684 can successfully connect to the WiFi AP, it will response with
```AT
WIFI DISCONNECT
WIFI CONNECTED
WIFI GOT IP

OK
```

## Connect with RUI3 API calls and ESP8684 AT commands

1. Power up the ESP8684

```cpp
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
```

2. Set ESP8684 into WiFi station mode

```cpp
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
```

3. Setup the WiFi connection parameters and connect

```cpp
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
```
