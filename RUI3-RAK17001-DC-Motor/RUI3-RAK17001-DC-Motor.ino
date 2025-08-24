/**
 * @file RUI3-RAK17001.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RAK17001 example for RUI3
 * @version 0.1
 * @date 2025-08-21
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <Arduino.h>
#define STSPIN250_PH WB_IO4			 // The PH pin determines the direction of the current
#define STSPIN250_PWM WB_IO3		 // The PWM pin can be used to regulate the speed of the rotation
#define STSPIN250_FAULT_CHECK WB_IO5 // Fault output of the STSPIN250
#define STSPIN250_EN WB_IO6			 // Enable/Disable pin
#define FAULT_CHECK_COUNT 4			 // if four protection interrupts detected within 5 seconds, Sensor may be thermal shutdown or Overcurrent and short-circuit protections
#define FAULT_CHECK_INTERVAL 5000	 // Check value in [ms] ,

/** Error counter */
volatile uint16_t error_counter = 0;
/** Time since last error */
volatile int old_time = 0;
/** Time tested */
volatile int new_time = 0;
/** Flag if error was detected */
bool error_flag = false;

/** Motor speed to apply between 0 and 255, but depending on motor the motor might not be spinning below 40 */
uint8_t motor_speed = 50;
/** Flag if motor should be stopped */
bool motor_stop = true;
/** Flag for rotation direction */
bool motor_turn_clockwise = true;
/** Flag if direction change is requested */
bool change_direction = false;
/** Payload buffer to send status */
uint8_t status[10];
/** Flag if motor control is required */
bool motor_change = false;
/** Flag to send initial status */
bool send_first_status = false;

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
void joinCallback(int32_t status)
{
	if (status != 0)
	{
		Serial.printf("LoRaWan OTAA - join fail! \r\n");
		// If it failed, try again
		api.lorawan.join();
	}
	else
	{
		Serial.printf("LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
		// Send a first packet, required in Class C to be able to receive downlinks
		send_first_status = true;
	}
}

/**
 * @brief Parse received payload
 *
 * Byte 0 ==> Marker 0xAA
 * Byte 1 ==> Marker 0x55
 * Byte 2 ==> Command 0x01 ==> set speed and direction
 *            Command 0xnn ==> for future use
 * Byte 3 ==> motor speed value 40 to 255 (0x28 to 0xFF)
 * Byte 4 ==> motor direction, 0 = counter clockwise, 1 = clockwise
 * Byte 5 ==> 0 = motor stop, 1 = motor start
 *
 */
void parse_command(uint8_t *data, uint8_t size)
{
	uint8_t rcvd_value = 0;
	bool rcvd_direction = true;

	Serial.printf("Size %d\n", size);
	// Check downlink for commands
	if ((data[0] == 0xAA) && (data[1] == 0x55))
	{
		// Got a command
		switch (data[2])
		{
		case 0x01:		   // Change motor speed
			if (size != 6) // Check command length
			{
				Serial.println("Motor Speed wrong size");
				Serial.println("+EVT:PARAM_ERROR");
			}
			// Get new value
			rcvd_value = data[3];
			rcvd_direction = data[4];
			// Check speed. 40 is difficult with some motors, needs calibration with specific motor.
			if (rcvd_value >= 40)
			{
				motor_speed = rcvd_value;
			}
			else
			{
				Serial.printf("Motor Speed too low %d\n", rcvd_value);
				Serial.println("+EVT:PARAM_ERROR");
				return;
			}
			Serial.printf("Motor Speed %d\n", motor_speed);

			if (rcvd_direction > 1)
			{
				Serial.println("Direction wrong value");
				Serial.println("+EVT:PARAM_ERROR");
				return;
			}
			if (rcvd_direction != motor_turn_clockwise)
			{
				change_direction = true;
			}
			motor_turn_clockwise = rcvd_direction;
			Serial.printf("Motor Direction %d\n", motor_turn_clockwise);

			// Get new value
			rcvd_value = data[5];
			if (rcvd_value > 1)
			{
				Serial.println("Start/Stop wrong value");
				Serial.println("+EVT:PARAM_ERROR");
				return;
			}
			else if (rcvd_value == 0)
			{
				motor_stop = true;
			}
			else
			{
				motor_stop = false;
			}
			Serial.printf("Motor Start/Stop %s\n", motor_stop ? "Stop" : "Run");
			// Set motor speed, direction and start/stop
			motor_change = true;
			break;
		default:
			Serial.println("+EVT:AT_ERROR");
			break;
		}
	}
}

/**
 * @brief LoRaWAN callback after packet was received
 *
 * @param data pointer to structure with the received data
 *
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	Serial.printf("RX, port %d, DR %d, RSSI %d, SNR %d\n", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");

	if (data->Port == 1)
	{
		parse_command(data->Buffer, data->BufferSize);
	}
}

/**
 * @brief Callback for LinkCheck result
 *
 * @param data pointer to structure with the linkcheck result
 */
void linkcheckCallback(SERVICE_LORA_LINKCHECK_T *data)
{
	Serial.printf("%s Margin %d GW# %d RSSI%d SNR %d\r\n", data->State == 0 ? "Success" : "Failed",
				  data->DemodMargin, data->NbGateways,
				  data->Rssi, data->Snr);
}

/**
 * @brief LoRaWAN callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	Serial.printf("TX status %d\r\n", status);
	digitalWrite(LED_BLUE, LOW);
}

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data pointer to the data with the received data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	Serial.printf("P2P RX, RSSI %d, SNR %d\r\n", data.Rssi, data.Snr);
	for (int i = 0; i < data.BufferSize; i++)
	{
		Serial.printf("%02X", data.Buffer[i]);
	}
	Serial.print("\r\n");

	parse_command(data.Buffer, data.BufferSize);
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	Serial.println("P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
}

/**
 * @brief LoRa P2P callback for CAD result
 *
 * @param result true if activity was detected, false if no activity was detected
 */
void cad_cb(bool result)
{
	Serial.printf("P2P CAD reports %s\r\n", result ? "activity" : "no activity");
}

/**
 * @brief Arduino setup routine, called once on power up or reset
 *
 */
void setup(void)
{
	// Setup for LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
		api.lorawan.registerLinkCheckCallback(linkcheckCallback);
		/** For motor control the device must be Class C */
		api.lorawan.deviceClass.set(RAK_LORA_CLASS_C);
	}
	else // Setup for LoRa P2P
	{
		api.lora.registerPRecvCallback(recv_cb);
		api.lora.registerPSendCallback(send_cb);
		api.lora.registerPSendCADCallback(cad_cb);
		/** Enable permanent RX */
		api.lora.precv(65533);
	}

	// Sensor power control
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	delay(300);

	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, LOW);
	Serial.begin(115200);
	time_t timeout = millis();
	while (!Serial)
	{
		if ((millis() - timeout) < 15000)
		{
			delay(100);
		}
		else
		{
			break;
		}
	}

	api.system.firmwareVersion.set("RUI3-Motor-Control-V1.0.0");

	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Version %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Create a timer to check for errors
	if (!api.system.timer.create(RAK_TIMER_0, motor_check_periodic_handler, RAK_TIMER_PERIODIC))
	{
		Serial.printf("timer 0 init failed\n");
	}

	analogWriteResolution(8);
	delay(100);

	// Setup STSPIN250
	Serial.println("Setup H-Bridge");
	pinMode(STSPIN250_PH, OUTPUT);
	digitalWrite(STSPIN250_PH, motor_turn_clockwise);

	pinMode(STSPIN250_PWM, OUTPUT);
	analogWrite(STSPIN250_PWM, 0);

	pinMode(STSPIN250_EN, OUTPUT);
	digitalWrite(STSPIN250_EN, LOW); // if set STSPIN250_EN High,sensor work normal.if set LOW, Sensors enter low power mode.

	pinMode(STSPIN250_FAULT_CHECK, INPUT);
	attachInterrupt(digitalPinToInterrupt(STSPIN250_FAULT_CHECK), motor_interrupt, FALLING);

	// Start fault check timer
	api.system.timer.start(RAK_TIMER_0, FAULT_CHECK_INTERVAL, NULL);

	// Join network
	api.lorawan.join();

	// Enable LPM while PWM not active
	api.system.lpm.set(1);
}

/**
 * @brief Arduino loop function
 *
 * Handle motor changes and errors
 *
 */
void loop(void)
{
	bool send_status_req = false;
	if (error_flag) // Thermal shutdown or Overcurrent and short-circuit protections
	{
		error_flag = false;
		error_counter = 0;
		digitalWrite(LED_BLUE, HIGH);
		Serial.println("Sensor thermal shutdown or Overcurrent and short-circuit protections, please check it ! ! !");
		digitalWrite(STSPIN250_EN, LOW);
		analogWrite(STSPIN250_PWM, 0); // Stop motor
		api.system.lpm.set(1);
		delay(500);

		// Prepare alarm
		status[0] = 0xAA;
		status[1] = 0x55;
		status[2] = 0x01;
		status[3] = 0x00;
		status[4] = motor_turn_clockwise;
		status[5] = motor_stop ? 0x00 : 0x01;
		status[6] = 0x01; // Set alarm

		Serial.println("Request status packet");
		send_status_req = true;
	}
	if (motor_change)
	{
		motor_change = false;

		if (motor_stop)
		{
			// Stop motor
			digitalWrite(STSPIN250_EN, LOW);
			analogWrite(STSPIN250_PWM, 0);
			Serial.println("motor_stop");
			// Enable LPM while PWM not active
			api.system.lpm.set(1);
		}
		else
		{
			// Stop motor before changing settings
			digitalWrite(STSPIN250_EN, LOW);
			analogWrite(STSPIN250_PWM, 0); // Stop motor
			if (motor_turn_clockwise)
			{
				Serial.println("Clockwise rotation");
				digitalWrite(STSPIN250_PH, HIGH); // The motor rotates clockwise
			}
			else
			{
				Serial.println("Counter clockwise rotation");
				digitalWrite(STSPIN250_PH, LOW); // The motor rotates counter-clockwise
			}

			Serial.printf("analogWrite, speed = %d\n", motor_speed);

			// Disable LPM to keep PWM active
			api.system.lpm.set(0);

			analogWrite(STSPIN250_PWM, motor_speed); // Set speed
			digitalWrite(STSPIN250_EN, HIGH);

			Serial.println("motor_setup");
		}
		// Prepare status
		status[0] = 0xAA;
		status[1] = 0x55;
		status[2] = 0x01;
		status[3] = motor_speed;
		status[4] = motor_turn_clockwise;
		status[5] = motor_stop ? 0x00 : 0x01;
		status[6] = 0x00; // Reset alarm

		// Report current status
		Serial.println("Request status packet");
		send_status_req = true;
	}
	if (send_first_status)
	{
		send_first_status = false;
		// Send status
		status[0] = 0xAA;
		status[1] = 0x55;
		status[2] = 0x01;
		status[3] = motor_speed;
		status[4] = motor_turn_clockwise;
		status[5] = motor_stop ? 0x00 : 0x01;
		status[6] = 0x00; // Reset alarm
		Serial.println("Request status packet");
		send_status_req = true;
	}

	// Need to send status?
	if (send_status_req)
	{
		send_status_req = false;
		send_status();
	}
}

void send_status(void)
{
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		// Send the packet
		if (api.lorawan.send(7, status, 10))
		{
			Serial.println("Packet enqueued, size 7");
		}
		else
		{
			Serial.println("Send failed");
		}
	}
	// It is P2P
	else
	{
		if (api.lora.psend(7, status))
		{
			Serial.println("P2P Packet enqueued");
		}
		else
		{
			Serial.println("P2P Send failed");
		}
	}
}

/**
 * @brief Handle interrupts from the motor driver chip
 *
 */
void motor_interrupt()
{
	old_time = new_time;
	new_time = millis();
	if (new_time - old_time >= 100)
		error_counter++;
}

/**
 * @brief Function for handling motor error events.
 */
void motor_check_periodic_handler(void *)
{
	if (error_counter >= FAULT_CHECK_COUNT)
	{
		Serial.print("error_counter=");
		Serial.println(error_counter);
		error_flag = true;
	}
	error_counter = 0;
}
