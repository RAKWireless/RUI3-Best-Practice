| <img src="./assets/RAK-Whirls.png" alt="RAKWireless"> | <img src="./assets/RUI3.jpg" alt="RUI3" width=30%> | <img src="./assets/rakstar.jpg" alt="RAKstar" > |    
| :-: | :-: | :-: |     

# Indoor air quality sensor with Bosch BME680, RAK3172 and IAQ algorithm
We tend to think of air pollution as a risk faced outside, but the air we breathe indoors can also be polluted.    
Environmental health researchers study how indoor air quality affects human health and well-being.     

This little PoC is building a low power IAQ sensor with the Bosch BME680 sensor. It does not use the Bosch BSEC algorithm to calculate the IAQ value. Instead it is using an open source code provided by David Bird. Please check the [license provided with the code](#license-for-the-bme680-iaq-algorithm) and please honor the request of the author to keep the license with the source code.

----

# Hardware

| WisBlock module | Function               | Documentation                                                                           |
| --------------- | ---------------------- | --------------------------------------------------------------------------------------- |
| RAK19003        | Mini Base Board        | [RAK19003 datasheet](https://docs.rakwireless.com/product-categories/wisblock/rak19003) |
| RAK3372         | MCU & LoRa transceiver | [RAK3372 datasheet](https://docs.rakwireless.com/product-categories/wisblock/rak3372)   |
| RAK1906         | IAQ sensor             | [RAK1906 datasheet](https://docs.rakwireless.com/product-categories/wisblock/rak1906)   |

----

# Software

| WisBlock module   | Documentation                                                                                            |
| ----------------- | -------------------------------------------------------------------------------------------------------- |
| RUI3 BSP          | [RUI3 API](https://docs.rakwireless.com/product-categories/software-apis-and-libraries/rui3/arduino-api) |
| IAQ algorithm     | [IAQ algorithm](https://github.com/G6EJD/BME680-Example)                                                 |

----

# Function
The code is based on my low power RUI3 example. It is complete based on timers. One timer handles the readings from the Bosch BME680 sensor in a configurable interval.    
The second timer handles sending the sensor data in a configurable interval.    

For the IAQ, here the quote from the original code provider David Bird:    

> The sensor is used to obtain the so-called Gas Resistance and then calculate an Index of Air Quality (IAQ) from a combination of humidity and the gas content readings of the air, optionally temperature could be added too, but omittied in this example.     
> The index is a function of humidity which contributes up to 25% and gas concentrations that contributes up to 75%. See [slide1.jpg](https://github.com/G6EJD/BME680-Example/blob/master/Slide1.JPG) for details of the IAQ index formulation.

Comparison between the Bosch BME680 BSEC algorithm and David's algorithm showed very similar air quality indices.    

For me David's algorithm has the advantage that his code is much smaller than the BSEC library. In addition the measurement results are immediately available, while with the BSEC library it can take hours (or days) before the algorithm gives any results.    

----

# Setup of the device
The setup of the device to work as a LoRaWAN or LoRa P2P endnode is complete done with [RUI3 AT commands](https://docs.rakwireless.com/product-categories/software-apis-and-libraries/rui3/at-command-manual). The setup has to be done only once, the settings are stored in the flash memory of the RAK3372.

For sensor specific settings and device status requests a few custom AT commands have been added:

----

## Sensor read interval
As uptime of the sensor is influencing the battery life, the sensor readings for the IAQ calculation can be set with _**ATC+IAQINT**_

| Command        | Input Parameter                 | Return Value                                                                    | Return Code      |
| -------------- | ------------------------------- | ------------------------------------------------------------------------------- | ---------------- |
| ATC+IAQINT     | -                               | ATC+IAQINT: Set/Get the IAQ reading time seconds 0 = off, max 2,147,483 seconds | OK               |
| ATC+IAQINT=?   | -                               | current read interval in seconds                                                | OK               |
| ATC+IAQINT=xxx | Sensor read interval in seconds | xxx                                                                             | OK or ERROR code |

----

## Sensor data transmission interval
The second function that is influencing the battery life, is the time between sensor data transmission over LoRa or LoRaWAN.    
The send interval can be set with _**ATC+SENDINT**_    

| Command         | Input Parameter                 | Return Value                                                                         | Return Code      |
| --------------- | ------------------------------- | ------------------------------------------------------------------------------------ | ---------------- |
| ATC+SENDINT     | -                               | ATC+IAQINT: Set/Get the interval sending time seconds 0 = off, max 2,147,483 seconds | OK               |
| ATC+SENDINT=?   | -                               | current send interval in seconds                                                     | OK               |
| ATC+SENDINT=xxx | Transmision interval in seconds | xxx                                                                                  | OK or ERROR code |

----

## Device status report
The device status report will return the current LoRa or LoRaWAN settings of the device.    

| Command        | Input Parameter | Return Value                             | Return Code      |
| -------------- | --------------- | ---------------------------------------- | ---------------- |
| ATC+STATUS     | -               | ATC+STATUS: Get device information       | OK               |
| ATC+STATUS=?   | -               | returns device status, see example below | OK               |

```log
Device Status:
Test Mode: LoRa P2P
Module: RAK3172
Version: RUI3-Clock-V1.0.0
Send time: 0 s
Network mode P2P
Frequency = 915375000
SF = 7
BW = 1
CR = 0
Preamble length = 16
TX power = 7
OK
```

----

# License for the BME680 IAQ algorithm 

```
/*******************************************************************************************************************/
/** License for the BME680 IAQ algorithm                                                                           */
/** Origin from https://github.com/G6EJD/BME680-Example                                                            */
/*******************************************************************************************************************/
/*
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.

 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.

 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
*/
```

----

# Get RUI3 devices

Get a RAKwireless RUI3 WisDuo stamp module, breakout board or evaluation board from our [store](https://store.rakwireless.com/collections/new-menu-modules)

Get a RAKwireless RUI3 WisBlock Core module from our [store](https://store.rakwireless.com/collections/wisblock-core)

----

# LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. 

----

# LoRaWAN® is a licensed mark.