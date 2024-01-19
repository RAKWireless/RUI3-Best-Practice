| <img src="../assets/rakstar.jpg" alt="RAKWireless"> | <img src="../assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="../assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Example for a simple Modbus Master with RAK5802 and RUI3

This example is a simple Modbus Master that reads sensor values from a Modbus Slave. For easy testing a Modbus Slave application is available for another WisBlock/WisDuo based module with a temperature, humidity and barometric pressure sensor.    

This example code is _**NOT**_ using the loop at all. Instead it is complete event driven. The WisDuo/WisBlock module is sleeping unless an event occurs. An event can be a timer callback or an external interrupt, or if using LoRaWAN Class C, it can be a packet received from the LoRaWAN server.

(1) A simple Modbus master that uses a timer to wake up the device in the desired send interval, retrieves sensor values from the Modbus slave and send them over LoRaWAN. Then the system goes back to sleep automatically. The code for the master is in the [RUI3-RAK5802-Modbus-Master](./RUI3-RAK5802-Modbus-Master) folder.    
   
(2) A simple Modbus slave that reads temperature, humidity and barometric pressure from a RAK1901 and RAK1902 module. It offers then the acquired values in 4 registers. This example does not include any coils to set or read binary values. The code for the slave is in the [RUI3-RAK5802-Modbus-Slave](./RUI3-RAK5802-Modbus-Slave) folder. This example is not optimized for low power consumption as the Modbus Slave has to listen all the time for incoming messages over the RS485 port.   

### ⚠️ INFORMATION    
This example uses a modified version of the [Modbus-Master-Slave-for-Arduino](https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino) library. This library was choosen because of its small code size. However, due to some incompatible definitions, it did not compile with RUI3. The library was slightly modified to work with RUI3 and is included as project files _**`RUI3_ModbusRtu.cpp`**_ and _**`RUI3_ModbusRtu.h`**_.    
The original libray is licensed under the [GNU LESSER GENERAL PUBLIC LICENSE Version 2.1](https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino/blob/master/LICENSE.md)

----

# Code sections (only Modbus Master Code)

### ⚠️ INFORMATION    
The code snippets below are simplified. Check the provided source code for the full code.     

## LoRa/LoRaWAN callbacks

There are different system callbacks that are invoked on LoRa/LoRaWAN events.    

For LoRaWAN there are 3 callbacks for join, send and receive events.

LoRaWAN join event is called when a join request was successful or failed. If it fails, it is called after all retries are used up, not on each single retry. The callback has the parameter _**`int32_t status`**_. The status is based on low level LoRaMAC stack results, at the moment the only usuable check is whether the status is 0 ==> join success or <>0 ==> join failed. It is possible to restart the join request from this callback if the attempt failed before.    

```cpp
void joinCallback(int32_t status)
{
	if (status != 0)
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - join fail! \r\n");
	}
	else
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
	}
}
```

LoRaWAN TX finished callback is an important callback. It tells you whether the transmission of a LoRaWAN packet is finished. It is important, because a TX finished event is waking up the device, and when using the loop() function, a sleep call should be issued _**AFTER**_ the TX finished event. The callback has the parameter _**`int32_t status`**_. The status is based on low level LoRaMAC stack results, at the moment the only usuable check is whether the status is 0 ==> send success or <>0 ==> send failed.

```cpp
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX status %d", status);
}
```

LoRaWAN RX callback is called when a data packet was received from the LoRaWAN server. The callback has as parameter a pointer to a structure with information about the received data and pointers to the payload and its length.    

```cpp
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
}
```

LoRa P2P TX finished callback is an important callback. It tells you whether the transmission of a LoRa packet is finished. It is important, because a TX finished event is waking up the device, and when using the loop() function, a sleep call should be issued _**AFTER**_ the TX finished event. The callback has the no parameters, as LoRa P2P does not have any protocol that would allow to know if the sent packet was received by another node..

```cpp
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");
}
```

LoRa P2P RX callback is called when a data packet was received from another LoRa node. The callback has as parameter a structure with information about the received data, the payload and its length.    

```cpp
void recv_cb(rui_lora_p2p_recv_t data)
{
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
}
```

----

## setup() and loop()

**`setup()`** is used as known from Arduino sketches to setup the system. It is called once after reboot or power up.    
There are two important parts in the setup() code.

First it checks whether the system is working in LoRaWAN mode or as a LoRa P2P mode. Depending on this, the different callbacks are setup.

```cpp
	// Setup for LoRaWAN
	if (api.lora.nwm.get() == 1)
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
```

Next, the Modbus Master interface is initialized. The RS485 module RAK5802 is connected through Serial1 to the RUI3 module. The power of the RAK5802 is controlled with the WB_IO2 GPIO. If WB_IO2 is low, the RAK5802 is powered off, if WB_IO2 is high, the RAK5802 is powered up.

```cpp
	// Initialize the Modbus interface on Serial1 (connected to RAK5802 RS485 module)
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	Serial1.begin(19200); // baud-rate at 19200
	master.start();
	master.setTimeOut(2000); // if there is no answer in 2000 ms, roll over
```

Then a periodic timer is initialized to wake up the system in intervals to send a packet to the LoRaWAN server or other LoRa P2P nodes. The interval time is set with the variable **`custom_parameters.send_interval`**.

```cpp
	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}
```

The **`loop()`** function does nothing beside of killing itself, which prevents that it is called frequently from the underlaying RUI3 scheduler.

```cpp
void loop()
{
	api.system.scheduler.task.destroy();
}
```

## sensor_handler & send_packet

This functions are where the action is happening. 

**`sensor_handler`** is called by the timer. First it will send a query to the Modbus Slave device on address 1 to get the latest sensor values. The sensor values are provided in 4 Modbus registers. As Modbus standard does not define float values, the sensor data is received as integer with a multiplier applied.
- Temperature is multiplied by 100
- Humidity is multiplied by 100
- Barometer is multiplied by 10
- Battery is multiplied by 100

If data could be retrieved from the Modbus Slave, the data is added to the LoRaWAN payload.

```cpp
void sensor_handler(void *)
{
	digitalWrite(WB_IO2, HIGH);
	MYLOG("SENS", "Send request over ModBus");

	au16data.data[0] = au16data.data[1] = au16data.data[2] = au16data.data[3] = 0;

	telegram.u8id = 1;				  // slave address
	telegram.u8fct = 3;				  // function code (this one is registers read)
	telegram.u16RegAdd = 0;			  // start address in slave
	telegram.u16CoilsNo = 4;		  // number of elements (coils or registers) to read
	telegram.au16reg = au16data.data; // pointer to a memory array in the Arduino

	master.query(telegram); // send query (only once)

	time_t start_poll = millis();

	bool data_ready = false;
	while ((millis() - start_poll) < 5000)
	{
		master.poll(); // check incoming messages
		if (master.getState() == COM_IDLE)
		{
			if ((au16data.data[0] == 0) && (au16data.data[1] == 0) && (au16data.data[2] == 0) && (au16data.data[3] == 0))
			{
				MYLOG("SENS", "No data received");
				break;
			}
			else
			{
				MYLOG("SENS", "Temperature = %.2f", au16data.sensor_data.temperature / 100.0);
				MYLOG("SENS", "Humidity = %.2f", au16data.sensor_data.humidity / 100.0);
				MYLOG("SENS", "Barometer = %.1f", au16data.sensor_data.pressure / 10.0);
				MYLOG("SENS", "Battery = %.2f", au16data.sensor_data.battery / 100.0);

				data_ready = true;

				// Clear payload
				g_solution_data.reset();

				if (au16data.sensor_data.temperature != 0)
				{
					g_solution_data.addTemperature(LPP_CHANNEL_TEMP, au16data.sensor_data.temperature / 100.0);
				}
				if (au16data.sensor_data.humidity != 0)
				{
					g_solution_data.addRelativeHumidity(LPP_CHANNEL_HUMID, au16data.sensor_data.humidity / 100.0);
				}
				if (au16data.sensor_data.pressure != 0)
				{
					g_solution_data.addBarometricPressure(LPP_CHANNEL_PRESS, au16data.sensor_data.pressure / 10.0);
				}
				if (au16data.sensor_data.battery != 0)
				{
					g_solution_data.addVoltage(LPP_CHANNEL_TEMP, au16data.sensor_data.battery / 100.0);
				}

				float battery_reading = 0.0;
				// Add battery voltage
				for (int i = 0; i < 10; i++)
				{
					battery_reading += api.system.bat.get(); // get battery voltage
				}

				battery_reading = battery_reading / 10;

				g_solution_data.addVoltage(LPP_CHANNEL_BATT, battery_reading);

				break;
			}
		}
	}
```

If data could be retrieved from the Modbus Slave, the data is sent over Lora P2P or LoRaWAN.    

```cpp
	// Send the packet
	send_packet();
}
```

**`send_packet`** is checking whether the node is in LoRaWAN or LoRa P2P mode and sends the created payload depending on the selected mode.

```cpp
void send_packet(void)
{
	// Check if it is LoRaWAN
	if (api.lora.nwm.get() == 1)
	{
		// Send the packet
		if (api.lorawan.send(4, g_solution_data, set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("UPLINK", "Packet enqueued, size 4");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
	// It is P2P
	else
	{
		MYLOG("UPLINK", "Send packet with size 4 over P2P");
		if (api.lora.psend(4, g_solution_data))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
```

----

# Get RUI3 devices

Get a RAKwireless RUI3 WisDuo stamp module, breakout board or evaluation board from our [store](https://store.rakwireless.com/collections/new-menu-modules)

Get a RAKwireless RUI3 WisBlock Core module from our [store](https://store.rakwireless.com/collections/wisblock-core)

### ⚠️ INFORMATION 
Only the following modules are supported by RUI3:    

_**WisDuo**_
- [RAK3172 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Module/Overview/)
- [RAK3272S breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3272S-Breakout-Board/Overview/)
- [RAK3172 evaluation board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Evaluation-Board/Overview/)
- [RAK3172-SiP chip](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-SiP/Overview/)
- [RAK3272-SiP breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3272-SiP-Breakout-Board/Overview/)
- [RAK4630 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK4630-Module/Overview/)
- [RAK11720 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11720-Module/Overview/)    
- [RAK11721 breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11721-Breakout-Board/Overview/)    

_**WisBlock Core**_
- [RAK4631-R](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631-R/Overview/)
- [RAK3372](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK3372/Overview/)
- [RAK11722](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11722/Overview/)

----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.