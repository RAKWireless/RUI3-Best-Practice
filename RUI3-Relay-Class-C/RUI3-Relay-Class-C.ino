/**
 * @file RUI3-Relay-Class.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for remote relay control with downlinks or multicast downlinks
 * @version 0.1
 * @date 2025-05-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app.h"

#define RELAY_IO WB_IO4

/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;

/** Time interval to send packets in milliseconds */
uint32_t g_send_repeat_time = 60000;

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** Relay status */
uint8_t relay_status = LOW;

/** LoRaWAN multicast session structure */
RAK_LORA_McSession session;
/** Multicast address */
uint8_t node_mc_address[4] = {0x00, 0xe2, 0x27, 0x13}; // 00e22713
/** Multicast AppSKey */
uint8_t node_mc_AppSKey[16] = {0x93, 0x98, 0xa5, 0xe3, 0xb6, 0xd2, 0x20, 0xd6, 0x86, 0x15, 0xe0, 0xc6, 0x2d, 0xad, 0x33, 0x07}; // 9398a5e3b6d220d68615e0c62dad3307
/** Multicast NwkSKey */
uint8_t node_mc_NwkSKey[16] = {0xe7, 0x04, 0xca, 0x06, 0xca, 0xf0, 0xbe, 0x78, 0x15, 0x52, 0xc0, 0x4b, 0x4d, 0x46, 0xbd, 0x06}; // e704ca06caf0be781552c04b4d46bd06

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

		// Try to remove last session
		if (api.lorawan.rmvmulc(node_mc_address[0] << 24 | node_mc_address[1] << 16 | node_mc_address[2] << 8 | node_mc_address[3]) == true)
		{
			MYLOG("JOIN-CB", "Remove Multicast Success");
		}
		else
		{
			MYLOG("JOIN-CB", "Remove Multicast Fail");
		}
		// LoRaWAN Multicast Setting
		if (api.lorawan.addmulc(session) == true)
		{
			MYLOG("JOIN-CB", "Add Multicast Success");
		}
		else
		{
			MYLOG("JOIN-CB", "Add Multicast Fail");
		}
	}
	else
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
		sensor_handler(NULL);
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

	// Check if it is a command received on fPort 10
	if (data->Port == 10)
	{
		// Check for valid command sequence
		if ((data->Buffer[0] == 0xAA) && (data->Buffer[1] == 0x55))
		{
			// Check for valid relay status request
			if ((data->Buffer[2] >= 0) && (data->Buffer[2] < 2))
			{
				// Save the status and call the handler
				if (data->Buffer[2] == 0)
				{
					relay_status = LOW;
				}
				else
				{
					relay_status = HIGH;
				}

				api.system.timer.start(RAK_TIMER_1, 100, NULL);
			}
			else
			{
				MYLOG("RX_CB", "Wrong command");
			}
		}
		else
		{
			MYLOG("RX_CB", "Wrong format");
		}
	}
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
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
	for (int i = 0; i < data.BufferSize; i++)
	{
		Serial.printf("%02X", data.Buffer[i]);
	}
	Serial.print("\r\n");
	tx_active = false;

	// Check if it is a command with a valid command sequence
	if ((data.Buffer[0] == 0xAA) && (data.Buffer[1] == 0x55))
	{
		// Check for valid relay status request
		if ((data.Buffer[2] >= 0) && (data.Buffer[2] < 2))
		{
			// Save the status and call the handler
			relay_status = data.Buffer[2];
			api.system.timer.start(RAK_TIMER_1, 100, NULL);
		}
		else
		{
			MYLOG("RX-P2P-CB", "Wrong command");
		}
	}
	else
	{
		MYLOG("RX-P2P-CB", "Wrong format");
	}
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
		api.lorawan.registerLinkCheckCallback(linkcheckCallback);

		// This application requires Class C to receive data at any time
		api.lorawan.deviceClass.set(2);

		// LoRaWan Multicast Session

		// Setup new multicast session
		session.McDevclass = 2;
		session.McAddress = node_mc_address[0] << 24 | node_mc_address[1] << 16 | node_mc_address[2] << 8 | node_mc_address[3];
		session.McFrequency = 916600000;
		session.McDatarate = 4;
		session.McPeriodicity = 0;
		session.McGroupID = 2;
		session.entry = 0;

		memcpy(session.McAppSKey, node_mc_AppSKey, 16);
		memcpy(session.McNwkSKey, node_mc_NwkSKey, 16);
	}
	else // Setup for LoRa P2P
	{
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

	api.system.firmwareVersion.set("RUI3-Relay-V1.0.1");

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

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}

	// Create a timer to handle incoming packets
	api.system.timer.create(RAK_TIMER_1, relay_handler, RAK_TIMER_ONESHOT);

	// Initialize relay control port
	pinMode(RELAY_IO, OUTPUT);
	digitalWrite(RELAY_IO, LOW);
	delay(500);
	digitalWrite(RELAY_IO, HIGH);
	delay(500);
	digitalWrite(RELAY_IO, LOW);

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		// Force continous receive mode
		api.lora.precv(65533);
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
}

/**
 * @brief Set or reset the relay, depending on last received packet
 * 
 */
void relay_handler(void *) {
	MYLOG("DOWNLINK", "Set relay to %s", relay_status == LOW ? "Off" : "On");
	digitalWrite(RELAY_IO, relay_status);
	digitalWrite(LED_GREEN, relay_status);
}

/**
 * @brief sensor_handler is a timer function called every
 * g_send_repeat_time milliseconds.
 *
 */
void sensor_handler(void *)
{
	MYLOG("UPLINK", "Start");
	digitalWrite(LED_BLUE, HIGH);

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

	g_solution_data.addPresence(LPP_CHANNEL_SWITCH, relay_status);

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
		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
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
		MYLOG("UPLINK", "Send packet over P2P");

		digitalWrite(LED_BLUE, LOW);

		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
