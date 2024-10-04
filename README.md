| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# RUI3 Best Practices

----
----
#### ⚠️ Important note about RUI3 version compatibility 
The examples require RUI3 V4.1.1.    
_**Older RUI3 versions do not support the LinkCheck functionality!**_    
_**In addition the format of API calls for LoRa P2P have changed in RUI3 V4.1.1!**_     

The examples for RUI3 V4.1.0 and older versions are in the [branch V4.1.0-older](https://github.com/RAKWireless/RUI3-Best-Practice/tree/V4.1.0-older).        

Installation guides for Arduino IDE and Visual Studio Code can be found in the [RAK Documentation Center](https://docs.rakwireless.com/RUI3/Supported-IDE/).    

----
----

This repo provides additional examples for RUI3. It focuses on low power consumption and practical examples for range tests, location tracking and sensing applications using the WisBlock Modular System.    

A collection of tips and tricks about the usage or RUI3 is as well available in the [Tips-n-Tricks](./Tips-n-Tricks.md) file.

### ⚠️ WARNING 
_**Due to the limited Flash and RAM size of the RAK3172, some of the examples require code size reduction before they fit into the available memory!**_    
_**Actions to reduce code size can be**_
- _**Limit the supported LoRaWAN regions in the `Arduino Board Manager Menu`**_
- _**Disable the debug output of the examples code. This is done by setting `#define MY_DEBUG 0` in the file `app.h` of the examples**_

### ⚠️ INFORMATION 
#### ⚠️ Usage info
All examples are prepared to work in either LoRaWAN or LoRa P2P mode.    
All examples are using AT commands for setup of the device instead of hard-coding the parameters with API calls!.

#### ⚠️ Required libraries not available in the Arduino Library Manager
Some of the examples are using RUI3 libraries for the WisBlock modules. These libraries are simplified (to save code space on the RAK3172) and can be downloaded as ZIP files from
- [RAKwireless_RAK1901_Temperature_and_Humidity_SHTC3.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1901_Temperature_and_Humidity_SHTC3.zip)
- [RAKwireless_RAK1902_Pressure_LPS22HB.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1902_Pressure_LPS22HB.zip)
- [RAKwireless_RAK1903_Optical_OPT3001.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1903_Optical_OPT3001.zip)
- [RAKwireless_RAK1904_Accelerate_LIS3DH.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1904_Accelerate_LIS3DH.zip)
- [RAKwireless_RAK1906_Environment_BME680.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1906_Environment_BME680.zip)
- [RAKwireless_RAK1910_GPS_UBLOX7.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK1910_GPS_UBLOX7.zip)
- [RAKwireless_RAK5860_LTE_NBIoT_Module.zip](https://downloads.rakwireless.com/RUI/RUI3/Library/RAKwireless_RAK5860_LTE_NBIoT_Module.zip)    

The libraries can be installed with the "Install from ZIP" function in the ArduinoIDE Library Manager.    
The libraries can be installed as well manually by unzipping the content into the default Arduino Library folder.

#### ⚠️ Supported RUI3 modules and devices
The following modules are supported by RUI3:    

| _**WisDuo modules and EVB's**_ | _**WisBlock Core Modules**_ |
| --- | --- |
| [RAK3172 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Module/Overview/) | [RAK4631-R](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631-R/Overview/) |
| [RAK3272S breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3272S-Breakout-Board/Overview/) | [RAK3372](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK3372/Overview/) |
| [RAK3172 evaluation board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-Evaluation-Board/Overview/) | [RAK11722](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11722/Overview/) |
| [RAK3172-SiP chip](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3172-SiP/Overview/) |  |
| [RAK3272-SiP breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK3272-SiP-Breakout-Board/Overview/) |  |
| [RAK4630 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK4630-Module/Overview/) |  |
| [RAK11720 stamp module](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11720-Module/Overview/)     |  |
| [RAK11721 breakout board](https://docs.rakwireless.com/Product-Categories/WisDuo/RAK11721-Breakout-Board/Overview/)     |  |

----

# Available Examples

## [RUI3-LowPower-Example](./RUI3-LowPower-Example)
The simpliest example is just showing how to design an application that is event based and keeps the MCU and LoRa transceiver in low power mode whenever possible.    
It includes as well a simple example how to implement custom AT commands.

## [RUI3-RAK1901-RAK1902-Example](./RUI3-RAK1901-RAK1902-Example)    
A simple sensor application based on RAK1901 and RAK1902. It reads temperature, humidity and barometric pressure from these sensors and sends them in a configurable interval.    
This example is using the Cayenne LPP data format to send the formatted payload.    

## [RUI3-Sensor-Node-EPD-2.13](./RUI3-Sensor-Node-EPD-2.13)    
A simple sensor application based on RAK1906 and optional RAK1903 or RAK12010. It reads temperature, humidity and barometric pressure and environment light from these sensors. It displays the values on the RAK14000 and sends them in a configurable interval.    
This example is using the Cayenne LPP data format to send the formatted payload.    
Due to the memory required for driving the EPD, when using with the RAK3172, the LoRaWAN regions have to be reduced to a few to have enough flash available.    

## [RUI3-Sensor-Node-EPD-2.13 for Visual Studio Code](./RUI3-Sensor-Node-EPD-2.13-VSC)    
Same example as `RUI3-Sensor-Node-EPD-2.13`, but this project uses some of the VSC tricks from the [Tips-n-Tricks](Tips-n-Tricks.md) for Visual Studio Code.

## [RUI3-RAK12007-Waterlevel](./RUI3-RAK12007-Waterlevel)    
This example uses the RAK12007 ultrasonic sensor module to measure the water level in a water tank.    
This example is using the Cayenne LPP data format to send the formatted payload.    

## [RUI3-RAK12500-RAK1904-GNSS](./RUI3-RAK12500-RAK1904-GNSS)    
A motion based location tracker using the RAK12500 and RAK1904 WisBlock modules. Based on the RUI3-LowPower-Example code.    
This example is using the Cayenne LPP data format to send the formatted payload.    

## [RUI3-RAK13011-Alarm](./RUI3-RAK13011-Alarm)
A door or window alarm system using the RAK13011 magnetic relay switch module. This examples features the usage of a queue to collect events. For the alarm system (or open/close counter), this was necessary, because events could be triggered faster than the uplink packets can be sent over LoRaWAN.

## [RUI3-RAK5811-Example](./RUI3-RAK5811-Example)
Example code for using the RAK5811 0-5V analog input module with RUI3. For lowest power consumption, the RAK5811 is powered down until a data reading is required. The interval time for reading the data can be setup with AT command.    

## [RUI3-RAK12022](./RUI3-RAK12022)
Example code for using the RAK12022 PT100 sensor module with RUI3. For lowest power consumption, the RAK120221 is powered down until a data reading is required. The interval time for reading the data can be setup with AT command.    

## [RUI3-Sensor-Node-Air-Quality](./RUI3-Sensor-Node-Air-Quality)
A sensor application that combines temperature, humidity, barometric pressure, CO2 and VOC sensing. It detect automatically which sensors are attached and adjusts its payload to the available sensor data. It supports WisBlock RAK1906, RAK12037 and RAK12047 sensor modules

## [RUI3-Signal-Meter-P2P-LPWAN](./RUI3-Signal-Meter-P2P-LPWAN)
This is a PoC of a very basic signal tester. It works in both LoRa P2P and LoRaWAN mode. It uses a simple OLED display and has no GUI. All settings are done over USB with RUI3 AT commands.    
It is a very simple (~30 US$ cheap) device that can help to check LoRa and LoRaWAN coverage. It does not claim to be a super precise instrument, it is just an affordable small instrument to check signal coverage.    

## [RUI3-Relay-Class-C](./RUI3-Relay-Class-C)
A control application that changes the status of a relay depending on a received data packet. This is as well an example how to use LoRaWAN Class C to be able to receive downlink packets from the LoRaWAN server at any time. It supports WisBlock RAK13001 and RAK13007 relay modules

## [RUI3 Modbus Master example](./ModBus)     
This example is a simple Modbus Master that reads sensor values from a Modbus Slave. For easy testing a Modbus Slave application is available for another WisBlock/WisDuo based module with a temperature, humidity and barometric pressure sensor.    
_**⚠️ INFORMATION**_      
This example uses a modified version of the [Modbus-Master-Slave-for-Arduino](https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino) library. This library was choosen because of its small code size. However, due to some incompatible definitions, it did not compile with RUI3. The library was slightly modified to work with RUI3 and is included as project files _**`RUI3_ModbusRtu.cpp`**_ and _**`RUI3_ModbusRtu.h`**_.    
The original libray is licensed under the [GNU LESSER GENERAL PUBLIC LICENSE Version 2.1](https://github.com/smarmengol/Modbus-Master-Slave-for-Arduino/blob/master/LICENSE.md)

## [LoRa P2P](./LoRa_P2P)
RUI3 code example for a simple LoRa P2P communication. _**LoRa_P2P_TX**_ is sending packets in intervals (can be setup with custom AT command ATC+SENDINT=xxx (xxx in seconds)). _**LoRa_P2P_RX**_ is listening to incoming packets and response within a second with its own packet.

## [RUI3-Mesh](./RUI3-Mesh)
This is a PoC of a simple LoRa Mesh Network. It includes everything to setup a Mesh Network (compatible with [WisBlock-API-Mesh](https://github.com/beegee-tokyo/WisBlock-API-Mesh)).    
It supports a limited number of Mesh nodes (15 with the RAK3172 and 30 with the RAK4630 and RAK11720) and sends a dummy data packet.    
Beside of packet broadcasting it supports sending data to a specific other node and it is as well possible to define a master node to which all other nodes are sending the packets.    

## [RUI3-WisBlock-Kit-1](./RUI3-WisBlock-Kit-1)
RUI3 code example for the [WisBlock Kit 1](https://store.rakwireless.com/products/wisblock-kit-1-weather-monitor) which includes RAK1901 (temperature and humidity), RAK1902 (barometric pressure) and RAK1903 (light) sensor support. Ideal if the Kit 1 is changed to use RUI3 for lower the power consumption, specially if the RAK4631 is replaced with a RAK3372 WisBlock Core Module (RAK3172).    

## [RUI3-WisBlock-Kit-4](./RUI3-WisBlock-Kit-4)
RUI3 code example for the [WisBlock Kit 4](https://store.rakwireless.com/products/wisblock-kit-4-air-quality-monitor) which uses the RAK1906 (temperature, humidity and barometric pressure) sensor. Ideal if the Kit 4 is changed to use RUI3 for lower the power consumption, specially if the RAK4631 is replaced with a RAK3372 WisBlock Core Module (RAK3172).    

## [RUI3 GNSS Location](./RUI3-GNSS)
RUI3 code example for an I2C or serial connected GNSS module based on u-blox MAX-7Q (RAK1910) or u-blox ZOE-M8Q (RAK12500).    
Then it starts a location acquisition in the time interval set with ATC+SENDINT. It will try to get a location in 1/2 the time of the acquisition time.     
If a location was aquired, it will send the location in Cayenne LPP format over LoRaWAN or LoRa P2P. If no location was found, it will send only the battery voltage.     

This library is tested with u-blox MAX-7Q (RAK1910) and u-blox ZOE-M8Q (RAK12500).
It uses Sparkfun libraries over I2C (RAK12500) or Serial (RAK1910) and UBX messages, which are shorter than NMEA messages and reduce the communication effort.    
For the u-blox MAX-7Q (RAK1910) an adapted version of the Sparkfun library is used.

## [RUI3 RAK3172 DS18B20 OneWire Sensor](./RUI3-RAK3172-OneWire)
RUI3 had at the time of release timing problems with OneWire communications. This example code uses a changed open source library [GyverDS18](https://github.com/GyverLibs/GyverDS18) provided by Alex Gyver for the communication.     
The example reads the sensor ID and the sensor values in an interval that can be setup with _**`ATC+SENDINT`**_. It sends the sensor values in Cayenne LPP packet format over LoRaWAN or LoRa P2P.    

_**⚠️ INFORMATION**_      
This example works only with the RAK3172 modules. It is not compatible with the RAK4630 or RAK11720.    

----

# Get RUI3 devices

Get a RAKwireless RUI3 WisDuo stamp module, breakout board or evaluation board from our [store](https://store.rakwireless.com/collections/new-menu-modules)

Get a RAKwireless RUI3 WisBlock Core module from our [store](https://store.rakwireless.com/collections/wisblock-core)

----
----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.