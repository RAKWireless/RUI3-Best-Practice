/**
 * @file RUI3-RAK3271-DS18B20.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for DS18B20 sensor
 * @version 0.1
 * @date 2024-10-04
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"
#include "board.h"

// Enable (1) or disable (0) encryption in P2P
#define USE_ENCRYPT 0

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

/** P2P Encryption key */
uint8_t node_encrypt_key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/** Instance for first DS18B20 sensor on WB_IO4 PB2 */
MicroDS18B20<WB_IO4> ds_1;
/** Instance for second DS18B20 sensor on WB_IO5 PA15 */
MicroDS18B20<WB_IO5> ds_2;

bool has_ds_1 = false;
bool has_ds_2 = false;

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
 * @brief Result of time request
 *
 * @param status 0 if ok,1 if failed
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
#if USE_ENCRYPT == 1
		api.lora.enckey.set(node_encrypt_key, 16);
		api.lora.encry.set(1);
#endif
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

	api.system.firmwareVersion.set("RUI3-OneWire-V1.0.0");

	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Version %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Initialize module
	// Wire.begin();

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

	// Initialize temperature sensors
	digitalWrite(WB_IO2, HIGH);
	delay(1000);

	uint8_t address[8];

	has_ds_1 = ds_1.readAddress(address);
	if (has_ds_1)
	{
		MYLOG("SETUP", "Sensor #1 ID: 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X",
			  address[0], address[1], address[2], address[3],
			  address[4], address[5], address[6], address[7]);
	}
	else
		Serial.println("DS #1 Not connected");

	has_ds_2 = ds_2.readAddress(address);
	if (has_ds_2)
	{
		MYLOG("SETUP", "Sensor #2 ID: 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X, 0x%0X",
			  address[0], address[1], address[2], address[3],
			  address[4], address[5], address[6], address[7]);
	}
	else
		Serial.println("DS #2 Not connected");
	digitalWrite(WB_IO2, LOW);

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
		api.lorawan.timereq.set(1);
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

		MYLOG("SETUP", "LINKCHECK = %d", g_linkcheck);
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
 * g_send_repeat_time milliseconds. Default is 120000. Can be
 * changed in main.h
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

	digitalWrite(WB_IO2, HIGH);
	delay(250);

	// Read temperature sensors
	float ds18_temp_1 = 0.0;
	float ds18_temp_2 = 0.0;
	if (has_ds_1 || has_ds_2)
	{
		if (read_ds18b20(ds18_temp_1, ds18_temp_2))
		{
			if (has_ds_1)
			{
				g_solution_data.addTemperature(LPP_CHANNEL_TEMP1, ds18_temp_1);
				MYLOG("UPLINK", "DS18B20 sensor #1 %.2f", ds18_temp_1);
			}
			if (has_ds_2)
			{
				g_solution_data.addTemperature(LPP_CHANNEL_TEMP2, ds18_temp_2);
				MYLOG("UPLINK", "DS18B20 sensor #2 %.2f", ds18_temp_2);
			}
		}
		else
		{
			MYLOG("UPLINK", "No DS18B20 sensor or read failed");
		}
	}

	digitalWrite(WB_IO2, LOW);

	// Add battery voltage
	float bat_voltage = api.system.bat.get();
	bat_voltage = 0.0; // Throw away first reading
	for (int ridx = 0; ridx < 10; ridx++)
	{
		bat_voltage += api.system.bat.get();
	}
	bat_voltage = bat_voltage / 10; // Create average of 10 readings

	g_solution_data.addVoltage(LPP_CHANNEL_BATT, bat_voltage);

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
	// api.system.sleep.all();
	api.system.scheduler.task.destroy();
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
		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
		{
#if USE_ENCRYPT == 1
			MYLOG("UPLINK", "Enrypted P2P Packet enqueued");
#else
			MYLOG("UPLINK", "P2P Packet enqueued");
#endif
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
