/**
 * @file RUI3-Modular.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 example to change settings via downlink
 * @version 0.1
 * @date 2025-02-07
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app.h"

/** Time interval to send packets in milliseconds */
uint32_t g_send_repeat_time = 60000;

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** Payload buffer */
uint8_t g_solution_data[64];

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
		// To be checked if this makes sense
		// api.lorawan.join();
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
 * @param data Structure with the received data
 * 			commands are structered like
 * 			0xAA0x55<commandID><commandValue>
 * 			and have to come over fPort 10
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");

	uint32_t rcvd_value = 0;

	// Check downlink for commands
	if ((data->Buffer[0] == 0xAA) && (data->Buffer[1] == 0x55) && (data->Port == 10))
	{
		// Got a command
		switch (data->Buffer[2])
		{
		case 0x01:					   // Change Send Interval
			if (data->BufferSize != 7) // Check command length
			{
				MYLOG("RX_CB", "Send interval wrong size");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Get new value
			rcvd_value = (uint32_t)(data->Buffer[3]) << 24;
			rcvd_value += (uint32_t)(data->Buffer[4]) << 16;
			rcvd_value += (uint32_t)(data->Buffer[5]) << 8;
			rcvd_value += (uint32_t)(data->Buffer[6]) << 0;

			g_send_repeat_time = rcvd_value * 1000;

			MYLOG("RX_CB", "New interval %ld", g_send_repeat_time);
			// Stop the timer
			api.system.timer.stop(RAK_TIMER_0);
			if (g_send_repeat_time != 0)
			{
				// Restart the timer
				api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
			}
			// Save custom settings
			save_at_setting();
			break;
		case 0x02:					   // Change DR
			if (data->BufferSize != 4) // Check command length
			{
				MYLOG("RX_CB", "DR wrong size");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Get new value
			rcvd_value = data->Buffer[3];
			if ((rcvd_value < 0) || (rcvd_value > 7))
			{
				MYLOG("RX_CB", "DR wrong value");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Set new DR value
			api.lorawan.dr.set(rcvd_value);
			break;
		case 0x03:					   // Enable/Disable ADR
			if (data->BufferSize != 4) // Check command length
			{
				MYLOG("RX_CB", "ADR wrong size");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Get new value
			rcvd_value = data->Buffer[3];
			if ((rcvd_value < 0) || (rcvd_value > 1))
			{
				MYLOG("RX_CB", "ADR wrong value");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Set new ADR value
			api.lorawan.adr.set(rcvd_value);
			break;
		case 0x04:					   // Enable/Disable confirmed mode
			if (data->BufferSize != 4) // Check command length
			{
				MYLOG("RX_CB", "CFM wrong size");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Get new value
			rcvd_value = data->Buffer[3];
			if ((rcvd_value < 0) || (rcvd_value > 1))
			{
				MYLOG("RX_CB", "CFM wrong value");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Set new CFM value
			api.lorawan.cfm.set(rcvd_value);
			break;
		case 0x05:					   // Restart device
			if (data->BufferSize != 3) // Check command length
			{
				MYLOG("RX_CB", "Reboot wrong size");
				AT_PRINTF("+EVT:PARAM_ERROR");
			}
			// Restart device (no data value for this command)
			delay(100);
			api.system.reboot();
			break;

			// Add more commands here

		default:
			AT_PRINTF("+EVT:AT_ERROR");
			break;
		}
	}
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
 * @param data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
	for (int i = 0; i < data.BufferSize; i++)
	{
		Serial.printf("%02X", data.Buffer[i]);
	}
	Serial.print("\r\n");
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
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// Setup for LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
	}
	else // Setup for LoRa P2P
	{
		api.lora.registerPRecvCallback(recv_cb);
		api.lora.registerPSendCallback(send_cb);
	}

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, LOW);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);

	// api.system.firmwareVersion.set("RUI_4.0.6_RAK11720");

	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	// Serial.printf("RUI3 %s\n", api.system.firmwareVersion.get().c_str());
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

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (g_send_repeat_time != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
	}

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		sensor_handler(NULL);
	}

	api.system.lpm.set(1);

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
// No BLE
#else
	Serial6.begin(115200, RAK_AT_MODE);
	api.ble.advertise.start(30);
#endif
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

	if (api.lorawan.nwm.get() == 1)
	{ // Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			MYLOG("UPLINK", "Not joined, skip sending");
			return;
		}
	}

	// Create payload
	char value = 0x31;
	for (int idx = 0; idx < 4; idx++)
	{
		g_solution_data[idx] = value;
		value++;
	}
	g_solution_data[0] = 0x01;
	g_solution_data[1] = 0x74;
	g_solution_data[2] = 0x01;
	g_solution_data[3] = 0x8c;

	// Send the packet
	send_packet();
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
}

/**
 * @brief Send the data packet that was prepared in
 * Cayenne LPP format by the different sensor and location
 * aqcuision functions
 *
 */
void send_packet(void)
{
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Send the packet
		if (api.lorawan.send(4, g_solution_data, 2))
		{
			MYLOG("UPLINK", "Packet enqueued, size 4");
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

		if (api.lora.psend(4, g_solution_data))
		{
			MYLOG("UPLINK", "P2P Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "P2P Send failed");
		}
	}
}
