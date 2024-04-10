/**
 * @file RUI3-RAK13011-Alarm.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for a window and door alarm system
 * @version 0.1
 * @date 2022-10-29
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

/** Initialization results */
bool ret;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

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

/** Enable automatic DR selection based on payload size */
#if AUTO_DR == 1
bool auto_dr_enabled = true;
#else
bool auto_dr_enabled = false;
#endif

/**
 * @brief Callback after packet was received
 *
 * @param data Structure with the received data
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
 * @brief Callback after TX is finished
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
		// bool result_set = api.lorawan.dr.set(g_data_rate);
		// MYLOG("JOIN-CB", "Set the data rate  %s", result_set ? "Success" : "Fail");
		MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
	}
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// api.ble.advertise.start(30);

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
	else
	{

	}

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	// pinMode(WB_IO2, OUTPUT);
	// digitalWrite(WB_IO2, LOW);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);
	api.system.firmwareVersion.set("RUI3-Door-Alarm-V1.0.0");
	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Ver %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Initialize module
	Wire.begin();
	if (!init_rak13011())
	{
		// MYLOG("SETUP", "Could not initialize RAK13011");
	}
	else
	{
		Serial.println("+EVT:RAK13011 OK");
	}

	// Register the custom AT command to get device status
	if (!init_status_at())
	{
		MYLOG("SETUP", "Add custom AT command STATUS fail");
	}
	digitalWrite(LED_GREEN, LOW);

	// Register the custom AT command to set the send interval
	if (!init_interval_at())
	{
		MYLOG("SETUP", "Add custom AT command Send Interval fail");
	}

	// Get saved sending interval from flash
	get_at_setting();

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (g_custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
	}

	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		sensor_handler(NULL);
	}
#if MY_DEBUG == 0
	MYLOG("SETUP", "Debug is disabled");
#else
	MYLOG("SETUP", "Debug is enabled");
#endif
#if AUTO_DR == 0
	MYLOG("SETUP", "Auto DR is disabled");
#else
	MYLOG("SETUP", "Auto DR is enabled");
#endif

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

	// To be checked if this makes sense
	// api.lorawan.join();

	// Force low power mode
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
 * g_custom_parameters.send_interval milliseconds. Can be
 * changed with ATC+SENDINT command
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

	// Clear payload
	g_solution_data.reset();

	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

	// Add last switch status
	g_solution_data.addPresence(LPP_CHANNEL_SWITCH, switch_status);

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
	if (api.lorawan.nwm.get() == 1)
	{
		uint8_t proposed_dr = get_min_dr(api.lorawan.band.get(), g_solution_data.getSize());
		MYLOG("UPLINK", "Check if datarate allows payload size, proposed is DR %d, current DR is %d", proposed_dr, api.lorawan.dr.get());

		if (proposed_dr == 16)
		{
			MYLOG("UPLINK", "No matching DR found");
		}
		else
		{
			if (proposed_dr < api.lorawan.dr.get())
			{
				if (auto_dr_enabled)
				{
					MYLOG("UPLINK", "Proposed DR is lower than current selected, if enabled, switching to lower DR");
					api.lorawan.dr.set(proposed_dr);
				}
			}

			if (proposed_dr > api.lorawan.dr.get())
			{
				if (auto_dr_enabled)
				{
					MYLOG("UPLINK", "Proposed DR is higher than current selected, if enabled, switching to higher DR");
					api.lorawan.dr.set(proposed_dr);
				}
			}
		}

		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("UPLINK", "Packet enqueued, size %d", g_solution_data.getSize());
			tx_active = true;
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
			tx_active = false;
		}
	}
	else
	{
		MYLOG("UPLINK", "Send packet with size %d over P2P", g_solution_data.getSize() + 8);

		digitalWrite(LED_BLUE, LOW);
		uint8_t packet_buffer[g_solution_data.getSize() + 8];
		if (!api.lorawan.deui.get(packet_buffer, 8))
		{
			MYLOG("UPLINK", "Could not get DevEUI");
		}

		memcpy(&packet_buffer[8], g_solution_data.getBuffer(), g_solution_data.getSize());

		for (int idx = 0; idx < g_solution_data.getSize() + 8; idx++)
		{
			Serial.printf("%02X", packet_buffer[idx]);
		}
		Serial.println("");

		if (api.lorawan.psend(g_solution_data.getSize() + 8, packet_buffer))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
