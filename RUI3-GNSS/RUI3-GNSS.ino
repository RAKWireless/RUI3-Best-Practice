/**
 * @file RUI3-Modular.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for low power practice
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

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** Flag for GNSS presence */
bool has_gnss = false;

/** Counter for GNSS readings */
uint16_t check_gnss_counter = 0;
/** Max number of GNSS readings before giving up */
time_t check_gnss_max_try = 0;

/** Flag for GNSS readings active */
bool gnss_active = false;

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
		// MYLOG("JOIN-CB", "LoRaWan OTAA - join fail! \r\n");
	}
	else
	{
		// MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
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
	// 	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	// #if MY_DEBUG > 0
	// 	for (int i = 0; i < data->BufferSize; i++)
	// 	{
	// 		Serial.printf("%02X", data->Buffer[i]);
	// 	}
	// 	Serial.print("\r\n");
	// #endif
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
 * @brief Callback for Time Request result
 *
 * @param status
 */
void timeReqCallback(int32_t status)
{
	if (status == GET_DEVICE_TIME_OK)
	{
		MYLOG("TREQ", "Get device time success");
	}
	else if (status == GET_DEVICE_TIME_FAIL)
	{
		MYLOG("TREQ", "Get device time fail");
	}
}

/**
 * @brief LoRaWAN callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	// MYLOG("TX-CB", "TX status %d", status);
	// digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data pointer to the data with the received data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	// 	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);

	// #if MY_DEBUG > 0
	// 	for (int i = 0; i < data.BufferSize; i++)
	// 	{
	// 		Serial.printf("%02X", data.Buffer[i]);
	// 	}
	// 	Serial.print("\r\n");
	// #endif

	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	// MYLOG("TX-P2P-CB", "P2P TX finished");
	// digitalWrite(LED_BLUE, LOW);
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

		g_linkcheck = api.lorawan.linkcheck.get();

		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
		api.lorawan.registerLinkCheckCallback(linkcheckCallback);
		api.lorawan.registerTimereqCallback(timeReqCallback);
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

	api.system.firmwareVersion.set("RUI3-GNSS-V1.0.0");

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
		// MYLOG("SETUP", "Add custom AT command STATUS fail");
	}

	// Register the custom AT command to set the send interval
	if (!init_interval_at())
	{
		// MYLOG("SETUP", "Add custom AT command Send Interval fail");
	}

	// Get saved sending interval from flash
	get_at_setting();

	// Initialize GNSS
	has_gnss = init_gnss();
	if (!has_gnss)
	{
		MYLOG("SETUP", "No GNSS module found");
	}
	else
	{
		MYLOG("SETUP", "Create timer for GNSS polling");
		// Create a timer.
		api.system.timer.create(RAK_TIMER_1, gnss_handler, RAK_TIMER_PERIODIC);
	}
	digitalWrite(LED_GREEN, LOW);

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

		// Enable Timerequest
		// api.lorawan.timereq.set(1);

		// if (api.lorawan.nwm.get() == 1)
		// {
		// 	if (g_confirmed_mode)
		// 	{
		// 		MYLOG("SETUP", "Confirmed enabled");
		// 	}
		// 	else
		// 	{
		// 		MYLOG("SETUP", "Confirmed disabled");
		// }

		// 	MYLOG("SETUP", "Retry = %d", g_confirmed_retry);

		// 	MYLOG("SETUP", "DR = %d", g_data_rate);

		// 	MYLOG("SETUP", "LINKCHECK = %d", g_linkcheck);
	}

	// Enable low power mode
	api.system.lpm.set(1);

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
// No BLE
#else
	Serial6.begin(115200, RAK_AT_MODE);
	api.ble.advertise.start(30);
#endif

	digitalWrite(WB_IO2, LOW);

	if (api.lorawan.nwm.get() == 1)
	{
		if (api.lorawan.njs.get() != 1)
		{
			api.lorawan.join(1, 1, 10, 50);
		}
	}
}

/**
 * @brief GNSS location aqcuisition
 * Called every 2.5 seconds by timer 1
 * Gives up after 1/2 of send interval
 * or when location was aquired
 *
 */
void gnss_handler(void *)
{
	digitalWrite(LED_GREEN, HIGH);
	if (poll_gnss())
	{
		// Power down the module
		digitalWrite(WB_IO2, LOW);
		gnss_active = false;
		delay(100);
		MYLOG("GNSS", "Got location");
		digitalWrite(LED_BLUE, HIGH);
		api.system.timer.stop(RAK_TIMER_1);
		send_packet();
	}
	else
	{
		if (check_gnss_counter >= check_gnss_max_try)
		{
			// Power down the module
			digitalWrite(WB_IO2, LOW);
			delay(100);
			gnss_active = false;
			MYLOG("GNSS", "Location timeout");
			digitalWrite(LED_BLUE, LOW);
			api.system.timer.stop(RAK_TIMER_1);
			send_packet();
		}
	}
	check_gnss_counter++;
	digitalWrite(LED_GREEN, LOW);
}

/**
 * @brief sensor_handler is a timer function called every
 * custom_parameters.send_interval milliseconds. Default is 120000. Can be
 * changed in main.h
 *
 */
void sensor_handler(void *)
{
	MYLOG("UPLINK", "Start");
	// digitalWrite(LED_BLUE, HIGH);

	if (api.lorawan.nwm.get() == 1)
	{
		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			// MYLOG("UPLINK", "Not joined, skip sending");
			return;
		}
	}

	// Clear payload
	g_solution_data.reset();

	// Create payload
	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

	// If it is a GNSS location tracker, start the timer to aquire the location
	if (has_gnss && !gnss_active)
	{
		digitalWrite(WB_IO2, HIGH);

		// Set flag for GNSS active to avoid retrigger */
		gnss_active = true;
		// Startup GNSS module
		init_gnss();
		check_gnss_counter = 0;
		// Set location acquisition time
		if (custom_parameters.send_interval < 30000)
		{
			MYLOG("UPLINK", "Acquistion time too low!");
		}

		// Max location aquisition time is half of send interval
		check_gnss_max_try = custom_parameters.send_interval / 2 / 2500;
		// Try to get location with timer function
		api.system.timer.start(RAK_TIMER_1, 2500, NULL);
	}
	else if (gnss_active)
	{
		return;
	}
	else
	{
		// No GNSS module, just send the packet with the sensor data
		send_packet();
	}
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
	// MYLOG("UPLINK", "Sending %d bytes", g_solution_data.getSize());

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
			MYLOG("UPLINK", "LW Packet enqueued");
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
		// digitalWrite(LED_BLUE, LOW);

		// Enable/disable CAD with AT commands.
		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
		{
			MYLOG("UPLINK", "P2P Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
