/**
 * @file RUI3-RAK12007-Waterlevel.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Water level measurement with RAK12007 ultrasonic sensor
 * @version 0.1
 * @date 2023-12-30
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

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** Payload buffer */
WisCayenne g_solution_data(255);

/** Packet backup buffer in case TX fails */
uint8_t packet_backup_buffer[256];

/** Size of backup buffer */
uint8_t packet_backup_len = 0;

/** Flag is TX was retry */
bool tx_retry = false;

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
 * @param data pointer to structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");
	tx_active = false;
}

/**
 * @brief Callback for LinkCheck result
 *
 * @param data pointer to structure with the linkcheck result
 */
void linkcheckCallback(SERVICE_LORA_LINKCHECK_T *data)
{
	MYLOG("LC_CB", "%s Margin %d GW# %d RSSI%d SNR %d", data->State == 0 ? "Success" : "Failed",
		  data->DemodMargin, data->NbGateways,
		  data->Rssi, data->Snr);
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
	// data.Status is not supported in RUI4.1.0
	// if (data.Status != 0)
	// {
	// 	MYLOG("RX-P2P-CB", "P2P RX error %d", data.Status);
	// }
	// else
	{
		MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
#if MY_DEBUG == 1
		for (int i = 0; i < data.BufferSize; i++)
		{
			Serial.printf("%02X", data.Buffer[i]);
		}
		Serial.print("\r\n");
#endif
	}
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

	if (result)
	{
		if (tx_retry == true)
		{
			// If CAD reports 2 times channel activity, give up
			MYLOG("CAD-P2P-CB", "P2P CAD failed twice");
			tx_retry = false;
			return;
		}
		// Start timer to resend the packet in 5 seconds
		tx_retry = true;
		api.system.timer.start(RAK_TIMER_1, 2000, NULL);
	}
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
		// api.lorawan.registerLinkCheckCallback(linkcheckCallback);  // Requires RUI3 V4.1.1
	}
	else // Setup for LoRa P2P
	{
		api.lorawan.registerPRecvCallback(recv_cb);
		api.lorawan.registerPSendCallback(send_cb);
		api.lorawan.registerPSendCADCallback(cad_cb);
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

	api.system.firmwareVersion.set("RUI3-Water-Level-V1.0.1");

	Serial.println("RAKwireless RUI3 Water level sensor");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Version %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Initialize module
	Wire.begin();

	// Initialize RAK12007
	has_rak12007 = init_rak12007();

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

	// Register the custom AT command to set the tank depth
	if (!init_tank_depth_at())
	{
		MYLOG("SETUP", "Add custom AT command Tank Depth fail");
	}

	// Register the custom AT command to set the node ID
	if (!init_node_id_at())
	{
		MYLOG("SETUP", "Add custom AT command Node ID fail");
	}

	// Get saved sending user settings from flash
	get_at_setting();

	digitalWrite(LED_GREEN, LOW);

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (g_custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
	}
	api.system.timer.create(RAK_TIMER_1, resend_packet, RAK_TIMER_ONESHOT);

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		sensor_handler(NULL);
	}
	// else
	// {
	// 	// Enable permanent linkcheck
	// 	api.lorawan.linkcheck.set(2);
	// }

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
}

/**
 * @brief sensor_handler is a timer function called every
 * g_custom_parameters.send_interval milliseconds. Default is 120000. Can be
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

	digitalWrite(WB_IO2, HIGH);

	// Reset payload
	g_solution_data.reset();

	// Create payload
	float batt_lvl = 0.0;
	for (int i = 0; i < 10; i++)
	{
		batt_lvl += api.system.bat.get();
	}
	batt_lvl = batt_lvl / 10.0;

	g_solution_data.addVoltage(LPP_CHANNEL_BATT, batt_lvl);

#if MY_DEBUG == 1
	MYLOG("PAYL", "After adding battery");
	uint8_t *payload_temp = g_solution_data.getBuffer();
	for (int i = 0; i < g_solution_data.getSize(); i++)
	{
		Serial.printf("%02X", payload_temp[i]);
	}
	Serial.print("\r\n");
#endif
	// Get distance from US sensor
	if (has_rak12007)
	{
		read_rak12007(true);
	}

	digitalWrite(WB_IO2, LOW);

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
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		MYLOG("UPLINK", "Sending LoRaWAN packet with size %d", g_solution_data.getSize());
		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("UPLINK", "Packet enqueued");
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
		// Add the node ID to the payload
		if (g_solution_data.addDevID(LPP_CHANNEL_DEVID, g_custom_parameters.node_id) == 0)
		{
			MYLOG("UPLINK", "Failed to add device id");
		}
#if MY_DEBUG == 1
		MYLOG("PAYL", "After adding node ID");
		uint8_t *payload_temp = g_solution_data.getBuffer();
		for (int i = 0; i < g_solution_data.getSize(); i++)
		{
			Serial.printf("%02X", payload_temp[i]);
		}
		Serial.print("\r\n");
#endif

		// Create backup of packet
		packet_backup_len = g_solution_data.getSize();
		memset(packet_backup_buffer, 0, 256);
		memcpy(packet_backup_buffer, g_solution_data.getBuffer(), packet_backup_len);

		MYLOG("PARSE", "Sending payload:");
#if MY_DEBUG == 1
		for (int i = 0; i < packet_backup_len; i++)
		{
			Serial.printf("%02X", packet_backup_buffer[i]);
		}
		Serial.print("\r\n");
#endif

		MYLOG("UPLINK", "Send P2P packet with size %d", g_solution_data.getSize());

		digitalWrite(LED_BLUE, LOW);
		tx_retry = false;

		if (api.lorawan.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), false))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}

void resend_packet(void *)
{
	if (api.lorawan.psend(packet_backup_len, packet_backup_buffer, true))
	{
		MYLOG("UPLINK", "Packet enqueued");
	}
	else
	{
		MYLOG("UPLINK", "Send failed");
	}
}