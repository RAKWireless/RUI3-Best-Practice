| <img src="../assets/rakstar.jpg" alt="RAKWireless"> | <img src="../assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="../assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Example For the WisBlock Kit 4 setup using RUI3

This code example utilizes the WisBlock Kit 4 with RUI3. It supports the RAK1906 sensor module coming with the WisBlock Kit 4.

This example code is _**NOT**_ using the loop at all. Instead it is complete event driven. The WisDuo/WisBlock module is sleeping unless an event occurs. An event can be a timer callback or an external interrupt, or if using LoRaWAN Class C, it can be a packet received from the LoRaWAN server.

This code does setup a timer that wakes up the device in the desired send interval, then it reads the sensor values from the WisBlock modules and sends a packet. After that the system goes back to sleep automatically.    

### ⚠️ INFORMATION
The payload is in Cayenne LPP format with extended data types. A matching decoder can be found in the [RAKwireless_Standardized_Payload Github repo](https://github.com/RAKWireless/RAKwireless_Standardized_Payload/blob/main/RAKwireless_Standardized_Payload.js)

### ⚠️ INFORMATION
The code requires RUI3 library for the RAK1906 module. The libraries can be downloaded as ZIP files from
- [RAKwireless_RAK1906_Environment_BME680.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1906_Environment_BME680.zip)

The libraries can be installed with the "Install from ZIP" function in the ArduinoIDE Library Manager.    
The libraries can be installed as well manually by unzipping the content into the default Arduino Library folder.

This examples includes three custom AT commands:     
- **`ATC+SENDINT`** to set the send interval time or heart beat time. If the device is not in motion it will send a payload with this interval. The time is set in seconds, e.g. **`AT+SENDINT=600`** sets the send interval to 600 seconds or 10 minutes.    
- **`ATC+STATUS`** to get some status information from the device.    

----

# Code sections

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
	if (api.lorawan.nwm.get() == 1)
	{
		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
	}
	else // Setup for LoRa P2P
	{
		api.lorawan.registerPRecvCallback(recv_cb);
		api.lorawan.registerPSendCallback(send_cb);
	}
```

Second a periodic timer is initialized to wake up the system in intervals to send a packet to the LoRaWAN server or other LoRa P2P nodes. The interval time is set with the variable **`custom_parameters.send_interval`**.

```cpp
	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}
```

The setup() is as well checking if the sensor is present by initializing it and setting a flag.
```cpp
	// Check if sensors are connected and initialize them
	has_rak1906 = init_rak1906();
	if (has_rak1906)
	{
		Serial.println("+EVT:RAK1906");
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

**`sensor_handler`** is called by the timer. First, if in LoRaWAN mode, it checks whether the node has already joined the network. Then, in this example, it is as a first step powering up the sensors. To give the sensor some time to adjust to the environment, they are kept on for 10 seconds. Then the sensor values are read and put together with the battery value in the payload.    
Again, to lower the power consumption, the sensor are then shut-down.
After the payload is ready, it calls **`send_packet`** to get the packet sent out.

```cpp
void sensor_handler(void *)
{
	MYLOG("UPLINK", "Start");

	if (api.lorawan.nwm.get() == 1)
	{
		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			MYLOG("UPLINK", "Not joined, skip sending");
			return;
		}
	}

	// Wakeup sensors
	if (has_rak1906)
	{
		startup_rak1906();
	}

	// Let sensors work for 10 seconds before reading them
	delay(10000);

	digitalWrite(LED_BLUE, HIGH);

	MYLOG("UPLINK", "Read Sensors");

	// Clear payload
	g_solution_data.reset();

	// Get sensor values
	if (has_rak1906)
	{
		read_rak1906();
		shutdown_rak1906();
	}

	float battery_reading = 0.0;
	// Add battery voltage
	for (int i = 0; i < 10; i++)
	{
		battery_reading += api.system.bat.get(); // get battery voltage
	}

	battery_reading = battery_reading / 10;

	g_solution_data.addVoltage(1, battery_reading);

	// Send the packet
	send_packet();
}
```

**`send_packet`** is checking whether the node is in LoRaWAN or LoRa P2P mode and sends the created payload depending on the selected mode.

```cpp
void send_packet(void)
{
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		MYLOG("UPLINK", "Sending packet # %d", my_fcount);
		my_fcount++;
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
	// It is P2P
	else
	{
		MYLOG("UPLINK", "Send packet with size %d over P2P", g_solution_data.getSize());

		digitalWrite(LED_BLUE, LOW);

		if (api.lorawan.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
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