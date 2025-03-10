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

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	// Add received data into FiFo Queue
	if (has_wifi_conn && has_mqtt_conn)
	{
		if (!Fifo.enQueue(data.Buffer, data.BufferSize))
		{
			// interrupts();
			MYLOG("RX-P2P-CB", "FiFo full");
			return;
		}
	}
	MYLOG("RX-P2P-CB", "%d FiFo entries ", Fifo.getSize());
	if (!wifi_sending)
	{
		MYLOG("RX-P2P-CB", "Handle Fifo");
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
	digitalWrite(LED_WIFI, LOW);
}
