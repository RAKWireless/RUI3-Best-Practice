# Build a door open counter and alarm sensor with WisBlock

Reed relays are often used for contactless switching in many applications. The reed relay is normally open, but when a magnet is getting close to it, the relay contacts will close.

Here we will use this feature to create door (or window) open/close detector. This can be useful for home alarm systems or in industrial applications where an automatic door in a production line requires to count how often it is opened and closed or to make sure that the door is actually working.    

<center>
<img
  src=".\assets\\sensor-plate-2.jpg"
  width="60%"
  caption="Door sensor"
/>
</center>

----

The RAK13011 magnetic switch module includes a WisBlock module, an external reed relay and a magnet to control the reed relay.    

The RAK13011 module fits into a sensor slot, but due to its size it occupies two sensor slots. It is recommended to use Slot C of the [RAK19003](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK19003/Overview/) Base Board or Slot D of the [RAK19007](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK19007/Overview/) Base Board. Using these slots will not occupy two sensor slots, as they are designed for these longer sensor modules.    

<center>
<img
  src=".\assets\\module-and-reed-magnet.jpg"
  width="60%"
  caption="RAK13011"
/>
</center>

### 📝 NOTE    
> All the modules are available at the [RAKwireless Store](https://store.rakwireless.com/pages/wisblock).


----


## A few things that have to be considered

### Transmission interval

The application is using LoRaWAN to transmit the status through a gateway to a LoRaWAN server. Integrations connected to the LoRaWAN server can then be used to take actions.    

As LoRaWAN is not a _**fast**_ communication medium, a ring buffer is used to store open and close events in case they are happening faster than the WisBlock Core module can transmit the events.    
The buffer is designed to keep up to 50 events, but it can be increased if required.

----

### Debouncing of the relay contacts

The reed relay contacts do not have any debouncing mechanism. It can easily happen that the bouncing of the relay contacts triggers multiple interrupts. The debouncing is done in the application by checking if the status has changed since the last interrupt.    

----

### RAKwireless RUI3 API

This source code is for RUI3 and works with the RAK4631-R, the RAK3372 and the RAK11722 Core modules. 

### ⚠️ WARNING    
> Due to a GPIO conflict with the RAK3372, The RAK13011 does not work when using Sensor Slot C. With the RAK3372 the RAK13011 must be used in another Sensor Slot.    

### ⚠️ IMPORTANT    
> Select the sensor slot in the file _**`RAK13011_switch.cpp`**_. This is important, because the GPIO used for interrupts is different for every Sensor Slot.    

----

### Enclosure

When using as a window or door sensor, there are two options to deply the sensors.    
1) Use the encapsulated reed relay and magnet parts and connect the RAK13011 with wires. But this is not a very elegant solution and can look very ugly.
2) Take the reed relay out fo the encapsulated part (or just buy a separate reed relay) and put the WisBlock modules, the battery and the reed relay into a small custom enclosure.

For this tutorial I chose the second option:     
| <img src=".\assets\\sensor-open.jpg" caption="RAK13011"/> | <img src=".\assets\\sensor-place-1.jpg" caption="RAK13011"/> |
| --- | --- |

----

## Hardware Used

| Module                                                                                  | Function                        |
| --------------------------------------------------------------------------------------- | ------------------------------- |
| [RAK4631-R](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631-R/Overview/)   | WisBlock Core Module            |
| [RAK3372](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Evaluation-Board/)   | WisBlock Core Module            |
| [RAK11722](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11722/Overview/)   | WisBlock Core Module            |
| [RAK19003](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK19003/Overview/) | WisBlock Base Board             |
| [or RAK19007 (for RAK3372)](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK19007/Overview/) | WisBlock Base Board             |
| [RAK13011](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK13011/Overview/)   | WisBlock Magnetic Switch     |
| [Optional RAK1901](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK1901/Overview/)   | WisBlock Temperature and Humidity Sensor     |

----

## Software Used

<b>1. IDE </b>

  RUI3
  - [Arduino IDE](https://www.arduino.cc/en/software) (requires some file name changes and manual installation of libraries)
  - [RUI3 BSP for RAK4631, RAK3372 and RAK11722](https://github.com/RAKWireless/RAKwireless-Arduino-BSP-Index)

<b>2. LoRaWAN and BLE Communication </b>

  - [CayenneLPP](https://registry.platformio.org/libraries/sabas1080/CayenneLPP)
  - [ArduinoJson](https://registry.platformio.org/libraries/bblanchon/ArduinoJson)

### 📝 NOTE
For the RUI3 version, all libraries must be installed manually with the Arduino IDE Library Manager.


----


## How Does the Application Work?

The WisBlock Core module is going into sleep mode after it connected to the LoRaWAN server.    

There are two events that are waking up the WisBlock Core module:    
- Door opening or closing event triggers an interrupt 
- (optional) the module can wakeup in a configurable interval to send an _**\<I am Alive\>**_ message to the LoRaWAN server. 

Everything else is handled in the background by RUI3. 

### 📝 NOTE    
> Code examples used are from the PlatformIO version of the code.    
These is not the complete code, only snippets taken from the sources.    


In case a door opening or closing event is detected, the application will receive a wake up signal from the interrupt callback. It will then start the debounce logic by starting a timer to recheck the signal status. 

```cpp
void switch_int_handler(void)
{
	MYLOG("REED", "Interrupt, start bounce check");
	switch_status = digitalRead(SW_INT_PIN);
	api.system.timer.start(RAK_TIMER_2, 50, NULL);
}
```

The application will then check the status of the relay to eliminate bouncing effects. 

```cpp
void switch_bounce_check(void *)
{
	MYLOG("REED", "Bounce check");
	int new_switch_status = digitalRead(SW_INT_PIN);
	if (new_switch_status != switch_status)
	{
		MYLOG("REED", "Bounce detected");
		return;
	}
	if (switch_status == LOW)
	{
		if (!Fifo.enQueue(false))
		{
			MYLOG("REED", "FiFo full");
			return;
		}
	}
	else
	{
		if (!Fifo.enQueue(true))
		{
			MYLOG("REED", "FiFo full");
			return;
		}
	}

	// ...
```

Next the event will be added to the ring buffer. If there is still a packet sending activity, a timer will be started to delay the next packet sending    

```cpp
/
	MYLOG("REED", "Added event to queue");
	if (handler_available)
	{
		MYLOG("REED", "Start event handler");
		// Wake the switch handler
		handler_available = false;
		handle_rak13011(NULL);
	}
}
```

In case it is possible to send the packet immediately, the event type is changed and the sending routine is called. First the battery level is measured, then the oldest event status is pulled ring buffer and both values are added to the LoRaWAN packet.
```cpp
	if (!tx_active)
	{
		// Reset automatic interval sending if active
		if (g_custom_parameters.send_interval != 0)
		{
			// Restart a timer
			api.system.timer.stop(RAK_TIMER_0);
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}

		// Clear payload
		g_solution_data.reset();

		noInterrupts();
		g_solution_data.addPresence(LPP_CHANNEL_SWITCH, !Fifo.deQueue() ? 0 : 1);
		interrupts();

		// Add battery voltage
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

		// Send the packet
		send_packet();
	}
```

----

## LoRaWAN Packet Format

The packet data is made compatible with the extended Cayenne LPP encoding from [ElectronicCats/CayenneLPP](https://github.com/ElectronicCats/CayenneLPP).

The content of the packet depends on the modules installed on the WisBlock Base Board:

| Data                     | Channel No. | Channel ID | Length        | Comment                                                         | Required Module   | Decoded Field Name |
| ------------------------ | ----------- | ---------- | ------------- | --------------------------------------------------------------- | ----------------- | ------------------ |
| Battery value            | 1           | _**116**_  | 2&nbsp;bytes  | 0.01&nbsp;V Unsigned MSB                                        | RAK4631           | voltage_1          |
| Door event               | 48          | 102        | 1&nbsp;bytes  | bool                                                            | RAK13011          | presence_48        |

### 📝 NOTE
> - The Channel IDs in cursive are extended format and not supported by standard Cayenne LPP data decoders.
> - A matching decoder can be found in the [RAKwireless_Standardized_Payload Github repo](https://github.com/RAKWireless/RAKwireless_Standardized_Payload/blob/main/RAKwireless_Standardized_Payload.js)



----

## Using Datacake to show the door opening and closing events

As a simple example to visualize the door events, I created a device in [**Datacake**](https://datacake.co).
Datacake is an easy-to-use **Low Code IoT** platform. In my Datacake account, I set up the device with the matching payload decoder, visualization, and creation of an email alert.

### Datacake Payload Decoder

In the device configuration, the Datacake decoder from the  [RAKwireless_Standardized_Payload Github repo](https://github.com/RAKWireless/RAKwireless_Standardized_Payload/blob/main/RAKwireless_Standardized_Payload.js) folder is used.

----

### Datacake Fields

The battery values and the door events are stored in Datacake fields. The door events are called **`Window 1`** because I started the project as a window opening alarm system.

| Field Name              | Identifier          | Type    |
| ----------------------- | ------------------- | ------- |
| Battery                 | VOLTAGE_1           | Float   |
| Window 1                | PRESENCE_48         | Boolean |

----

### Datacake Visualization

In the dashboard, you can show the current status of the door/window. In the latest code, I added a temperature and humidity sensor, so these values are shown as well (only when using the open source Arduino BSP).

<img
  src=".\assets\\datacake.png"
  width="100%"
  caption="Window/Door switch Datacake dashboard"
/>
