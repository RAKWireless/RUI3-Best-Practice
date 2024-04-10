/**
 * @file WisBlock-Sensor-Node.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for easy testing of WisBlock I2C modules
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"

// Flag for ADR on or off
bool adr_enabled = false;

// Data rate 0 .. 15 (validity depnends on Region)
uint8_t data_rate = 3;

// Flag to enable confirmed messages
bool confirmed_msg_enabled = false;

/** Initialization results */
bool ret;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** Flag if RAK1906 was found */
bool has_1906 = false;
/** Flag if RAK1903 was found */
bool has_1903 = false;
/** Flag if RAK12010 was found */
bool has_12010 = false;

/**
 * @brief Callback after packet was received
 *
 * @param data Structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	// for (int i = 0; i < data->BufferSize; i++)
	// {
	// 	Serial.printf("%02X", data->Buffer[i]);
	// }
	// Serial.print("\r\n");
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
	// Display power off
	digitalWrite(POWER_ENABLE, LOW);
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
		if (!(ret = api.lorawan.join()))
		{
			// Serial.println("+EVT:JOIN FAILED");
		}
	}
	else
	{
		// Serial.println("+EVT:JOINED");
		digitalWrite(LED_BLUE, LOW);

		// api.system.timer.start(RAK_TIMER_1, 5000, NULL);
	}
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// if (api.lorawan.nwm.get() == 1)
	{ // Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
	}
	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	// Initialize EPD
	init_rak14000();

	// Use RAK_CUSTOM_MODE supresses AT command and default responses from RUI3
	// Serial.begin(115200, RAK_CUSTOM_MODE);
	// Use "normal" mode to have AT commands available
	Serial.begin(115200);

	time_t serial_timeout = millis();
#ifdef _VARIANT_RAK4630_
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
#else
	// For RAK3172 & RAK11720 just wait a little bit for the USB to be ready
	while (1)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
#endif

	// Initialize modules
	has_1906 = init_rak1906();
	has_1903 = init_rak1903();
	has_12010 = init_rak12010();

	// // Add custom status AT command
	init_status_at();

	MYLOG("SETUP", "RAK RUI3 Sensor Node");
	MYLOG("SETUP", "Setup the device with AT commands first");

	// Get saved sending frequency from flash
	get_at_setting();

	// Create a timer for the frequent data sending
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		MYLOG("SETUP", "Start RAK_TIMER_0");
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}

	// Register the custom AT command to set the send frequency
	init_interval_at();
	// MYLOG("SETUP", "Add custom AT command %s", init_interval_at() ? "Success" : "Fail");

	digitalWrite(LED_GREEN, LOW);

	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);
	}

	api.system.lpm.set(1);

	if (api.lorawan.njs.get())
	{
		sensor_handler(NULL);
	}
}

/**
 * @brief sensor_handler is a timer function called every
 * custom_parameters.send_interval milliseconds. Default is 120000. Can be
 * changed in main.h
 *
 */
void sensor_handler(void *)
{
	// MYLOG("SENS", "Start");
	delay(5000);
	digitalWrite(LED_BLUE, HIGH);

	// if (api.lorawan.nwm.get() == 1)
	{ // Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			// MYLOG("UPLINK", "Not joined, skip sending");
			digitalWrite(LED_BLUE, LOW);
			return;
		}
	}

	// Clear payload
	g_solution_data.reset();

	// Display power on
	digitalWrite(POWER_ENABLE, HIGH);

	// Read sensor data
	if (has_1906)
	{
		read_rak1906();
	}
	if (has_1903)
	{
		read_rak1903();
	}
	if (has_12010)
	{
		read_rak12010();
	}

	// Update EPD
	refresh_rak14000();

	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

	send_packet();

	// Display power off
	digitalWrite(POWER_ENABLE, LOW);
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
	if (api.lorawan.nwm.get() == 1)
	{
		// MYLOG("UPLINK", "Send packet with size %d", g_solution_data.getSize());

		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 2, confirmed_msg_enabled, 1))
		{
			MYLOG("UPLINK", "Send %d", g_solution_data.getSize());
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
			digitalWrite(LED_BLUE, LOW);
			delay(5000);
			// Display power off
			digitalWrite(POWER_ENABLE, LOW);
		}
	}
	else
	{
		// MYLOG("UPLINK", "Send packet with size %d over P2P", g_solution_data.getSize() + 8);

		digitalWrite(LED_BLUE, LOW);

		if (api.lorawan.psend(g_solution_data.getSize(), g_solution_data.getBuffer()))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
