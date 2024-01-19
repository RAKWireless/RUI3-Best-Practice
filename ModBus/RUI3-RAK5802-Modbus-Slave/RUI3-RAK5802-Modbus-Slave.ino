/**
 * @file main.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Modbus Slave supplying data from environment sensors
 * @version 0.1
 * @date 2024-01-17
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "app.h"

// data array for modbus network sharing
union au16data_u au16data = {0, 0, 0, 0};

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  port : serial port
 *  u8txenpin : 0 for RS-232 and USB-FTDI
 *               or any pin number > 1 for RS-485
 */
Modbus slave(1, Serial1, 0); // this is slave @1 and RS-232 or USB-FTDI

/** Flag if RAK1901 was found */
bool has_rak1901 = false;

/** Flag if RAK1902 was found */
bool has_rak1902 = false;

/** Flag if sensor reading is active */
volatile bool sensor_active = false;

void setup()
{
	// We simulate a ModBus sensor here and switch off the LoRa complete
	api.lora.nwm.set();
	api.lora.precv(0);

	Serial.begin(115200);
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	Serial1.begin(19200); // baud-rate at 19200
	slave.start();
	while (Serial1.available())
	{
		Serial1.read();
	}
	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);
	api.system.firmwareVersion.set("RUI3-ModBus-Slave-V1.0.0");
	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Ver %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	// Start a timer with 2 minutes interval.
	api.system.timer.start(RAK_TIMER_0, 120000, NULL);

	// Check if sensors are connected and initialize them
	Wire.begin();
	has_rak1901 = init_rak1901();
	if (has_rak1901)
	{
		Serial.println("+EVT:RAK1901");
	}
	has_rak1902 = init_rak1902();
	if (has_rak1902)
	{
		Serial.println("+EVT:RAK1902");
	}

	api.system.lpm.set(2);

	// Do an initial reading
	sensor_handler(NULL);
}

void sensor_handler(void *)
{
	sensor_active = true;
	// Get sensor values
	if (has_rak1901)
	{
		read_rak1901();
	}
	if (has_rak1902)
	{
		read_rak1902();
	}

	float battery_reading = 0.0;
	// Add battery voltage
	for (int i = 0; i < 10; i++)
	{
		battery_reading += api.system.bat.get(); // get battery voltage
	}

	battery_reading = battery_reading / 10;
	au16data.sensor_data.battery = (int16_t)(battery_reading * 100);

	MYLOG("SENS", "Data[0] = %d", au16data.data[0]);
	MYLOG("SENS", "Data[1] = %d", au16data.data[1]);
	MYLOG("SENS", "Data[2] = %d", au16data.data[2]);
	MYLOG("SENS", "Data[3] = %d", au16data.data[3]);
	sensor_active = false;
}

void loop()
{
	if (!sensor_active)
	{
		int8_t result = slave.poll(au16data.data, 4);
		if (result != 0)
		{
			MYLOG("POLL", "Poll result is %d", result);
		}
	}
}