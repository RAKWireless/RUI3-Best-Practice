/**
 * @file RUI3-Mesh.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 main application for a LoRa Mesh network
 * @version 0.1
 * @date 2023-08-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"

/** Flag for the event type */
volatile uint16_t g_task_event_type = NO_EVENT;

/** Union to convert uint32_t into uint8_t array */
longlong_byte_u convert_value;

/** Counter, only for testing */
uint64_t msg_cnt = 0;

/** Buffer for BLE/Mesh data */
char data_buffer[512] = {0};

bool has_rak1921 = false;

/** Buffer for OLED output */
char line_str[256];

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
	
	// Save RX packet in queue to be processed by the mesh handler
	if (!add_rx_packet(data.Rssi, data.Snr, data.BufferSize, data.Buffer))
	{
		MYLOG("RX-P2P-CB", "RX queue is full");
	}
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
	// Check what to do after successful TX
	mesh_check_tx();
}

void cad_cb(bool result)
{
	MYLOG("CAD-P2P-CB", "CAD returned channel is %s", result ? "busy" : "free");
	if (result)
	{
		MYLOG("CAD-P2P-CB", "Restart RX");
	}
}

void setup(void)
{
	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_BLUE, LOW);
	digitalWrite(LED_GREEN, LOW);

	// Initialize Serial for debug output
	Serial.begin(115200);

	time_t serial_timeout = millis();
#if defined _VARIANT_RAK4630_
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
#endif

#if not defined _VARIANT_RAK4630_
	while ((millis() - serial_timeout) < 5000)
	{
		delay(100);
		digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
	}
#endif

	// Set LoRa P2P configuration
	api.lora.nwm.set();

	/// \todo this should be done by AT commands!
	// AT+P2P=916000000:7:0:1:8:5
	api.lora.pfreq.set(916000000);
	api.lora.psf.set(7);
	api.lora.pbw.set(0);
	api.lora.pcr.set(1);
	api.lora.ppl.set(8);
	api.lora.ptp.set(5);

	digitalWrite(LED_GREEN, LOW);

	has_rak1921 = init_rak1921();

	if (has_rak1921)
	{
		sprintf(line_str, "RUI3 Mesh - B %.2fV", api.system.bat.get());
		rak1921_write_header(line_str);
	}
	else
	{
		MYLOG("APP", "No OLED found");
	}
	Serial.println("***************************************************************");
	Serial.println("WisBlock RUI3 Mesh");
	Serial.println("***************************************************************");
	Serial.println("** All mesh nodes MUST be set to the same                    **");
	Serial.println("** LoRa frequency        Bandwidth                           **");
	Serial.println("** Spreading factor      Coding rate                         **");
	Serial.println("** Preamble length       Symbol timeout                      **");
	Serial.println("***************************************************************");

	// Init custom AT commands
	init_interval_at();
	init_status_at();
	init_map_at();
	init_master_at();

	// Get saved custom parameters
	get_at_setting();

	// Setup callbacks
	g_mesh_events.data_avail_cb = on_mesh_data;
	g_mesh_events.map_changed_cb = map_changed_cb;
	api.lora.registerPRecvCallback(recv_cb);
	api.lora.registerPSendCallback(send_cb);
	api.lora.registerPSendCADCallback(cad_cb);

	// Initialize the LoRa Mesh * events
	init_mesh(&g_mesh_events);

	// Enable RX mode (always with TX allowed)
	api.lora.precv(65533);

	// Timer to handle events
	api.system.timer.create(RAK_TIMER_3, timed_loop, RAK_TIMER_ONESHOT);
	api.system.timer.start(RAK_TIMER_3, EVENT_HANDLER_TIME, NULL);

	// Timer for interval data sending
	api.system.timer.create(RAK_TIMER_2, status_timer, RAK_TIMER_PERIODIC);
	if (g_custom_parameters.send_interval != 0)
	{
		MYLOG("APP", "Start Interval %d", g_custom_parameters.send_interval);
		api.system.timer.start(RAK_TIMER_2, g_custom_parameters.send_interval, NULL);
	}
	api.system.lpm.set(1);

	api.system.wdt.enable(0x0FA0);
}

/**
 * @brief This example is complete timer
 * driven. The loop() does nothing than
 * sleep.
 *
 */
void loop()
{
	api.system.sleep.all();
	// api.system.scheduler.task.destroy();
}

void status_timer(void *)
{
	// MYLOG("WDT","status_timer");
	api.system.wdt.reset();
	g_task_event_type |= STATUS;
}

void timed_loop(void *)
{
	digitalWrite(LED_BLUE, HIGH);
	// MYLOG("WDT", "timed_loop 1");
	api.system.wdt.reset();

	while (g_task_event_type != NO_EVENT)
	{
		// Mesh Map changed
		if ((g_task_event_type & MESH_MAP_CHANGED) == MESH_MAP_CHANGED)
		{
			g_task_event_type &= N_MESH_MAP_CHANGED;
			Serial.println("Mesh Map");
			// Print the node map
			print_mesh_map();
			// print_mesh_map_oled();
		}

		// Timer triggered event
		if ((g_task_event_type & STATUS) == STATUS)
		{
			// print_mesh_map_oled();
			g_task_event_type &= N_STATUS;
			MYLOG("APP", "STATUS");

			// Create a dummy data packet with 2x 64bit counter value + device address
			msg_cnt++;

			convert_value.l_value = msg_cnt;
			memcpy(&data_buffer[0], convert_value.b_values, 8);
			memcpy(&data_buffer[8], convert_value.b_values, 8);
			convert_value.l_value = g_this_device_addr;
			memcpy(&data_buffer[16], convert_value.b_values, 8);

			// Select a random node from the map
			uint8_t selected_node_idx = 0;
			// Select broadcast as default
			bool use_broadcast = true;
			// Target node address;
			uint32_t node_addr = 0x00;

			if (g_custom_parameters.master_address != 0)
			{
				node_addr = g_custom_parameters.master_address;
				g_nodes_list_s route;
				// Check if we have a route to the master
				if (get_route(g_custom_parameters.master_address, &route))
				{
					MYLOG("APP", "Send to master node %08lX", g_custom_parameters.master_address);
					use_broadcast = false;
				}
				else
				{
					// No route, send as broadcast
					MYLOG("APP", "No route to master node %08lX", g_custom_parameters.master_address);
					use_broadcast = true;
				}
			}
			else
			{
				// Get the number of nodes in the map
				uint8_t node_index = nodes_in_map() + 1;
				// MYLOG("APP", "%d nodes in the map", node_index);

				// Check how many nodes are in the map
				if (node_index > 2)
				{
					// Multiple nodes, select a random one
					selected_node_idx = (uint8_t)random(1, (long)node_index - 1);
					use_broadcast = false;
					MYLOG("APP", "Using node %d", selected_node_idx);
				}
				else if (node_index == 2)
				{
					// Only 2 nodes in the map, send to the other one
					selected_node_idx = 1;
					use_broadcast = false;
					MYLOG("APP", "Using node 1");
				}
				else
				{
					// No other node, lets send a broadcast
					selected_node_idx = 0;
					use_broadcast = false;
					MYLOG("APP", "Using broadcast");
				}
				node_addr = get_node_addr(selected_node_idx);
				// MYLOG("APP", "Got receiver address %08lX", node_addr);
			}

			// Enqueue the data package for sending
			uint8_t data_size = 24;
			if (!send_to_mesh(use_broadcast, node_addr, (uint8_t *)data_buffer, data_size))
			{
				MYLOG("APP", "Error enqueue packet");
			}
		}
	}

	// MYLOG("WDT", "timed_loop 2");
	api.system.wdt.reset();

	// Handle Mesh events
	while (mesh_event != NO_EVENT)
	{
		mesh_task(NULL);
	}

	// MYLOG("WDT", "timed_loop 3");
	api.system.wdt.reset();

	// mesh_task(NULL);
	digitalWrite(LED_BLUE, LOW);

	api.system.timer.start(RAK_TIMER_3, EVENT_HANDLER_TIME, NULL);
}

/**
 * @brief Callback after a LoRa Mesh data package was received
 *
 * @param fromID		Sender Node address
 * @param rxPayload		Pointer to the received data
 * @param rxSize		Length of the received package
 * @param rxRssi		Signal strength while the package was received
 * @param rxSnr			Signal to noise ratio while the package was received
 */
void on_mesh_data(uint32_t fromID, uint8_t *rxPayload, uint16_t rxSize, int16_t rxRssi, int8_t rxSnr)
{
	Serial.println("-------------------------------------");
	Serial.printf("Got data from node %08lX\n", fromID);

	longlong_byte_u new_counter;

	// Demo output of the received data packet
	memcpy(new_counter.b_values, &rxPayload[0], 8);
	Serial.printf("Counter %Ld\n", new_counter.l_value);
	memcpy(new_counter.b_values, &rxPayload[8], 8);
	Serial.printf("Sensor Value 1 %Ld\n", new_counter.l_value);
	memcpy(new_counter.b_values, &rxPayload[16], 8);
	Serial.printf("Device %08lX\n", new_counter.l_value);

	Serial.println("-------------------------------------");
}

/**
 * @brief Callback after the nodes list changed
 *
 */
void map_changed_cb(void)
{
	// No actions required, but can be used to react to a change in the Mesh Map
	// Do not use long code here, just set a flag and handle the event in the app_event_handler()
	g_task_event_type |= MESH_MAP_CHANGED;
}