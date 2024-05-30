/**
 * @file RUI3-Modular.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for RAK12022 PT100 sensor
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;
/** Enable/disable CAD (Set with AT commands) */
bool g_use_cad = false;
/** Link check request  (Set with AT commands) */
uint8_t g_linkcheck = 0;

/** Time interval to send packets in milliseconds */
uint32_t g_send_repeat_time = 60000;

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** SPI NSS/CS */
const int Max_CS = WB_SPI_CS;
/** MAX31865 instance */
MAX31865 maxTemp;
/** Flag if RAK12022 was found */
bool has_rak12022 = false;

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
void joinCallback(int32_t status)
{
	// MYLOG("JOIN-CB", "Join result %d", status);
	if (status != 0)
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - join fail! \r\n");
	}
	else
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
	}
}

/**
 * @brief LoRaWAN callback after packet was received
 *
 * @param data pointer to structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
#if MY_DEBUG > 0
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");
#endif
	tx_active = false;
}

/**
 * @brief LoRaWAN callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX status %d", status);
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data pointer to the data with the received data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
#if USE_ENCRYPT == 1
	MYLOG("RX-P2P-CB", "Encrypted P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
#else
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
#endif

#if MY_DEBUG > 0
	for (int i = 0; i < data.BufferSize; i++)
	{
		Serial.printf("%02X", data.Buffer[i]);
	}
	Serial.print("\r\n");
#endif

	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief LoRa P2P callback for CAD result
 *
 * @param result true if activity was detected, false if no activity was detected
 */
void cad_cb(bool result)
{
	MYLOG("CAD-P2P-CB", "P2P CAD reports %s", result ? "activity" : "no activity");
	Serial.printf("P2P CAD reports %s\r\n", result ? "activity" : "no activity");
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// Setup for LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		g_confirmed_mode = api.lorawan.cfm.get();

		g_confirmed_retry = api.lorawan.rety.get();

		g_data_rate = api.lorawan.dr.get();

		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
	}
	else // Setup for LoRa P2P
	{
		g_use_cad = api.lora.cad.get() == 1 ? true : false;
		api.lora.registerPRecvCallback(recv_cb);
		api.lora.registerPSendCallback(send_cb);
		api.lora.registerPSendCADCallback(cad_cb);
	}

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);

	api.system.firmwareVersion.set("RUI3-PT100-V1.0.0");

	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Version %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Initialize module
	Wire.begin();

	// Register the custom AT command to get device status
	if (!init_status_at())
	{
		MYLOG("SETUP", "Add custom AT command STATUS fail");
	}

	// Register the custom AT command to set the send interval
	if (!init_interval_at())
	{
		MYLOG("SETUP", "Add custom AT command Send Interval fail");
	}

	// Get saved sending interval from flash
	get_at_setting();

	digitalWrite(LED_GREEN, LOW);

	// Configure PT100 sensor
	has_rak12022 = maxTemp.begin(Max_CS, MAX31865_3WIRE, MAX31865_PT100); // depending on sensor type, choose MAX31865_2WIRE, MAX31865_3WIRE or MAX31865_4WIRE)
	if (has_rak12022)
	{
		MYLOG("APP", "Found MAX31865");
		maxTemp.MAX31865_SetLowFaultThreshold(29.0);  // Set the low fault threshold to 29 degrees C
		maxTemp.MAX31865_SetHighFaultThreshold(34.0); // Set the high fault threshold to 34 degrees C
	}
	else
	{
		MYLOG("APP", "MAX31865 is not connected, Please check your connections\r\n");
	}

	// Assign data ready input
	pinMode(WB_IO6, INPUT);

	api.lorawan.join(1, 1, 30, 10);
	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		api.lora.precv(65533);

		sensor_handler(NULL);
	}
	else
	{
		// Enable permanent linkcheck
		// api.lorawan.linkcheck.set(2);
	}

	if (api.lorawan.nwm.get() == 1)
	{
		if (g_confirmed_mode)
		{
			MYLOG("SETUP", "Confirmed enabled");
		}
		else
		{
			MYLOG("SETUP", "Confirmed disabled");
		}

		MYLOG("SETUP", "Retry = %d", g_confirmed_retry);

		MYLOG("SETUP", "DR = %d", g_data_rate);
	}

	// Enable low power mode
	api.system.lpm.set(1);

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
// No BLE
#else
	Serial6.begin(115200, RAK_AT_MODE);
	api.ble.advertise.start(30);
#endif

	// Power down RAK12022
	digitalWrite(WB_IO2, LOW);
}

/**
 * @brief sensor_handler is a timer function called every
 * g_send_repeat_time milliseconds. Default is 120000. Can be
 * changed in main.h
 *
 */
void sensor_handler(void *)
{
	MYLOG("UPLINK", "Start");
	digitalWrite(LED_BLUE, HIGH);
	digitalWrite(WB_IO2, HIGH);

	if (api.lorawan.nwm.get() == 1)
	{
		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			MYLOG("UPLINK", "Not joined, skip sending");
			return;
		}
	}

	// Clear payload
	g_solution_data.reset();

	// Create payload
	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

	if (has_rak12022)
	{
		// Read PT100 sensor
		delay(500); // give it some time after power up

		// Re-init sensor after power down
		maxTemp.begin(Max_CS, MAX31865_3WIRE, MAX31865_PT100);
		maxTemp.MAX31865_SetLowFaultThreshold(25.0);  // Set the low fault threshold to 29 degrees C
		maxTemp.MAX31865_SetHighFaultThreshold(34.0); // Set the high fault threshold to 34 degrees C

		// Wait for data ready
		delay(100);
		MYLOG("PT100", "DRDY = %d", digitalRead(WB_IO6));
		time_t start_wait = millis();
		bool wait_timeout = false;
		while (digitalRead(WB_IO6) == HIGH)
		{
			delay(1000);

			if ((millis() - start_wait) > 10000)
			{
				wait_timeout = true;
				break;
			}
		}
		if (wait_timeout)
		{
			MYLOG("PT100", "DRDY timeout");
		}
		else
		{
			MYLOG("PT100", "DRDY = 0 after %ld ms", millis() - start_wait);

			// Get temperature
			float mTemp, mResistance;
			uint8_t mStatus = 0;

			maxTemp.MAX31865_GetTemperatureAndStatus(mTemp, mResistance, mStatus);

			MYLOG("PT100", "PT100 temperature = %.2f", mTemp);
			if (mStatus & MAX31865_FAULT_TEMP_HIGH)
			{
				MYLOG("PT100", "RTD High Threshold");
			}
			if (mStatus & MAX31865_FAULT_TEMP_LOW)
			{
				MYLOG("PT100", "RTD Low Threshold");
			}
			if (mStatus & MAX31865_FAULT_REFIN_HIGH)
			{
				MYLOG("PT100", "REFIN- > 0.85 x Bias");
			}
			if (mStatus & MAX31865_FAULT_REFIN_LOW_OPEN)
			{
				MYLOG("PT100", "REFIN- < 0.85 x Bias - FORCE- open");
			}
			if (mStatus & MAX31865_FAULT_RTDIN_LOW_OPEN)
			{
				MYLOG("PT100", "RTDIN- < 0.85 x Bias - FORCE- open");
			}
			if (mStatus & MAX31865_FAULT_VOLTAGE_OOR)
			{
				MYLOG("PT100", "Voltage out of range fault");
			}

			// Add to payload
			g_solution_data.addTemperature(LPP_CHANNEL_PT100, mTemp);
		}
	}
	else
	{
		MYLOG("PT100", "No PT100 sensor available");
	}

	// Send the packet
	send_packet();

	digitalWrite(WB_IO2, LOW);
}

/**
 * @brief This example is complete timer
 * driven. The loop() does nothing than
 * sleep.
 *
 */
void loop()
{
	api.system.sleep.all();
	// api.system.scheduler.task.destroy();
}

/**
 * @brief Send the data packet that was prepared in
 * Cayenne LPP format by the different sensor and location
 * aqcuision functions
 *
 */
void send_packet(void)
{
	MYLOG("UPLINK", "Sending %d bytes", g_solution_data.getSize());

#if MY_DEBUG > 0
	uint8_t *packet_buffer = g_solution_data.getBuffer();

	for (int i = 0; i < g_solution_data.getSize(); i++)
	{
		Serial.printf("%02X", packet_buffer[i]);
	}
	Serial.print("\r\n");
#endif

	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("UPLINK", "LPW Packet enqueued");
			tx_active = true;
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
			tx_active = false;
		}
	}
	// It is P2P
	else
	{
		digitalWrite(LED_BLUE, LOW);

		// Enable/disable CAD with AT commands.
		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer()))
		{
			MYLOG("UPLINK", "P2P Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
