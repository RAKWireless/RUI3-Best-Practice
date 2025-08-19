/**
 * @file WisBlock-Sensor-Node.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for easy testing of WisBlock I2C modules
 * @version 0.1
 * @date 2022-04-10
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "inc/app.h"

// Flag for ADR on or off
bool adr_enabled = false;

// Data rate 0 .. 15 (validity depnends on Region)
uint8_t data_rate = 3;

// Flag to enable confirmed messages
bool confirmed_msg_enabled = false;

/** Flag if join has failed */
bool join_failed = false;

/** Flag for OTAA joining */
bool is_joining = false;

/** Flag if TX is active */
bool tx_active = false;

/** Flag if TX has finished (for display) */
bool tx_finished = false;

/** Flag if TX has started */
bool tx_started = false;

/** Flag if RX received */
bool has_rx = false;

/** Buffer for first 5 bytes of RX payload */
uint8_t last_rx_buff[6];

/* Device Address from OTAA, used when switching to ABP */
uint8_t dev_addr[4];

/* NwSkey from OTAA, used when switching to ABP */
uint8_t nwskey[16];

/* AppSkey from OTAA, used when switching to ABP */
uint8_t appskey[16];

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/**
 * @brief Callback after packet was received
 *
 * @param data Structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d", data->Port, data->RxDatarate, data->Rssi, data->Snr);
	tx_active = false;
	if (data->BufferSize != 0)
	{
		has_rx = true;
		memset(last_rx_buff, 0, 6);
		memcpy(last_rx_buff, data->Buffer, 5);
	}
}

/**
 * @brief Callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX status %d", status);
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
	event = 5;
	api.system.timer.start(RAK_TIMER_0, 100, &event);
}

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
void joinCallback(int32_t status)
{
	if (status != 0)
	{
		event = 6;
		api.system.timer.start(RAK_TIMER_0, 100, &event);
	}
	else
	{
		digitalWrite(LED_BLUE, LOW);
		event = 4;
		api.system.timer.start(RAK_TIMER_0, 100, &event);
	}
	tx_active = false;
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	if (api.lora.nwm.get() != 1)
	{
		api.lorawan.nwm.set();
	}

	// Disconnect from network, just in case
	// api.lorawan.join(0, 0, 6, 0);

	// Setup the callbacks for joined and send finished
	api.lorawan.registerRecvCallback(receiveCallback);
	api.lorawan.registerSendCallback(sendCallback);
	api.lorawan.registerJoinCallback(joinCallback);

	api.lorawan.njm.set(1);

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	Serial.begin(115200);

	time_t serial_timeout = millis();
#ifdef _VARIANT_RAK4630_
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
#else
	// For RAK3172 & RAK11720 just wait a little bit for the USB to be ready
	while (1)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
#endif

	// Initialize EPD
	init_rak14000();

	// // Add custom status AT command
	init_status_at();

	MYLOG("SETUP", "RAK RUI3 EPD Demo");
	MYLOG("SETUP", "Setup the device with AT commands first");

	MYLOG("SETUP", "NWM = %s", api.lorawan.njm.get() ? "OTAA" : "ABP");

	// Get saved sending frequency from flash
	get_at_setting();

	digitalWrite(LED_GREEN, LOW);

	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);
	}

	api.system.lpm.set(1);

	// Create a timer to handle events
	api.system.timer.create(RAK_TIMER_0, event_handler, RAK_TIMER_ONESHOT);

	// Initialize button interrupts
	pinMode(RIGHT_BUTTON, INPUT_PULLUP);
	pinMode(MIDDLE_BUTTON, INPUT_PULLUP);
	pinMode(LEFT_BUTTON, INPUT_PULLUP);

	attachInterrupt(LEFT_BUTTON, handle_OTAA, FALLING);
	attachInterrupt(RIGHT_BUTTON, handle_ABP, FALLING);
	attachInterrupt(MIDDLE_BUTTON, handle_SEND, FALLING);
}

/**
 * @brief event_handler is a timer callback to handle events outside of
 * lorawan callbacks and interrupts
 */
void event_handler(void *event)
{
	uint8_t *this_event = (uint8_t *)event;
	MYLOG("EVT", "Event %d", this_event[0]);
	switch (this_event[0])
	{
	case 1: // set OTAA
		api.lorawan.njm.set(1);

		if (api.lorawan.njs.get() != 0)
		{
			api.lorawan.join(0, 0, 7, 0);
		}

		MYLOG("EVT", "Start join OTAA");
		is_joining = true;
		api.lorawan.join();
		refresh_rak14000();
		attachInterrupt(LEFT_BUTTON, handle_OTAA, FALLING);
		break;
	case 2: // set ABP
		api.lorawan.daddr.set(dev_addr, 4);
		api.lorawan.appskey.set(appskey, 16);
		api.lorawan.nwkskey.set(nwskey, 16);
		api.lorawan.daddr.get(dev_addr, 4);
		api.lorawan.appskey.get(appskey, 16);
		api.lorawan.nwkskey.get(nwskey, 16);
		MYLOG("EVT", "DevAddr = %02x%02x%02x%02x", dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3]);
		MYLOG("EVT", "AppSKey = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			  appskey[0], appskey[1], appskey[2], appskey[3],
			  appskey[4], appskey[5], appskey[6], appskey[7],
			  appskey[8], appskey[9], appskey[10], appskey[11],
			  appskey[12], appskey[13], appskey[14], appskey[15]);
		MYLOG("EVT", "NwSKey = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			  nwskey[0], nwskey[1], nwskey[2], nwskey[3],
			  nwskey[4], nwskey[5], nwskey[6], nwskey[7],
			  nwskey[8], nwskey[9], nwskey[10], nwskey[11],
			  nwskey[12], nwskey[13], nwskey[14], nwskey[15]);

		api.lorawan.njm.set(0);

		if (api.lorawan.njs.get() != 0)
		{
			api.lorawan.join(0, 0, 7, 0);
		}

		MYLOG("EVT", "Start join ABP");
		api.lorawan.join();

		refresh_rak14000();

		attachInterrupt(RIGHT_BUTTON, handle_ABP, FALLING);
		break;
	case 3: // send packet
		if (tx_active)
		{
			// Transmission still ongoing
			MYLOG("EVT", "TX still active, skip sending");
			attachInterrupt(MIDDLE_BUTTON, handle_SEND, FALLING);
			return;
		}

		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			// MYLOG("UPLINK", "Not joined, skip sending");
			digitalWrite(LED_BLUE, LOW);
			attachInterrupt(MIDDLE_BUTTON, handle_SEND, FALLING);
			return;
		}

		digitalWrite(LED_BLUE, HIGH);
		tx_started = true;
		refresh_rak14000();

		// Clear payload
		g_solution_data.reset();

		// Create payload
		// Add battery voltage
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

		send_packet();
		attachInterrupt(MIDDLE_BUTTON, handle_SEND, FALLING);
		break;
	case 4: // Joined event
		refresh_rak14000();
		api.lorawan.daddr.get(dev_addr, 4);
		api.lorawan.appskey.get(appskey, 16);
		api.lorawan.nwkskey.get(nwskey, 16);
		MYLOG("EVT", "DevAddr = %02x%02x%02x%02x", dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3]);
		MYLOG("EVT", "AppSKey = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			  appskey[0], appskey[1], appskey[2], appskey[3],
			  appskey[4], appskey[5], appskey[6], appskey[7],
			  appskey[8], appskey[9], appskey[10], appskey[11],
			  appskey[12], appskey[13], appskey[14], appskey[15]);
		MYLOG("EVT", "NwSKey = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			  nwskey[0], nwskey[1], nwskey[2], nwskey[3],
			  nwskey[4], nwskey[5], nwskey[6], nwskey[7],
			  nwskey[8], nwskey[9], nwskey[10], nwskey[11],
			  nwskey[12], nwskey[13], nwskey[14], nwskey[15]);
		break;
	case 5: // TX finished
		tx_finished = true;
		refresh_rak14000();
		break;
	case 6: // Join failed
		join_failed = true;
		refresh_rak14000();
		break;
	default:
		MYLOG("EVT", "Invalid event");
		break;
	}
}

bool first_run = true;
/**
 * @brief This example is complete timer
 * driven. The loop() does nothing than
 * sleep.
 *
 */
void loop()
{
	// Fix RUI3 problem with interrupt attached in setup()
	if (first_run)
	{
		attachInterrupt(RIGHT_BUTTON, handle_ABP, FALLING);
		first_run = false;
	}
	// api.system.sleep.all();
	api.system.scheduler.task.destroy();
}

/**
 * @brief Send the data packet that was prepared in
 * Cayenne LPP format by the different sensor and location
 * aqcuision functions
 *
 */
void send_packet(void)
{
	if (api.lora.nwm.get() == 1)
	{
		// MYLOG("UPLINK", "Send packet with size %d", g_solution_data.getSize());

		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), 2))
		{
			MYLOG("UPLINK", "Send %d", g_solution_data.getSize());
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
			digitalWrite(LED_BLUE, LOW);
		}
	}
	else
	{
		// MYLOG("UPLINK", "Send packet with size %d over P2P", g_solution_data.getSize() + 8);

		digitalWrite(LED_BLUE, LOW);

		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer()))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
