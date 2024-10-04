| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Example for RAK3172 use with a DS18B20 one wire sensor

This code uses the open source library [GyverDS18](https://github.com/GyverLibs/GyverDS18) provided by Alex Gyver to read temperature values from a DS18B20 sensor.    
Due to different GPIUO handling, the library has been changed and is included in the source code.    
For usage of the library, see [README-microDS18B20](./README-microDS18B20.md)    

### _**Thanks to Alex Gyver for providing his code to the public domain!**_    

It is based on the low power example and is event driven. The read intervals can be set by using the ATC+SENDINT command. The send interval is saved in the flash of the device.

### ⚠️ WARNING    
The code is only working with the RAK3172.    

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

Second a periodic timer is initialized to wake up the system in intervals to send a packet to the LoRaWAN server or other LoRa P2P nodes. The interval time is set with the variable **`g_send_repeat_time`**.

```cpp
	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, sensor_handler, RAK_TIMER_PERIODIC);
	if (g_send_repeat_time != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, g_send_repeat_time, NULL);
	}
```

The **`loop()`** function does nothing beside of killing itself, which prevents that it is called frequently from the underlaying RUI3 scheduler.

```cpp
void loop()
{
	api.system.scheduler.task.destroy();
}
```

## send_handler & send_packet

This functions are where the action is happening. 

**`send_handler`** is called by the timer. First, if in LoRaWAN mode, it checks whether the node has already joined the network. Then it is reading two DS18B20 sensors and adding their readings together with the current battery status to the payload. Payload is formatted in Cayenne LPP format.        
After the payload is ready, it calls **`send_packet`** to get the packet sent out.

```cpp
void sensor_handler(void *)
{
	if (api.lora.nwm.get() == 1)
	{ // Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			return;
		}
	}

	// Clear payload
	g_solution_data.reset();

	// Create payload

	// Read temperature sensors
	digitalWrite(WB_IO2, HIGH);
	delay(250);

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

## Check and initialize the DS18B20 sensors

Checking if a DS18B20 sensor is connected is done by reading its address. If the returned address is `0`, then no sensor was found. This is done in `setup()`:    

```cpp
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
```
## Read data from the sensors

Reading from the sensors is done by initiating a request, waiting for the conversion time and then reading the temperature value:    

```cpp
bool read_ds18b20(float &ds_temp_1, float &ds_temp_2)
{
    if (has_ds_1)
        ds_1.requestTemp();
    if (has_ds_2)
        ds_2.requestTemp();

    delay(1000);

    bool result = true;
    if (has_ds_1)
        if (ds_1.readTemp())
            ds_temp_1 = ds_1.getTemp();
        else
            result = false;
    if (has_ds_2)
        if (ds_2.readTemp())
            ds_temp_2 = ds_2.getTemp();
        else
            result = false;
    return result;
}
```

----

# Get RUI3 devices

Get a RAKwireless RUI3 WisDuo stamp module, breakout board or evaluation board from our [store](https://store.rakwireless.com/collections/new-menu-modules)

Get a RAKwireless RUI3 WisBlock Core module from our [store](https://store.rakwireless.com/collections/wisblock-core)

### ⚠️ INFORMATION 
Only the following modules are supported by RUI3:    

_**WisDuo**_
- RAK3172 stamp module, breakout board, evaluation board
- RAK3172-SiP chip, breakout board
- RAK4630 stamp module
- RAK11720 stamp module, breakout board    

_**WisBlock Core**_
- RAK4631-R
- RAK3372
- RAK11722

----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.