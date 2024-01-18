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

/** Initialization results */
bool ret;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** Set the device name, max length is 10 characters */
char g_dev_name[64] = "RUI3 Sensor Node                                              ";

/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;

/**
 * @brief Callback after packet was received
 *
 * @param data Structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, fP %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");
}

/**
 * @brief Callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX %d", status);
	digitalWrite(LED_BLUE, LOW);
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
			MYLOG("J-CB", "Fail! \r\n");
		}
	}
	else
	{
		MYLOG("J-CB", "Joined\r\n");
		digitalWrite(LED_BLUE, LOW);
	}
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// Setup the callbacks for joined and send finished
	api.lorawan.registerRecvCallback(receiveCallback);
	api.lorawan.registerSendCallback(sendCallback);
	api.lorawan.registerJoinCallback(joinCallback);

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	Serial.begin(115200);

#ifdef _VARIANT_RAK4630_
	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial.available())
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
	// For RAK3172 just wait a little bit for the USB to be ready
	delay(5000);
#endif

	digitalWrite(LED_GREEN, LOW);

	// Add custom status AT command
	init_status_at();

	MYLOG("SETUP", "RAKwireless %s Node", g_dev_name);
	MYLOG("SETUP", "Setup the device with AT commands first");

	// Search for modules
	find_modules();

	// Get saved sending frequency from flash
	get_at_setting();

	// Create a unified timer
	api.system.timer.create(RAK_TIMER_0, start_sensors, RAK_TIMER_PERIODIC);
	if (g_custom_parameters.send_interval != 0)
	{
		// Start a unified C timer
		api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
	}

	// Timer to start sensor readings after sensor heatup is finished
	api.system.timer.create(RAK_TIMER_2, sensor_handler, RAK_TIMER_ONESHOT);

	// Register the custom AT command to set the send interval
	init_interval_at();

	// Show found modules
	announce_modules();

	// Power down the sensors
	digitalWrite(WB_IO2, LOW);
}

/**
 * @brief Power up sensors and wait 30 seconds before 
 * reading their values
 * 
 */
void start_sensors(void *)
{
	// Set flag for RAK12047 readings
	sensor_reading_active = true;

	MYLOG("SENS", "Powerup Sensors");

	digitalWrite(WB_IO2, HIGH);
	delay(500);
	Wire.begin();

	start_rak12037();

	api.system.timer.start(RAK_TIMER_2, 30000, NULL);
}

/**
 * @brief sensor_handler is a timer function called every
 * g_custom_parameters.send_interval milliseconds. Default is 120000. Can be
 * changed with ATC+SENDINT
 *
 */
void sensor_handler(void *)
{
	// MYLOG("SENS", "Start");
	digitalWrite(LED_BLUE, HIGH);

	// Check if the node has joined the network
	if (!api.lorawan.njs.get())
	{
		// MYLOG("UPL", "Not joined, skip sending");
		return;
	}

	// Clear payload
	g_solution_data.reset();

	// Read sensor data
	get_sensor_values();

	Wire.end();
	// Power down sensors
	digitalWrite(WB_IO2, LOW);

	// Reset flag for RAK12047 readings
	sensor_reading_active = false;

	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());
	// MYLOG("UPL", "Bat %.4f", api.system.bat.get());
	// MYLOG("UPL", "Send %d", g_solution_data.getSize());

	// Send the packet
	if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 2, g_confirmed_mode))
	{
		MYLOG("UPL", "Enqueued");
	}
	else
	{
		MYLOG("UPL", "Send fail");
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
}