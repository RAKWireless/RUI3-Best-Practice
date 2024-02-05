| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Example how control a relay with downlinks with RUI3

One problem often seen when writing custom code with RUI3 is that the **`loop()`** is used together with **`api.system.sleep.all()`**. This is IMHO not the best solution to achieve lowest power consumption and brings some timing problems, as when a LoRa or LoRaWAN packet is sent, the TX finished event will wake up the system and it will not go back to sleep.

This example code is _**NOT**_ using the loop at all. Instead it is complete event driven. The WisDuo/WisBlock module is sleeping unless an event occurs. An event can be a timer callback or an external interrupt, or if using LoRaWAN Class C, it can be a packet received from the LoRaWAN server.

This code does setup a timer that wakes up the device in the desired send interval, send a packet and then the system goes back to sleep automatically.    
It can be extended to use external interrupts, see my other examples codes.

The application is listening to downlinks on _**fPort 10**_. If the downlink is in the hex format _**`AAFFxx`**_ it accepts the downlink as a command to set (xx = 01) or reset (xx = 00) a relay that is controlled over the WB_IO4 GPIO. Even this could be done directly from the receive callbacks, it uses a timer to start a function that controls the GPIO.

The downlink to the LoRaWAN server sends back in a programmable interval the status of the relay.

### ⚠️ INFORMATION    
If used with LoRaWAN, the node has to be setup as Class C node to receive downlinks from the LoRaWAN server immediately. If set as Class A, the downlink will be only received after an uplink (heart beat) was performed and the relay status will not change immediately.    

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
The function checks then the fPort and proper format of the payload. If it is accepted as a command, it calls the relay_handler function through a timer to change the status of the relay.    

```cpp
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);

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
				relay_status = data->Buffer[2];
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
```

LoRa P2P TX finished callback is an important callback. It tells you whether the transmission of a LoRa packet is finished. It is important, because a TX finished event is waking up the device, and when using the loop() function, a sleep call should be issued _**AFTER**_ the TX finished event. The callback has the no parameters, as LoRa P2P does not have any protocol that would allow to know if the sent packet was received by another node..

```cpp
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");
}
```

LoRa P2P RX callback is called when a data packet was received from another LoRa node. The callback has as parameter a structure with information about the received data, the payload and its length.    
The function checks then the proper format of the payload. If it is accepted as a command, it calls the relay_handler function through a timer to change the status of the relay.    

```cpp
void recv_cb(rui_lora_p2p_recv_t data)
{
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);

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

After that another timer is initialized that is used to handle the received packet. Here the GPIO that controls the relay is initialized as well.   

```cpp
	// Create a timer to handle incoming packets
	api.system.timer.create(RAK_TIMER_1, relay_handler, RAK_TIMER_ONESHOT);

	// Initialize relay control port
	pinMode(RELAY_IO, OUTPUT);
	digitalWrite(RELAY_IO, relay_status);
```

The **`loop()`** function does nothing beside of killing itself, which prevents that it is called frequently from the underlaying RUI3 scheduler.

```cpp
void loop()
{
	api.system.scheduler.task.destroy();
}
```

## relay_handler

This function is called when a valid command packet was received. It is using the received value to set or reset the relay.    

```cpp
void relay_handler(void *) {
	MYLOG("DOWNLINK", "Set relay");
	digitalWrite(RELAY_IO, relay_status);
}
```

## send_handler & send_packet

This functions are called be a timer. They act as a heart-beat and feedback of the relay status. 

**`send_handler`** is called by the timer. First, if in LoRaWAN mode, it checks whether the node has already joined the network. Then it creates a payload that includes the battery status and the relay status.    
After the payload is ready, it calls **`send_packet`** to get the packet sent out.

```cpp
void sensor_handler(void *)
{
	if (api.lora.nwm.get() == 1)
	{ // Check if the node has joined the network
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
```

**`send_packet`** is checking whether the node is in LoRaWAN or LoRa P2P mode and sends the created payload depending on the selected mode.

```cpp
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