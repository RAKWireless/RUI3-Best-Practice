| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# RUI3 Best Practices

This repo provides additional examples for RUI3. It focuses on low power consumption and practical examples for range tests, location tracking and sensing applications using the WisBlock Modular System.    

### ⚠️ WARNING 
_**Due to the limited Flash and RAM size of the RAK3172, some of the examples require code size reduction before they fit into the available memory!**_    
_**Actions to reduce code size can be**_
- _**Limit the supported LoRaWAN regions in the `Arduino Board Manager Menu`**_
- _**Disable the debug output of the examples code. This is done by setting `#define MY_DEBUG 0` in the file `app.h` of the examples**_

### ⚠️ INFORMATION 
All examples are prepared to work in either LoRaWAN or LoRa P2P mode.    
All examples are using AT commands for setup of the device instead of hard-coding the parameters with API calls!.

### ⚠️ INFORMATION
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

### ⚠️ INFORMATION 
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

### ⚠️ INFORMATION 
These examples require at least RUI3 V4.1.0 or newer.
RUI3 V4.1.0 can be installed by using the Arduino BSP JSON file _**`package_rakwireless.com_rui_index.json`**_ that can be found in the [BSP repository](https://github.com/RAKWireless/RAKwireless-Arduino-BSP-Index).     
Installation guides for Arduino IDE and Visual Studio Code can be found in the [RAK Documentation Center](https://docs.rakwireless.com/RUI3/Supported-IDE/).

----

# Available Examples

## [RUI3-LowPower-Example](./RUI3-LowPower-Example)
The simpliest example is just showing how to design an application that is event based and keeps the MCU and LoRa transceiver in low power mode whenever possible.    
It includes as well a simple example how to implement custom AT commands.

## [RUI3-RAK1901-RAK1902-Example](./RUI3-RAK1901-RAK1902-Example)    
A simple sensor application based on RAK1901 and RAk1902. It reads temperature, humidity and barometric pressure from these sensors and sends them in a configurable interval.    
This example is using the Cayenne LPP data format to send the formatted payload.    

## [RUI3-RAK12500-RAK1904-GNSS](./RUI3-RAK12500-RAK1904-GNSS)    
A motion based location tracker using the RAK12500 and RAK1904 WisBlock modules. Based on the RUI3-LowPower-Example code.    
This example is using the Cayenne LPP data format to send the formatted payload.    

## [RUI3-RAK13011-Alarm](./RUI3-RAK13011-Alarm)
A door or window alarm system using the RAK13011 magnetic relay switch module. This examples features the usage of a queue to collect events. For the alarm system (or open/close counter), this was necessary, because events could be triggered faster than the uplink packets can be sent over LoRaWAN.

## [RUI3-RAK5811-Example](./RUI3-RAK5811-Example)
Example code for using the RAK5811 0-5V analog input module with RUI3. For lowest power consumption, the RAK5811 is powered down until a data reading is required. The interval time for reading the data can be setup with AT command.    

## [RUI3-Sensor-Node-Air-Quality](./RUI3-Sensor-Node-Air-Quality)
A sensor application that combines temperature, humidity, barometric pressure, CO2 and VOC sensing. It detect automatically which sensors are attached and adjusts its payload to the available sensor data. It supports WisBlock RAK1906, RAK12037 and RAK12047 sensor modules

## [RUI3-Signal-Meter-P2P-LPWAN](./RUI3-Signal-Meter-P2P-LPWAN)
This is a PoC of a very basic signal tester. It works in both LoRa P2P and LoRaWAN mode. It uses a simple OLED display and has no GUI. All settings are done over USB with RUI3 AT commands.    
It is a very simple (~30 US$ cheap) device that can help to check LoRa and LoRaWAN coverage. It does not claim to be a super precise instrument, it is just an affordable small instrument to check signal coverage.    

----

# Get RUI3 devices

Get a RAKwireless RUI3 WisDuo stamp module, breakout board or evaluation board from our [store](https://store.rakwireless.com/collections/new-menu-modules)

Get a RAKwireless RUI3 WisBlock Core module from our [store](https://store.rakwireless.com/collections/wisblock-core)

----
----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.