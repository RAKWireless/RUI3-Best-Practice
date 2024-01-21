| <img src="../assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="../assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="../assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Example for a RUI3 based location tracker with the WisBlock RAK12500 GNSS module and the RAK1904 Acceleration sensor

Baseds on the [RUI3-LowPower-Example](../RUI3-LowPower-Example), this code implements a simple location tracker with adjustable send interval (heart beat) and an acceleration sensor to trigger location acquisition when the device is moving.

The GNSS sensor is powered down when not in use to minimize the power consumption. Once a location acquisition is triggered (by heart beat or motion), the GNSS module is powered up. To avoid power consuming waiting for a acquisition in a loop, a timer triggered task is checking the result every 2.5 seconds. If a location fix was found, the location data and acquisition time is added to the payload. If a time-out is reached during the acquisition, no location is added to the payload.    
Independent of the acquisition result, a payload is sent to the LoRaWAN server or LoRa P2P endpoint.    

### ⚠️ INFORMATION
The payload is in Cayenne LPP format with extended data types. A matching decoder can be found in the [RAKwireless_Standardized_Payload Github repo](https://github.com/RAKWireless/RAKwireless_Standardized_Payload/blob/main/RAKwireless_Standardized_Payload.js)


This examples includes three custom AT commands:     
- **`ATC+SENDINT`** to set the send interval time or heart beat time. If the device is not in motion it will send a payload with this interval. The time is set in seconds, e.g. **`AT+SENDINT=600`** sets the send interval to 600 seconds or 10 minutes.    
- **`ATC+MININT`** to set the minimal time between two location reports if the device is moving.
- **`ATC+STATUS`** to get some status information from the device.    

----

# Code sections

### ⚠️ INFORMATION    
The code snippets below are simplified. Check the provided source code for the full code.    

### ⚠️ INFORMATION    
This code requires additional libraries to be installed. The link to install them in Arduino IDE can be found in the file [_**app.h**_](./app.h).     
```cpp
#include <ArduinoJson.h> // Click here to install http://librarymanager/All#ArduinoJson
#include <CayenneLPP.h>	 // Click here to install http://librarymanager/All#CayenneLPP
```

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
		digitalWrite(LED_BLUE, LOW);
	}
}
```

LoRaWAN TX finished callback is an important callback. It tells you whether the transmission of a LoRaWAN packet is finished. It is important, because a TX finished event is waking up the device, and when using the loop() function, a sleep call should be issued _**AFTER**_ the TX finished event. The callback has the parameter _**`int32_t status`**_. The status is based on low level LoRaMAC stack results, at the moment the only usuable check is whether the status is 0 ==> send success or <>0 ==> send failed.    
In the sendCallback the interrupts for the motion sensor are enabled as well.

```cpp
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX status %d", status);

	// If it was motion triggered, reset the ACC interrupts
	if (has_rak1904)
	{
		clear_int_rak1904();
	}
}
```

LoRaWAN RX callback is called when a data packet was received from the LoRaWAN server. The callback has as parameter a pointer to a structure with information about the received data and pointers to the payload and its length.    

```cpp
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
}
```

LoRaWAN LinkCheck callback is called when LinkCheck was enabled through AT commands. It shows the demodulation margin, number of gateways that received the uplink and RSSI/SNR of the downlink.

```cpp
void linkcheckCallback(SERVICE_LORA_LINKCHECK_T *data)
{
	MYLOG("LC_CB", "%s Margin %d GW# %d RSSI%d SNR %d", data->State == 0 ? "Success" : "Failed",
		  data->DemodMargin, data->NbGateways,
		  data->Rssi, data->Snr);
}
```

LoRa P2P TX finished callback is an important callback. It tells you whether the transmission of a LoRa packet is finished. It is important, because a TX finished event is waking up the device, and when using the loop() function, a sleep call should be issued _**AFTER**_ the TX finished event. The callback has the no parameters, as LoRa P2P does not have any protocol that would allow to know if the sent packet was received by another node.
    
In the sendCallback the interrupts for the motion sensor are enabled as well.    

```cpp
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");

	// If it was motion triggered, reset the ACC interrupts
	if (has_rak1904)
	{
		clear_int_rak1904();
	}
}
```

LoRa P2P RX callback is called when a data packet was received from another LoRa node. The callback has as parameter a structure with information about the received data, the payload and its length.    

```cpp
void recv_cb(rui_lora_p2p_recv_t data)
{
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
}
```

If CAD is enabled, the CAD callback is called with the result of the channel activity detection. If a channel activity was detected, the packet should be re-sent. This is not implemented in this example.

```cpp
void cad_cb(bool result)
{
	MYLOG("CAD-CB", "P2P CAD reports %s", result ? "activity" : "no activity");
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

An additional timer is initialized for the periodic checking of the RAK12500 location acquisition status.    

```cpp
	// Create a timer.
	api.system.timer.create(RAK_TIMER_1, gnss_handler, RAK_TIMER_PERIODIC);
```

An third timer is initialized for to wakeup the module from the interrupt handler after a motion interrupt was triggered.    

```cpp
	// Create a timer
	api.system.timer.create(RAK_TIMER_2, sensor_handler, RAK_TIMER_ONESHOT);
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

**`sensor_handler`** is called by the timer or by an interrupt triggered by the accelerometer. First, if in LoRaWAN mode, it checks whether the node has already joined the network. 

```cpp
void sensor_handler(void *)
{
	if (api.lorawan.nwm.get() == 1)
	{
		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			// If it was motion triggered, reset the ACC interrupts
			if (has_rak1904)
			{
				clear_int_rak1904();
			}
			last_trigger = millis();
			return;
		}
	}

	digitalWrite(LED_BLUE, HIGH);
```
Then it checks whether the wakeup was triggered by a motion. This is not really required, it is just for debug output.    
```cpp
	// Just for debug, show if the call is because of a motion detection
	if (motion_detected)
	{
		MYLOG("UPLINK", "ACC triggered IRQ");
		motion_detected = false;
	}
```
Then it checks whether a location acquisition is already active (in case the motion sensor triggered while a heart beat event is already active). If no acquisition is active, it prepares the payload that will be sent after the acquisition has finished.    
Then it initializes the GNSS module and starts the counter that is checking the status every 2.5 seconds.    
```cpp
	// If GNSS is not yet active, start the timer to aquire the location
	if (!gnss_active)
	{
		// Clear payload
		g_solution_data.reset();

		// Add battery voltage
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());
		// Set flag for GNSS active to avoid retrigger */
		gnss_active = true;
		// Startup GNSS module
		init_gnss();
		check_gnss_counter = 0;
		// Max location aquisition time is half of minimal interval
		check_gnss_max_try = custom_parameters.min_interval / 2 / 2500;
		gnss_start = millis();
		// Start the timer
		api.system.timer.start(RAK_TIMER_1, 2500, NULL);
	}
	else
	{
		MYLOG("UPLINK", "GNSS still active");
	}
	digitalWrite(LED_BLUE, LOW);
}
```

**`send_packet`** is resetting the timer value for delays between motion trigger events. It adds as well (just for information), how long it took the GNSS engine to get a location fix.
```cpp
void send_packet(void)
{
	// Set flag for GNSS inactive */
	gnss_active = false;

	// Reset delay timer value
	last_trigger = millis();

	// Add acquisition time in seconds
	g_solution_data.addUnixTime(LPP_CHANNEL_GPS, (gnss_finished - gnss_start) / 1000);
```

### ⚠️ INFORMATION    
As the payload size might vary, depending on whether a location fix was achieved or not, an experimental adjustment of the data rate is implemented in this code, if LoRaWAN is active.    

```cpp
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Check DR
		uint8_t new_dr = get_min_dr(api.lorawan.band.get(), g_solution_data.getSize());
		if (new_dr != api.lorawan.dr.get())
		{
			api.lorawan.dr.set(new_dr);
			MYLOG("UPLINK", "Datarate changed to %d", api.lorawan.dr.get());
		}
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
			// If it was motion triggered, reset the ACC interrupts
			if (has_rak1904)
			{
				clear_int_rak1904();
			}
		}
	}
	// It is P2P
	else
	{
		MYLOG("UPLINK", "Send packet with size 4 over P2P");

		digitalWrite(LED_BLUE, LOW);

		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
			// If it was motion triggered, reset the ACC interrupts
			if (has_rak1904)
			{
				clear_int_rak1904();
			}
		}
	}
}
```

----

## RAK12500 initialization and location acquistion

**`init_gnss`** is called before every location acquisition, as the RAK12500 is powered down when not in use.    
```cpp
bool init_gnss(void)
{
	// Power on the GNSS module
	digitalWrite(WB_IO2, HIGH);

	// Give the module some time to power up
	delay(500);

	if (!my_gnss.begin(Wire))
	{
		MYLOG("GNSS", "Could not initialize RAK12500 on Wire");
		return false;
	}
	my_gnss.setI2COutput(COM_TYPE_UBX); // Set the I2C port to output UBX only (turn off NMEA noise)

	my_gnss.saveConfiguration(); // Save the current settings to flash and BBR

	my_gnss.setMeasurementRate(500);

	return true;
}
```

**`gnss_handler`** is the task called every 2.5 seconds while a location acquisition is active. It calls `poll_gnss` which is reading the status from the GNSS module, then, depending on the result, it is
- on a location fix stopping the acquisition and sending the payload
- checking if the maximum time is expired. If expired, it stops the acquisition and sends the payload 
```cpp
void gnss_handler(void *)
{
	digitalWrite(LED_GREEN, HIGH);
	if (poll_gnss())
	{
		// Power down the module
		digitalWrite(WB_IO2, LOW);
		delay(100);
		api.system.timer.stop(RAK_TIMER_1);
		gnss_finished = millis();
		digitalWrite(LED_BLUE, HIGH);
		send_packet();
	}
	else
	{
		if (check_gnss_counter >= check_gnss_max_try)
		{
			// Power down the module
			digitalWrite(WB_IO2, LOW);
			delay(100);
			api.system.timer.stop(RAK_TIMER_1);
			gnss_finished = millis();
			digitalWrite(LED_BLUE, HIGH);
			send_packet();
		}
	}
	check_gnss_counter++;
	digitalWrite(LED_GREEN, LOW);
}
```

**`poll_gnss`** is getting the location fix status from the GNSS module.    

```cpp
bool poll_gnss(void)
{
	last_read_ok = false;

	time_t time_out = millis();
	int64_t latitude = 0;
	int64_t longitude = 0;
	int32_t altitude = 0;
	int16_t accuracy = 0;
	uint8_t satellites = 0;

	bool has_pos = false;
	bool has_alt = false;

	if (my_gnss.getGnssFixOk())
	{
		byte fix_type = my_gnss.getFixType(); // Get the fix type
		char fix_type_str[32] = {0};
#if MY_DEBUG > 0
		if (fix_type == 0)
			sprintf(fix_type_str, "No Fix");
		else if (fix_type == 1)
			sprintf(fix_type_str, "Dead reckoning");
		else if (fix_type == 2)
			sprintf(fix_type_str, "Fix type 2D");
		else if (fix_type == 3)
			sprintf(fix_type_str, "Fix type 3D");
		else if (fix_type == 4)
			sprintf(fix_type_str, "GNSS fix");
		else if (fix_type == 5)
			sprintf(fix_type_str, "Time fix");
#endif
```
If the result of the location fix is satisfying, it saves the results and sets the flag `last_read_ok`.    
```cpp
		if ((fix_type >= 3) && (my_gnss.getSIV() >= 5)) /** Fix type 3D and at least 5 satellites */
														// if (fix_type >= 3) /** Fix type 3D */
		{
			last_read_ok = true;
			latitude = my_gnss.getLatitude();
			longitude = my_gnss.getLongitude();
			altitude = my_gnss.getAltitude();
			accuracy = my_gnss.getHorizontalDOP();
			satellites = my_gnss.getSIV();

			// MYLOG("GNSS", "Fixtype: %d %s", my_gnss.getFixType(), fix_type_str);
			// MYLOG("GNSS", "Lat: %.4f Lon: %.4f", latitude / 10000000.0, longitude / 10000000.0);
			// MYLOG("GNSS", "Alt: %.2f", altitude / 1000.0);
			// MYLOG("GNSS", "Acy: %.2f ", accuracy / 100.0);
		}
	}
```
In the next section it checks the result and adds the location data if it is valid.
```cpp
	char disp_str[255];
	if (last_read_ok)
	{
		if ((latitude == 0) && (longitude == 0))
		{
			last_read_ok = false;
			return false;
		}

		// Add the location to the payload
		g_solution_data.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
		return true;
	}
	else
	{
```
If no location fix was found, it returns a false to the timer triggered task.    
### ⚠️ INFORMATION    
For indoor testing it is possible to return a fake location by defining the _**`FAKE_GPS`**_ with 1.    

```cpp
		// No location found
#if FAKE_GPS > 0
		MYLOG("GNSS", "Faking GPS");
		// 14.4213730, 121.0069140, 35.000
		latitude = 144213730;
		longitude = 1210069140;
		altitude = 35000;
		accuracy = 1;
		satellites = 18;

		g_solution_data.addGNSS_6(LPP_CHANNEL_GPS, latitude, longitude, altitude);
		last_read_ok = true;
		return true;
#endif
	}

	// MYLOG("GNSS", "No valid location found");
	last_read_ok = false;
	return false;
}
```

----

## RAK1904 initialization and interrupt handler

### ⚠️ INFORMATION    
The RAK1904 requires some special setting to get the lowest power consumption. The used library does not allow to access the GNSS chip directly. As a workaround two functions were added to read and write directly from the GNSS chip, _**`rak1904_writeRegister`**_ and _**`rak1904_readRegister`**_.    

_**`init_rak1904`**_ is initializing up the acceleration sensor to 10 Hz sampling rate, 4G measurement range and a sensitivity of 1/8th of the measurement range.    
It is as well initializing the interrupt callback function on a GPIO.    

### ⚠️ INFORMATION    
Depending on the WisBlock Sensor Slot used, the GPIO connected to the interrupt output of the acceleration sensor is changing. It has to be set to the correct pin with    
```cpp
/** Interrupt pin, depends on slot */
uint8_t acc_int_pin = WB_IO5; // Slot D
```    
in the beginning of the RAK1904_acc.cpp file.

```cpp
bool init_rak1904(void)
{
	// Setup interrupt pin
	pinMode(acc_int_pin, INPUT);

	Wire.begin();
	usedWire = &Wire;

	acc_sensor.setDataRate(LIS3DH_DATARATE_10_HZ);
	acc_sensor.setRange(LIS3DH_RANGE_4_G);

	if (!acc_sensor.begin())
	{
		MYLOG("ACC", "ACC sensor initialization failed");
		return false;
	}

	// Enable interrupts
	acc_sensor.enableDRDY(true, 1);
	acc_sensor.enableDRDY(false, 2);

	// ........................
	// Setup of the registers of the acceleration sensor
	// ........................

	// Set the interrupt callback function
	// MYLOG("ACC", "Int pin %s", acc_int_pin == WB_IO3 ? "WB_IO3" : "WB_IO5");
	attachInterrupt(acc_int_pin, int_callback_rak1904, RISING);

	last_trigger = millis();

	return true;
}
```

_**`int_callback_rak1904`**_ is the callback triggered by the interrupt of the acceleration sensor. It first checks if a minimum time has expired since the last motion trigger. This is to avoid to send too many data packets. The delay between two packets can be set with a custom AT command.    
If the time since the last trigger is sufficient long, it sets a flag and calls the _**`sensor_handler`**_ to signal the start of a location acquisition.    
It resets as well the heart beat timer.    

```cpp
void int_callback_rak1904(void)
{
	if ((millis() - last_trigger) > (custom_parameters.min_interval) && !gnss_active)
	{
		motion_detected = true;
		// Stop a timer.
		api.system.timer.stop(RAK_TIMER_0);
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
		// Wake the handler and start location acquisition
		api.system.timer.start(RAK_TIMER_2, 100, NULL);
	}
	else
	{
		clear_int_rak1904();
		motion_detected = false;
	}
}
```

_**`clear_int_rak1904`**_ is re-enabling the interrupts from the acceleration sensor. This function is called after an acquisition attempt has finished.

```cpp
void clear_int_rak1904(void)
{
	acc_sensor.readAndClearInterrupt();
}
```
----

# Get RUI3 devices

Get a RAKwireless RUI3 WisDuo stamp module, breakout board or evaluation board from our [store](https://store.rakwireless.com/collections/new-menu-modules)

Get a RAKwireless RUI3 WisBlock Core module from our [store](https://store.rakwireless.com/collections/wisblock-core)

### ⚠️ INFORMATION 
The following modules are supported by RUI3:    

| _**WisDuo modules and EVB's**_ | _**WisBlock Core Modules**_ |
| --- | --- |
| - [RAK3172 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Module/Overview/) | - [RAK4631-R](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631-R/Overview/) |
| - [RAK3272S breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3272S-Breakout-Board/Overview/) | - [RAK3372](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK3372/Overview/) |
| - [RAK3172 evaluation board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Evaluation-Board/Overview/) | - [RAK11722](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11722/Overview/) |
| - [RAK3172-SiP chip](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-SiP/Overview/) |  |
| - [RAK3272-SiP breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3272-SiP-Breakout-Board/Overview/) |  |
| - [RAK4630 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK4630-Module/Overview/) |  |
| - [RAK11720 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11720-Module/Overview/)     |  |
| - [RAK11721 breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11721-Breakout-Board/Overview/)     |  |

----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.