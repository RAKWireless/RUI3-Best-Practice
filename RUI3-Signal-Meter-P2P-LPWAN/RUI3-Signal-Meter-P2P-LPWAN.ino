/**
 * @file RUI3-Signal-Meter-P2P-LPWAN.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Simple signal meter for LoRa P2P and LoRaWAN
 * @version 0.1
 * @date 2023-11-23
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

/** Last RX SNR level*/
volatile int8_t last_snr = 0;
/** Last RX RSSI level*/
volatile int16_t last_rssi = 0;
/** Link check result */
volatile uint8_t link_check_state;
/** Demodulation margin */
volatile uint8_t link_check_demod_margin;
/** Number of gateways */
volatile uint8_t link_check_gateways;
/** Sent packet counter */
volatile int32_t packet_num = 0;
/** Lost packet counter (only LPW mode)*/
volatile int32_t packet_lost = 0;
/** Last RX data rate */
volatile uint8_t last_dr = 0;
/** TX fail reason (only LPW mode)*/
volatile int32_t tx_fail_status;

/** LoRa mode */
bool lorawan_mode = true;
/** Flag if confirmed packets or LinkCheck should be used */
bool use_link_check = true;

/** Flag if OLED was found */
bool has_oled = false;
/** Buffer for OLED output */
char line_str[256];
/** Flag for display handler */
uint8_t display_reason;

/**
 * @brief Send a LoRaWAN packet
 *
 * @param data unused
 */
void send_packet(void *data)
{
	if (api.lorawan.njs.get())
	{
		MYLOG("APP", "Send packet");
		uint8_t payload[4] = {0x01, 0x02, 0x03, 0x04};
		
		// Always send confirmed packet to make sure a reply is received
		api.lorawan.send(4, payload, 2, true, 8);

		// if (use_link_check)
		// {
		// 	// Linkcheck is enabled, send an unconfirmed packet
		// 	api.lorawan.send(4, payload, 2, false);
		// }
		// else
		// {
		// 	// Linkcheck is disabled, send a confirmed packet
		// 	api.lorawan.send(4, payload, 2, true, 8);
		// }
	}
	else
	{
		MYLOG("APP", "Not joined, don't send packet");
	}
}

/**
 * @brief Display handler
 *
 * @param reason 1 = RX packet display
 *               2 = TX failed display (only LPW mode)
 *               3 = Join failed (only LPW mode)
 *               4 = Linkcheck result display (only LPW LinkCheck mode)
 *               5 = Join success (only LPW mode)
 */
void handle_display(void *reason)
{
	/** Update header and battery value */
	if (has_oled)
	{
		rak1921_clear();
		sprintf(line_str, "RAK Signal Meter - B %.2fV", api.system.bat.get());
		rak1921_write_header(line_str);
	}
	// Get the wakeup reason
	uint8_t *disp_reason = (uint8_t *)reason;

	// Check if we have a reason
	if (disp_reason == NULL)
	{
		Serial.println("Bug in code!");
	}
	else if (disp_reason[0] == 1)
	{
		// MYLOG("APP", "RX_EVENT %d\n", disp_reason[0]);
		// RX event display
		if (g_custom_parameters.test_mode == 1)
		{
			if (has_oled)
			{
				sprintf(line_str, "LPW CFM mode");
				rak1921_write_line(0, 0, line_str);
				sprintf(line_str, "Sent %d", packet_num);
				rak1921_write_line(1, 0, line_str);
				sprintf(line_str, "Lost %d", packet_lost);
				rak1921_write_line(1, 64, line_str);
				sprintf(line_str, "DR   %d", last_dr);
				rak1921_write_line(2, 0, line_str);
				sprintf(line_str, "RSSI %d", last_rssi);
				rak1921_write_line(3, 0, line_str);
				sprintf(line_str, "SNR  %d", last_snr);
				rak1921_write_line(3, 64, line_str);
				sprintf(line_str, "----------");
				rak1921_write_line(4, 0, line_str);
				rak1921_display();
			}
			Serial.println("LPW CFM mode");
			Serial.printf("Packet # %d RSSI %d SNR %d\n", packet_num, last_rssi, last_snr);
			Serial.printf("DR %d\n", last_dr);
		}
		else
		{
			if (has_oled)
			{
				sprintf(line_str, "LoRa P2P mode");
				rak1921_write_line(0, 0, line_str);
				sprintf(line_str, "Received packets %d", packet_num);
				rak1921_write_line(1, 0, line_str);
				sprintf(line_str, "F %.3f", (api.lora.pfreq.get() / 1000000.0));
				rak1921_write_line(2, 0, line_str);
				sprintf(line_str, "SF %d", api.lora.psf.get());
				rak1921_write_line(3, 0, line_str);
				sprintf(line_str, "BW %d", (api.lora.pbw.get() + 1) * 125);
				rak1921_write_line(3, 64, line_str);
				sprintf(line_str, "RSSI %d", last_rssi);
				rak1921_write_line(4, 0, line_str);
				sprintf(line_str, "SNR %d", last_snr);
				rak1921_write_line(4, 64, line_str);
				rak1921_display();
			}
			Serial.println("LPW P2P mode");
			Serial.printf("Packet # %d RSSI %d SNR %d\n", packet_num, last_rssi, last_snr);
			Serial.printf("F %.3f SF %d BW %d\n",
						  (float)api.lora.pfreq.get() / 1000000.0,
						  api.lora.psf.get(),
						  (api.lora.pbw.get() + 1) * 125);
		}
	}
	else if (disp_reason[0] == 2)
	{
		// MYLOG("APP", "TX_ERROR %d\n", disp_reason[0]);

		digitalWrite(LED_BLUE, HIGH);
		if (has_oled)
		{
			sprintf(line_str, "LPW CFM mode");
			rak1921_write_line(0, 0, line_str);
			sprintf(line_str, "Sent %d", packet_num);
			rak1921_write_line(1, 0, line_str);
			sprintf(line_str, "Lost %d", packet_num, packet_lost);
			rak1921_write_line(1, 64, line_str);
			sprintf(line_str, "TX failed with status %d", tx_fail_status);
			rak1921_write_line(2, 0, line_str);
		}
		Serial.println("LPW CFM mode");
		Serial.printf("Packet %d\n", packet_num);
		Serial.printf("TX failed with status %d\n", tx_fail_status);

		switch (tx_fail_status)
		{
		case RAK_LORAMAC_STATUS_ERROR:
			sprintf(line_str, "Service error");
			break;
		case RAK_LORAMAC_STATUS_TX_TIMEOUT:
			sprintf(line_str, "TX timeout");
			break;
		case RAK_LORAMAC_STATUS_RX1_TIMEOUT:
			sprintf(line_str, "RX1 timeout");
			break;
		case RAK_LORAMAC_STATUS_RX2_TIMEOUT:
			sprintf(line_str, "RX2 timeout");
			break;
		case RAK_LORAMAC_STATUS_RX1_ERROR:
			sprintf(line_str, "RX1 error");
			break;
		case RAK_LORAMAC_STATUS_RX2_ERROR:
			sprintf(line_str, "RX2 error");
			break;
		case RAK_LORAMAC_STATUS_JOIN_FAIL:
			sprintf(line_str, "Join failed");
			break;
		case RAK_LORAMAC_STATUS_DOWNLINK_REPEATED:
			sprintf(line_str, "Dowlink frame error");
			break;
		case RAK_LORAMAC_STATUS_TX_DR_PAYLOAD_SIZE_ERROR:
			sprintf(line_str, "Payload size error");
			break;
		case RAK_LORAMAC_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS:
			sprintf(line_str, "Fcnt loss error");
			break;
		case RAK_LORAMAC_STATUS_ADDRESS_FAIL:
			sprintf(line_str, "Adress error");
			break;
		case RAK_LORAMAC_STATUS_MIC_FAIL:
			sprintf(line_str, "MIC error");
			break;
		case RAK_LORAMAC_STATUS_MULTICAST_FAIL:
			sprintf(line_str, "Multicast error");
			break;
		case RAK_LORAMAC_STATUS_BEACON_LOCKED:
			sprintf(line_str, "Beacon locked");
			break;
		case RAK_LORAMAC_STATUS_BEACON_LOST:
			sprintf(line_str, "Beacon lost");
			break;
		case RAK_LORAMAC_STATUS_BEACON_NOT_FOUND:
			sprintf(line_str, "Beacon not found");
			break;
		default:
			sprintf(line_str, "Unknown error");
			break;
		}
		Serial.printf("%s\n", line_str);
		Serial.printf("Lost %d packets\n", packet_lost);
		if (has_oled)
		{
			rak1921_write_line(3, 0, line_str);
			sprintf(line_str, "----------");
			rak1921_write_line(4, 0, line_str);
			rak1921_display();
		}
	}
	else if (disp_reason[0] == 3)
	{
		// MYLOG("APP", "JOIN_ERROR %d\n", disp_reason[0]);
		if (has_oled)
		{
			sprintf(line_str, "LPW mode");
			rak1921_write_line(0, 0, line_str);
			rak1921_write_line(1, 0, (char *)"!!!!!!!!!!!!!!!!!!!");
			rak1921_write_line(2, 0, (char *)"!!!!!!!!!!!!!!!!!!!");
			sprintf(line_str, "Join failed");
			rak1921_write_line(3, 0, line_str);
			rak1921_write_line(4, 0, (char *)"!!!!!!!!!!!!!!!!!!!");
			rak1921_display();
		}
	}
	else if (disp_reason[0] == 5)
	{
		// MYLOG("APP", "JOIN_SUCCESS %d\n", disp_reason[0]);
		if (has_oled)
		{
			sprintf(line_str, "LPW mode");
			rak1921_write_line(0, 0, line_str);
			rak1921_write_line(1, 0, (char *)"!!!!!!!!!!!!!!!!!!!");
			rak1921_write_line(2, 0, (char *)"!!!!!!!!!!!!!!!!!!!");
			sprintf(line_str, "Device joined network");
			rak1921_write_line(3, 0, line_str);
			rak1921_write_line(4, 0, (char *)"!!!!!!!!!!!!!!!!!!!");
			rak1921_display();
		}
	}
	else if (disp_reason[0] == 4)
	{
		// MYLOG("APP", "LINK_CHECK %d\n", disp_reason[0]);
		// LinkCheck result event display
		if (has_oled)
		{
			sprintf(line_str, "LPW LinkCheck %s", link_check_state == 0 ? "OK" : "NOK");
			rak1921_write_line(0, 0, line_str);

			if (link_check_state == 0)
			{
				sprintf(line_str, "Demod Margin    %d", link_check_demod_margin);
				rak1921_write_line(1, 0, line_str);
				sprintf(line_str, "Sent %d", packet_num);
				rak1921_write_line(2, 0, line_str);
				sprintf(line_str, "Lost %d", packet_lost);
				rak1921_write_line(2, 64, line_str);
				sprintf(line_str, "%d GW(s)", link_check_gateways);
				rak1921_write_line(3, 0, line_str);
				sprintf(line_str, "DR %d", api.lorawan.dr.get());
				rak1921_write_line(3, 64, line_str);
				sprintf(line_str, "RSSI %d", last_rssi);
				rak1921_write_line(4, 0, line_str);
				sprintf(line_str, "SNR %d", last_snr);
				rak1921_write_line(4, 64, line_str);
			}
			else
			{
				sprintf(line_str, "Sent %d", packet_num);
				rak1921_write_line(1, 0, line_str);
				sprintf(line_str, "Lost %d", packet_lost);
				rak1921_write_line(1, 64, line_str);
				sprintf(line_str, "LinkCheck result %d ", link_check_state);
				rak1921_write_line(2, 0, line_str);
				switch (link_check_state)
				{
				case RAK_LORAMAC_STATUS_ERROR:
					sprintf(line_str, "Service error");
					break;
				case RAK_LORAMAC_STATUS_TX_TIMEOUT:
					sprintf(line_str, "TX timeout");
					break;
				case RAK_LORAMAC_STATUS_RX1_TIMEOUT:
					sprintf(line_str, "RX1 timeout");
					break;
				case RAK_LORAMAC_STATUS_RX2_TIMEOUT:
					sprintf(line_str, "RX2 timeout");
					break;
				case RAK_LORAMAC_STATUS_RX1_ERROR:
					sprintf(line_str, "RX1 error");
					break;
				case RAK_LORAMAC_STATUS_RX2_ERROR:
					sprintf(line_str, "RX2 error");
					break;
				case RAK_LORAMAC_STATUS_JOIN_FAIL:
					sprintf(line_str, "Join failed");
					break;
				case RAK_LORAMAC_STATUS_DOWNLINK_REPEATED:
					sprintf(line_str, "Dowlink frame error");
					break;
				case RAK_LORAMAC_STATUS_TX_DR_PAYLOAD_SIZE_ERROR:
					sprintf(line_str, "Payload size error");
					break;
				case RAK_LORAMAC_STATUS_DOWNLINK_TOO_MANY_FRAMES_LOSS:
					sprintf(line_str, "Fcnt loss error");
					break;
				case RAK_LORAMAC_STATUS_ADDRESS_FAIL:
					sprintf(line_str, "Adress error");
					break;
				case RAK_LORAMAC_STATUS_MIC_FAIL:
					sprintf(line_str, "MIC error");
					break;
				case RAK_LORAMAC_STATUS_MULTICAST_FAIL:
					sprintf(line_str, "Multicast error");
					break;
				case RAK_LORAMAC_STATUS_BEACON_LOCKED:
					sprintf(line_str, "Beacon locked");
					break;
				case RAK_LORAMAC_STATUS_BEACON_LOST:
					sprintf(line_str, "Beacon lost");
					break;
				case RAK_LORAMAC_STATUS_BEACON_NOT_FOUND:
					sprintf(line_str, "Beacon not found");
					break;
				default:
					sprintf(line_str, "Unknown error");
					break;
				}
				rak1921_write_line(3, 0, line_str);
			}
			rak1921_display();
		}
		Serial.printf("LinkCheck %s\n", link_check_state == 0 ? "OK" : "NOK");
		Serial.printf("Packet # %d RSSI %d SNR %d\n", packet_num, last_rssi, last_snr);
		Serial.printf("GW # %d Demod Margin %d\n", link_check_gateways, link_check_demod_margin);
	}

	digitalWrite(LED_GREEN, LOW);
}

/**
 * @brief Join network callback
 *
 * @param status status of join request
 */
void join_cb_lpw(int32_t status)
{
	if (status != 0)
	{
		display_reason = 3;
		api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
	}
	else
	{
		display_reason = 5;
		api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
	}
}

/**
 * @brief Receive callback for LoRa P2P mode
 *
 * @param data structure with RX packet information
 */
void recv_cb_p2p(rui_lora_p2p_recv_t data)
{
	digitalWrite(LED_GREEN, HIGH);
	last_rssi = data.Rssi;
	last_snr = data.Snr;
	packet_num++;

	display_reason = 1;
	api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
}

/**
 * @brief Receive callback for LoRaWAN mode
 *
 * @param data structure with RX packet information
 */
void recv_cb_lpw(SERVICE_LORA_RECEIVE_T *data)
{
	digitalWrite(LED_GREEN, HIGH);
	last_rssi = data->Rssi;
	last_snr = data->Snr;
	last_dr = data->RxDatarate;

	packet_num++;

	if (!use_link_check)
	{
		display_reason = 1;
		api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
	}
}

/**
 * @brief Send finished callback for LoRaWAN mode
 *
 * @param status
 */
void send_cb_lpw(int32_t status)
{
	if (status != RAK_LORAMAC_STATUS_OK)
	{
		MYLOG("APP", "LMC status %d\n", RAK_LORAMAC_STATUS_OK);
		tx_fail_status = status;

		if (!use_link_check)
		{
			packet_lost++;
			display_reason = 2;
			api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
		}
	}
}

/**
 * @brief Linkcheck callback
 *
 * @param data structure with the result of the Linkcheck
 */
void linkcheck_cb_lpw(SERVICE_LORA_LINKCHECK_T *data)
{
	// MYLOG("APP", "linkcheck_cb_lpw\n");
	last_snr = data->Snr;
	last_rssi = data->Rssi;
	link_check_state = data->State;
	link_check_demod_margin = data->DemodMargin;
	link_check_gateways = data->NbGateways;
	if (data->State != 0)
	{
		packet_lost++;
	}
	display_reason = 4;
	api.system.timer.start(RAK_TIMER_1, 250, &display_reason);
}

/**
 * @brief Setup routine
 *
 */
void setup(void)
{
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);
	Serial.begin(115200);
	sprintf(line_str, "RUI3_Tester_V%d.%d.%d", SW_VERSION_0, SW_VERSION_1, SW_VERSION_2);
	api.system.firmwareVersion.set(line_str);

	// Check if OLED is available
	Wire.begin();
	has_oled = init_rak1921();
	if (has_oled)
	{
		sprintf(line_str, "RAK Signal Meter - B %.2fV", api.system.bat.get());
		rak1921_write_header(line_str);
	}

	digitalWrite(LED_GREEN, HIGH);
#ifndef RAK3172
	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial.available())
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
	digitalWrite(LED_GREEN, HIGH);
	delay(5000);
#endif

	digitalWrite(LED_GREEN, LOW);
	digitalWrite(LED_BLUE, LOW);

	if (!has_oled)
	{
		MYLOG("APP", "No OLED found");
	}

	// Initialize custom AT commands
	if (!init_status_at())
	{
		MYLOG("APP", "Failed to initialize Status AT command");
	}
	if (!init_interval_at())
	{
		MYLOG("APP", "Failed to initialize Send Interval AT command");
	}
	if (!init_test_mode_at())
	{
		MYLOG("APP", "Failed to initialize Test Mode AT command");
	}

	// Get saved custom settings
	if (!get_at_setting())
	{
		MYLOG("APP", "Failed to read saved custom settings");
	}

	// Setup callbacks and timers depending on test mode
	switch (g_custom_parameters.test_mode)
	{
	default:
		Serial.println("Invalid test mode, use LinkCheck");
		if (has_oled)
		{
			sprintf(line_str, "Invalid test mode");
			rak1921_write_line(0, 0, line_str);
			sprintf(line_str, "Using LinkCheck");
			rak1921_write_line(1, 0, line_str);
			rak1921_display();
		}
	case MODE_LINKCHECK:
	set_linkcheck();
		break;
	case MODE_CFM:
	set_cfm();
		break;
	case MODE_P2P:
	set_p2p();
		break;
	}

	if (lorawan_mode)
	{
		api.system.timer.create(RAK_TIMER_0, send_packet, RAK_TIMER_PERIODIC);
		if (g_custom_parameters.send_interval != 0)
		{
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}
		api.system.timer.create(RAK_TIMER_1, handle_display, RAK_TIMER_ONESHOT);
	}
	else
	{
		api.system.timer.create(RAK_TIMER_1, handle_display, RAK_TIMER_ONESHOT);
	}

	// If LoRaWAN, start join if required
	if (lorawan_mode)
	{
		if (!api.lorawan.njs.get())
		{
			api.lorawan.join();
		}
	}
	MYLOG("APP", "Start testing");
	// Enable low power mode
	api.system.lpm.set(1);
}

/**
 * @brief Loop (unused)
 *
 */
void loop(void)
{
	api.system.sleep.all();
}

/**
 * @brief Set the module for LoRaWAN Confirmed Packet testing
 * 
 */
void set_cfm(void)
{
	MYLOG("APP", "Found CFM Mode");
	use_link_check = false;
	lorawan_mode = true;
	// Force LoRaWAN mode (might cause restart)
	api.lorawan.nwm.set();
	// Register callbacks
	api.lorawan.registerRecvCallback(recv_cb_lpw);
	api.lorawan.registerSendCallback(send_cb_lpw);
	api.lorawan.registerJoinCallback(join_cb_lpw);
	// Set confirmed packet mode
	api.lorawan.cfm.set(true);
	// Disable LinkCheck
	api.lorawan.linkcheck.set(0);
	if (!api.lorawan.join(1, 1, 10, 50))
	{
		MYLOG("APP", "Failed to start JOIN");
	}
}

/**
 * @brief Set the module for LoRaWAN LinkCheck testing
 *
 */
void set_linkcheck(void)
{
	MYLOG("APP", "Found LinkCheck Mode");
	use_link_check = true;
	lorawan_mode = true;
	// Force LoRaWAN mode (might cause restart)
	api.lorawan.nwm.set();
	// Register callbacks
	api.lorawan.registerRecvCallback(recv_cb_lpw);
	api.lorawan.registerSendCallback(send_cb_lpw);
	api.lorawan.registerJoinCallback(join_cb_lpw);
	api.lorawan.registerLinkCheckCallback(linkcheck_cb_lpw);
	// Set unconfirmed packet mode
	api.lorawan.cfm.set(false);
	// Disable LinkCheck
	api.lorawan.linkcheck.set(2);
	if (!api.lorawan.join(1, 1, 10, 50))
	{
		MYLOG("APP", "Failed to start JOIN");
	}
}

/**
 * @brief Set the module for LoRa P2P testing
 *
 */
void set_p2p(void)
{
	MYLOG("APP", "Found P2P Mode");
	lorawan_mode = false;
	if (!api.lorawan.join(0, 0, 10, 50))
	{
		MYLOG("APP", "Failed to stop JOIN");
	}
	if (!api.lora.precv(0))
	{
		MYLOG("APP", "Failed to stop P2P RX");
	}
	// Force LoRa P2P mode (might cause restart)
	if (!api.lora.nwm.set())
	{
		MYLOG("APP", "Failed to set P2P Mode");
	}
	// Register callbacks
	api.lora.registerPRecvCallback(recv_cb_p2p);
	// Enable RX mode
	api.lora.precv(65533);
}
