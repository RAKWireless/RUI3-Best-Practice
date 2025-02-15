/**
 * @file lora_cb.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief LoRa P2P callbacks
 * @version 0.1
 * @date 2025-02-12
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "app.h"

/** Temporary buffer for received LoRa P2P packets */
uint8_t rcvd_buffer[256];
/** Size of data in temporary buffer */
uint16_t rcvd_buffer_size = 0;

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	// MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
	// for (int i = 0; i < data.BufferSize; i++)
	// {
	// 	Serial.printf("%02X", data.Buffer[i]);
	// }
	// Serial.print("\r\n");

	if (!wifi_sending)
	{
		// Copy received data into buffer
		memcpy(rcvd_buffer, data.Buffer, data.BufferSize);
		rcvd_buffer_size = data.BufferSize;

		wifi_sending = true;
		// Activate send to WiFi function
		api.system.timer.start(RAK_TIMER_0, 100, NULL);
	}
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	// MYLOG("TX-P2P-CB", "P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
}
