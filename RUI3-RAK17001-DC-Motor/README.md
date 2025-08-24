| <img src="../assets/rakstar.jpg" alt="RAKWireless"> | <img src="../assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="../assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Example how to control a DC motor with PWM using the RAK17001 H-Bridge module and RUI3

## ⚠️ IMPORTANT 
_**Due to a PWM bug in RUI3, this example does not work with RUI3 version V4.2.1. Use an older or newer version of RUI3 with this example.**_

This examples shows how to control a DC motor with PWM by using the RAK17001. The control of the motor is through Class C downlinks from the LoraWAN server or through LoRa P2P packets.     

On startup of the application the device is set into Class C (if in LoRaWAN mode) or RX is enabled permanently (if in LoRa P2P mode).     
Then the RAK17001 H-Bridge module is setup and the motor is set to speed 0.

To control the motor speed and direction a downlink packet is required. If using LoRaWAN, the fPort must be set to 1.      

----

# Motor control packet format

The downlink packet to the device must start with two marker bytes and a command byte 

| Packet byte # | Content                                                                                   |
| ------------- | ----------------------------------------------------------------------------------------- |
| 0x00          | Marker 0xAA                                                                               |
| 0x01          | Marker 0x55                                                                               |
| 0x02          | Command                                                                                   |
|               | 0x01 ==> Motor control                                                                    |
|               | 0x02 .. 0xFF ==> for future use                                                           |
| 0x03          | Motor speed, ! very low values might not work !                                           |
| 0x04          | Motor direction ! real direction depends on the connection of the motor to the RAK17001 ! |
|               | 0x00 = counter clockwise                                                                  |
|               | 0x01 = clockwise                                                                          |
| 0x05          | Motor active or stopped                                                                   |
|               | 0x00 = motor stopped                                                                      |
|               | 0x01 = motor running                                                                      |

### Downlink packets examples:

aa5501500101
==> Sets motor with speed 0x50, rotating clockwise, running

aa5501500100
==> Sets motor with speed 0x50, rotating clockwise, stopped

aa5501FF0001
==> Sets motor to max speed 0xFF, rotating counter-clockwise, running


## ⚠️ IMPORTANT 
The minimum possible motor speed depends on the motor and the used power supply. It is best to determine the lowest value by testing.     

# Motor status uplink packet format

## Events that trigger a status uplink:

1) After joining the LoRaWAN network in LoRaWAN mode, or immediately after startup in LoRa P2P mode, the device is sending an status uplink packet.     
For LoRaWAN mode, this is required for the LoRaWAN server to be able to send downlink packets.

2) After each motor control downlink packet, the status is reported back to the LoRaWAN server or the controlling LoRa P2P device.

3) In case the STSPIN250 chip detects an error, the motor is stopped and the status is reported back to the LoRaWAN server or the controlling LoRa P2P device.

## Uplink packet format

| Packet byte # | Content                         |
| ------------- | ------------------------------- |
| 0x00          | Marker 0xAA                     |
| 0x01          | Marker 0x55                     |
| 0x02          | Status type                     |
|               | 0x01 ==> Motor status           |
|               | 0x02 .. 0xFF ==> for future use |
| 0x03          | Current motor speed             |
| 0x04          | Current motor direction         |
|               | 0x00 = counter clockwise        |
|               | 0x01 = clockwise                |
| 0x05          | Motor active or stopped         |
|               | 0x00 = motor stopped            |
|               | 0x01 = motor running            |
| 0x06          | Error flag                      |
|               | 0x00 = no error                 |
|               | 0x01 = error detected           |

The error flag is set if the STSPIN250 detects

1) thermal shutdown

2) overcurrent protection triggered

3) short-circuit protection triggered

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