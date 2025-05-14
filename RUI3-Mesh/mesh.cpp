/**
 * @file mesh.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Mesh network functions
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */
/*********************************************/
/* Could move to WisBlock-API-V2             */
/*********************************************/

#include "main.h"
#include <cppQueue.h>

/** Structure with Mesh event callbacks */
mesh_events_s g_mesh_events;

#define BROKEN_NET

/** Max number of messages in the queue */
#define TX_QUEUE_SIZE 2
#define RX_QUEUE_SIZE 4

data_msg_s tx_queue[TX_QUEUE_SIZE];
rx_msg_s rx_queue[RX_QUEUE_SIZE];

/** Queue to handle outgoing packets */
cppQueue mesh_tx_queue(sizeof(data_msg_s), TX_QUEUE_SIZE, FIFO, false, tx_queue, sizeof(tx_queue));

/** Queue to handle incoming data packets*/
cppQueue mesh_rx_queue(sizeof(rx_msg_s), RX_QUEUE_SIZE, FIFO, false, rx_queue, sizeof(rx_queue));

/** Event flag for Mesh Task */
volatile uint16_t mesh_event;

/** The Mesh node ID, created from ID of the nRF52 */
uint32_t g_this_device_addr;
/** The Mesh broadcast ID, created from node ID */
uint32_t g_broadcast_id;

/** Map message buffer */
map_msg_s map_sync_msg;

/** Structure for outgoing data */
data_msg_s out_data_buffer;

/** Max number of messages in the queue */
#define TX_QUEUE_SIZE 4

/** LoRa TX package */
uint8_t tx_pckg[256];
/** Size of data package */
uint16_t tx_buff_len = 0;
/** LoRa RX buffer */
uint8_t rx_buffer[256];

/** Sync time for routing at start, every 30 seconds */
#define INIT_SYNCTIME 30000
/** Sync time for routing after mesh has settled, every 10 minutes*/
#define DEFAULT_SYNCTIME 600000
/** Sync time */
time_t sync_time = INIT_SYNCTIME;
/** Counter to switch from initial sync time to default sync time*/
uint8_t switch_sync_time_cnt = 10;

/** Enum for network state */
typedef enum
{
	MESH_IDLE = 0, //!< The radio is idle
	MESH_RX,	   //!< The radio is in reception state
	MESH_TX,	   //!< The radio is in transmission state
	MESH_NOTIF	   //!< The radio is doing mesh notification
} mesh_radio_state_t;

/** Lora statemachine status */
mesh_radio_state_t lora_state = MESH_IDLE;

/** Mesh callback variable */
mesh_events_s *_mesh_events;

/** Number of nodes in the map */
int g_num_of_nodes = 0;

/** Flag if the nodes map has changed */
boolean nodes_changed = false;

/**
 * @brief Callback for Mesh Map Update Timer
 *
 * @param unused
 */
void map_sync_handler(void *)
{
	mesh_event |= SYNC_MAP;
}

/**
 * @brief Initialize the Mesh network
 *
 * @param events Structure of event callbacks
 */
void init_mesh(mesh_events_s *events)
{
	_mesh_events = events;

	// Adjust max number of nodes in the mesh depending on the RUI3 device
#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
	g_num_of_nodes = 15;
#else
	g_num_of_nodes = 30;
#endif

	// Prepare empty nodes map
	g_nodes_map = (g_nodes_list_s *)malloc(g_num_of_nodes * sizeof(g_nodes_list_s));

	if (g_nodes_map == NULL)
	{
		MYLOG("MESH", "Could not allocate memory for nodes map");
	}
	else
	{
		MYLOG("MESH", "Memory for nodes map is allocated");
	}
	memset(g_nodes_map, 0, g_num_of_nodes * sizeof(g_nodes_list_s));

	// Flush queue
	mesh_tx_queue.flush();
	mesh_rx_queue.flush();

	data_msg_s temp;
	rx_msg_s temp2;

	// Push/pop to test queue
	MYLOG("MESH", "TX Queue is %s", mesh_tx_queue.isInitialized() ? "initialized" : "not initialized");
	MYLOG("MESH", "TX Queue is %s", mesh_tx_queue.isFull() ? "full" : "not full");
	MYLOG("MESH", "TX Queue is %s", mesh_tx_queue.isEmpty() ? "empty" : "not empty");
	// Push/pop to test queue
	MYLOG("MESH", "RX Queue is %s", mesh_rx_queue.isInitialized() ? "initialized" : "not initialized");
	MYLOG("MESH", "RX Queue is %s", mesh_rx_queue.isFull() ? "full" : "not full");
	MYLOG("MESH", "RX Queue is %s", mesh_rx_queue.isEmpty() ? "empty" : "not empty");

	if (!mesh_tx_queue.push((void *)&temp.mark1))
	{
		MYLOG("MESH", "Failed to add packet to TX queue");
	}
	if (!mesh_tx_queue.pop((void *)&temp.mark1))
	{
		MYLOG("MESH", "Failed to get packet from TX queue");
	}
	if (!mesh_rx_queue.push((void *)&temp2.rx_size))
	{
		MYLOG("MESH", "Failed to add packet to RX queue");
	}
	if (!mesh_rx_queue.pop((void *)&temp2.rx_size))
	{
		MYLOG("MESH", "Failed to get packet from RX queue");
	}

	MYLOG("MESH", "Send queue created!");

	// Create node ID
	/// \todo this doesn't work in LoRa P2P mode
	uint8_t deviceMac[8];
	api.lorawan.deui.get(deviceMac, 8);

	g_this_device_addr = (uint32_t)deviceMac[4] << 24;
	g_this_device_addr += (uint32_t)deviceMac[5] << 16;
	g_this_device_addr += (uint32_t)deviceMac[6] << 8;
	g_this_device_addr += (uint32_t)deviceMac[7];

	MYLOG("MESH", "Got DevEUI %08X", g_this_device_addr);

	if (g_this_device_addr == 0)
	{
#if defined _VARIANT_RAK3172_ || defined _VARIANT_RAK3172_SIP_
		uint32_t unique_id = HAL_GetDEVID();
		MYLOG("MESH", "DEVID %08X", HAL_GetDEVID());
		MYLOG("MESH", "REVID %08X", HAL_GetREVID());
		g_this_device_addr = (HAL_GetDEVID() << 16) + HAL_GetREVID();
#endif
#if defined _VARIANT_RAK4630_
		uint32_t deviceID[2];
		deviceID[0] = NRF_FICR->DEVICEID[0];
		deviceID[1] = NRF_FICR->DEVICEID[1];
		MYLOG("MESH", "DEVID[0] %04X", deviceID[0]);
		MYLOG("MESH", "DEVID[1] %04X", deviceID[1]);

		g_this_device_addr += (uint32_t)deviceMac[4] << 24;
		g_this_device_addr += (uint32_t)deviceMac[5] << 16;
		g_this_device_addr += (uint32_t)deviceMac[6] << 8;
		g_this_device_addr += (uint32_t)deviceMac[7];
#endif
	}
	MYLOG("MESH", "Mesh NodeId = %08lX", g_this_device_addr);

	// Create broadcast ID
	g_broadcast_id = g_this_device_addr & 0xFFFFFF00;
	MYLOG("MESH", "Broadcast ID is %08lX", g_broadcast_id);

	// if (!api.system.scheduler.task.create("MeshSync", (RAK_TASK_HANDLER)mesh_task)) //
	// {
	// 	MYLOG("MESH", "Starting Mesh Sync Task failed");
	// }
	// else
	// {
	// 	MYLOG("MESH", "Starting Mesh Sync Task success");
	// }

	api.system.timer.create(RAK_TIMER_1, map_sync_handler, RAK_TIMER_ONESHOT);
	api.system.timer.start(RAK_TIMER_1, sync_time, NULL);

	mesh_task(NULL);

	// Wake up task to handle request for nodes map
	mesh_event |= SYNC_MAP;
}

bool mesh_task_active = false;

/**
 * @brief Task to handle the mesh
 *
 * @param unused Unused task parameters
 */
void mesh_task(void *unused)
{
	// if (mesh_task_active)
	// {
	// 	MYLOG("MESH", "Mesh task already running");
	// 	return;
	// }
	// mesh_task_active = true;

	lora_state = MESH_IDLE;

	if (mesh_event != 0)
	{
		if ((mesh_event & CHECK_RX) == CHECK_RX)
		{
			MYLOG("MESH", "Mesh task check RX");
			mesh_event &= N_CHECK_RX;
			mesh_check_rx();
			return;
		}

		// MYLOG("MESH", "Mesh task wakeup");
		if ((mesh_event & NODE_CHANGE) == NODE_CHANGE)
		{
			mesh_event &= N_NODE_CHANGE;

			MYLOG("MESH", "Mesh task Node change");
			nodes_changed = false;
			if ((_mesh_events != NULL) && (_mesh_events->map_changed_cb != NULL))
			{
				_mesh_events->map_changed_cb();
			}
			return;
		}

		if ((mesh_event & CHECK_QUEUE) == CHECK_QUEUE)
		{
			mesh_event &= N_CHECK_QUEUE;

			MYLOG("MESH", "Mesh task check send queue");

			// Check if we have something in the queue
			if (mesh_tx_queue.getCount() != 0)
			{
				if (lora_state != MESH_TX)
				{
					memset(tx_pckg, 0, 256);

					if (mesh_tx_queue.pop((data_msg_s *)tx_pckg))
					{
						// Get message length
						data_msg_s *tx_check = (data_msg_s *)tx_pckg;
						tx_buff_len = tx_check->data[242];

						lora_state = MESH_TX;

						// api.lora.precv(0);
						// Send packet over LoRa
						if (api.lora.psend(tx_buff_len, (uint8_t *)&tx_pckg, true))
						{
							MYLOG("MESH", "Packet enqueued");
						}
						else
						{
							// api.lora.precv(65535);
							MYLOG("MESH", "+EVT:SEND_ERROR");
						}
					}
					else
					{
						MYLOG("MESH", "Queue is full");
					}
				}
				else
				{
					MYLOG("MESH", "TX still active");
				}
			}
			else
			{
				MYLOG("MESH", "Packet queue is empty");
			}
			return;
		}

		if ((mesh_event & SYNC_MAP) == SYNC_MAP)
		{
			MYLOG("MESH", "Mesh task Sync Map");
			mesh_event &= N_SYNC_MAP;

			// Time to sync the Mesh

			// MYLOG("MESH", "Checking mesh map");
			if (!clean_map())
			{
				sync_time = INIT_SYNCTIME;
				if ((_mesh_events != NULL) && (_mesh_events->map_changed_cb != NULL))
				{
					_mesh_events->map_changed_cb();
				}
			}
			MYLOG("MESH", "Sending mesh map");
			map_sync_msg.from = g_this_device_addr;
			map_sync_msg.type = LORA_NODEMAP;
			memset(map_sync_msg.nodes, 0, 48 * 5);

			// Get sub nodes
			uint8_t subs_len = node_map(map_sync_msg.nodes);
			MYLOG("MESH", "Map size is %d", subs_len);
			if (subs_len != 0)
			{
				for (int idx = 0; idx < subs_len; idx++)
				{
					uint32_t checkNode = map_sync_msg.nodes[idx][0];
					checkNode |= map_sync_msg.nodes[idx][1] << 8;
					checkNode |= map_sync_msg.nodes[idx][2] << 16;
					checkNode |= map_sync_msg.nodes[idx][3] << 24;
				}
			}
			map_sync_msg.nodes[subs_len][0] = 0xAA;
			map_sync_msg.nodes[subs_len][1] = 0x55;
			map_sync_msg.nodes[subs_len][2] = 0x00;
			map_sync_msg.nodes[subs_len][3] = 0xFF;
			map_sync_msg.nodes[subs_len][4] = 0xAA;
			subs_len++;

			// print_mesh_map();

			// Serial.printf("Nodes %d\n", subs_len);
			// Serial.printf("%c%c%c\n", map_sync_msg.mark1, map_sync_msg.mark2, map_sync_msg.mark3);
			// Serial.printf("Type %d\n", map_sync_msg.type);
			// Serial.printf("Dest %08X\n", map_sync_msg.dest);
			// Serial.printf("From %08X\n", map_sync_msg.from);
			// for (int idx = 0; idx < subs_len; idx++)
			// {
			// 	Serial.printf("Node %02X%02X%02X%02X\n", map_sync_msg.nodes[idx][0], map_sync_msg.nodes[idx][1], map_sync_msg.nodes[idx][2], map_sync_msg.nodes[idx][3]);
			// }

			subs_len = MAP_HEADER_SIZE + (subs_len * 5);

			MYLOG("MESH", "Header Size %d", subs_len);

			if (!add_send_request((data_msg_s *)&map_sync_msg, subs_len))
			{
				MYLOG("MESH", "Cannot send map because send queue is full");
			}

			// Time to relax the syncing ???
			switch_sync_time_cnt--;

			if (switch_sync_time_cnt == 0)
			{
				MYLOG("MESH", "Switching sync time to DEFAULT_SYNCTIME");
				sync_time = DEFAULT_SYNCTIME;
			}

			// Restart map sync timer
			api.system.timer.start(RAK_TIMER_1, sync_time, NULL);
		}
	}
	// mesh_task_active = false;
}

// To simulate a mesh where some nodes are out of range,
// replace the 3 node addresses below with real node addresses
// Requires at least 3 nodes in the net
// NODE 1 can only connect to NODE 3
// NODE 2 can only connect to NODE 3

#define BROKEN_NODE_1 0x6FA6BC6C
#define BROKEN_NODE_2 0xCBE0E4F5
#define BROKEN_NODE_3 0x2BD56908

/**
 * Callback after a LoRa package was received
 *
 */
void mesh_check_rx(void)
{
	rx_msg_s rx_pckg;

	if (mesh_rx_queue.isInitialized())
	{
		while (!mesh_rx_queue.isEmpty())
		{
			if (mesh_rx_queue.pop((rx_msg_s *)&rx_pckg.rx_size))
			{
				lora_state = MESH_IDLE;

				// Check the received data
				if ((rx_pckg.rx_buffer[0] == 'L') && (rx_pckg.rx_buffer[1] == 'o') && (rx_pckg.rx_buffer[2] == 'R'))
				{
					// Valid Mesh data received
					map_msg_s *thisMsg = (map_msg_s *)rx_pckg.rx_buffer;
					data_msg_s *thisDataMsg = (data_msg_s *)rx_pckg.rx_buffer;

					if (thisMsg->type == LORA_NODEMAP)
					{
						/// \todo for debug make some nodes unreachable
#ifdef BROKEN_NET
						switch (g_this_device_addr)
						{
						case BROKEN_NODE_1:
							if (thisMsg->from == BROKEN_NODE_2)
							{
								return;
							}
						case BROKEN_NODE_2:
							if (thisMsg->from == BROKEN_NODE_1)
							{
								return;
							}
						}
#endif
						MYLOG("MESH", "Got map message");
						// Mapping received
						uint8_t subsSize = rx_pckg.rx_size - MAP_HEADER_SIZE;
						uint8_t numSubs = subsSize / 5;

						// Serial.println("********************************");
						// for (int idx = 0; idx < tempSize; idx++)
						// {
						// 	Serial.printf("%02X ", rx_buffer[idx]);
						// }
						// Serial.println("");
						// Serial.printf("subsSize %d -> # subs %d\n", subsSize, subsSize / 5);
						// Serial.println("********************************");

						// Serial.printf("%c%c%c\n", rx_buffer[0], rx_buffer[1], rx_buffer[2]);
						// Serial.printf("Type %d\n", thisMsg->type);
						// Serial.printf("Dest %08X\n", thisMsg->dest);
						// Serial.printf("From %08X\n", thisMsg->from);

						// for (int idx = 0; idx < numSubs; idx++)
						// {
						// 	Serial.printf("Node %02X%02X%02X%02X\n", map_sync_msg.nodes[idx][0], map_sync_msg.nodes[idx][1], map_sync_msg.nodes[idx][2], map_sync_msg.nodes[idx][3]);
						// }

#ifndef SHOW_MAP
						sprintf(line_str, "Map from %08LX", thisMsg->from);
						rak1921_add_line(line_str);
#endif
						// Check if end marker is in the message
						if ((thisMsg->nodes[numSubs - 1][0] != 0xAA) ||
							(thisMsg->nodes[numSubs - 1][1] != 0x55) ||
							(thisMsg->nodes[numSubs - 1][2] != 0x00) ||
							(thisMsg->nodes[numSubs - 1][3] != 0xFF) ||
							(thisMsg->nodes[numSubs - 1][4] != 0xAA))
						{
							MYLOG("MESH", "Invalid map, end marker is missing from %08lX", thisMsg->from);
							return;
						}
						nodes_changed = add_node(thisMsg->from, 0, 0);

						// Remove nodes that use sending node as hop
						clear_subs(thisMsg->from);

						// MYLOG("MESH", "From %08lX", thisMsg->from);
						// MYLOG("MESH", "Dest %08lX", thisMsg->dest);

						if (subsSize != 0)
						{
							// Mapping contains subs

							// MYLOG("MESH", "Msg size %d", tempSize);
							// MYLOG("MESH", "#subs %d", numSubs);

							// Serial.println("++++++++++++++++++++++++++++");
							// Serial.printf("From %08X Dest %08X #Subs %d\n", thisMsg->from, thisMsg->dest, numSubs);
							// for (int idx = 0; idx < numSubs; idx++)
							// {
							// 	uint32_t subId = (uint32_t)thisMsg->nodes[idx][0];
							// 	subId += (uint32_t)thisMsg->nodes[idx][1] << 8;
							// 	subId += (uint32_t)thisMsg->nodes[idx][2] << 16;
							// 	subId += (uint32_t)thisMsg->nodes[idx][3] << 24;
							// 	uint8_t hops = thisMsg->nodes[idx][4];
							// 	Serial.printf("ID: %08X numHops: %d\n", subId, hops);
							// }
							// Serial.println("++++++++++++++++++++++++++++");

							for (int idx = 0; idx < numSubs - 1; idx++)
							{
								uint32_t subId = (uint32_t)thisMsg->nodes[idx][0];
								subId += (uint32_t)thisMsg->nodes[idx][1] << 8;
								subId += (uint32_t)thisMsg->nodes[idx][2] << 16;
								subId += (uint32_t)thisMsg->nodes[idx][3] << 24;
								uint8_t hops = thisMsg->nodes[idx][4];
								if (subId != g_this_device_addr)
								{
									nodes_changed |= add_node(subId, thisMsg->from, hops + 1);
									// MYLOG("MESH", "Subs %08lX", subId);
								}
							}
						}

						if (nodes_changed)
						{
							// Wake up task to handle change of nodes map
							// mesh_event |= NODE_CHANGE;
							g_task_event_type |= MESH_MAP_CHANGED;
						}

						// print_mesh_map_oled();
					}
					else if (thisDataMsg->type == LORA_DIRECT)
					{
						MYLOG("MESH", "Direct message from %08lX", thisMsg->from);
						// MYLOG("MESH", "From %08lX", thisDataMsg->from);
						// MYLOG("MESH", "Dest %08lX", thisDataMsg->dest);

#ifndef SHOW_MAP
						sprintf(line_str, "Dir %08LX R %d S %d", thisMsg->from, rx_pckg.rx_rssi, rx_pckg.rx_snr);
						rak1921_add_line(line_str);
#endif

						if (thisDataMsg->dest == g_this_device_addr)
						{
							// 							MYLOG("MESH", "LoRa Packet received size:%d, rssi:%d, snr:%d", rx_pckg.rx_size, rx_pckg.rx_rssi, rx_pckg.rx_snr);
							// #if MY_DEBUG > 0
							// 							for (int idx = 0; idx < rx_pckg.rx_size; idx++)
							// 							{
							// 								Serial.printf(" %02X", rx_pckg.rx_buffer[idx]);
							// 							}
							// 							Serial.println("");
							// #endif
							// Message is for us, call user callback to handle the data
							// MYLOG("MESH", "Got data message type %c >%s<", thisDataMsg->data[0], (char *)&thisDataMsg->data[1]);
							if ((_mesh_events != NULL) && (_mesh_events->data_avail_cb != NULL))
							{
								if (thisDataMsg->orig == 0x00)
								{
									_mesh_events->data_avail_cb(thisDataMsg->from, thisDataMsg->data, rx_pckg.rx_size - DATA_HEADER_SIZE, rx_pckg.rx_rssi, rx_pckg.rx_snr, false);
								}
								else
								{
									_mesh_events->data_avail_cb(thisDataMsg->orig, thisDataMsg->data, rx_pckg.rx_size - DATA_HEADER_SIZE, rx_pckg.rx_rssi, rx_pckg.rx_snr, false);
								}
							}
						}
						else
						{
							// Message is not for us forward the message
							MYLOG("MESH", "Msg for node %08lX", thisDataMsg->dest);
						}
						// Check if we know that node
						if (!check_node(thisDataMsg->from))
						{
							// Unknown node, force a map update
							MYLOG("MESH", "Unknown node, force map update");
							send_map_request();
						}
					}
					else if (thisDataMsg->type == LORA_FORWARD)
					{
						MYLOG("MESH", "Forward message from %08lX", thisMsg->from);
#ifndef SHOW_MAP
						sprintf(line_str, "Forw %08LX R %d S %d", thisMsg->from, rx_pckg.rx_rssi, rx_pckg.rx_snr);
						rak1921_add_line(line_str);
#endif
						if (thisDataMsg->dest == g_this_device_addr)
						{
							// 							MYLOG("MESH", "LoRa Packet received size:%d, rssi:%d, snr:%d", rx_pckg.rx_size, rx_pckg.rx_rssi, rx_pckg.rx_snr);
							// #if MY_DEBUG > 0
							// 							for (int idx = 0; idx < rx_pckg.rx_size; idx++)
							// 							{
							// 								Serial.printf(" %02X", rx_pckg.rx_buffer[idx]);
							// 							}
							// 							Serial.println("");
							// #endif
							// Message is for us, call user callback to handle the data
							// MYLOG("MESH", "Got data message type %c >%s<", thisDataMsg->data[0], (char *)&thisDataMsg->data[1]);
							if ((_mesh_events != NULL) && (_mesh_events->data_avail_cb != NULL))
							{
								if (thisDataMsg->orig == 0x00)
								{
									_mesh_events->data_avail_cb(thisDataMsg->from, thisDataMsg->data, rx_pckg.rx_size - DATA_HEADER_SIZE, rx_pckg.rx_rssi, rx_pckg.rx_snr, false);
								}
								else
								{
									_mesh_events->data_avail_cb(thisDataMsg->orig, thisDataMsg->data, rx_pckg.rx_size - DATA_HEADER_SIZE, rx_pckg.rx_rssi, rx_pckg.rx_snr, false);
								}
							}
						}
						else
						{
							// Message is for sub node, forward the message
							g_nodes_list_s route;
							if (get_route(thisDataMsg->from, &route))
							{
								// We found a route, send package to next hop
								if (route.first_hop == 0)
								{
									// Node is in range, use direct message
									// MYLOG("MESH", "Route for %lX is direct", route.node_id);
									// Destination is a direct
									thisDataMsg->dest = thisDataMsg->from;
									thisDataMsg->from = thisDataMsg->orig;
									thisDataMsg->type = LORA_DIRECT;
								}
								else
								{
									// Node is not in range, use forwarding
									// MYLOG("MESH", "Route for %lX is to %lX", route.node_id, route.first_hop);
									// Destination is a sub
									thisDataMsg->dest = route.first_hop;
									thisDataMsg->type = LORA_FORWARD;
								}

								// Put message into send queue
								if (!add_send_request(thisDataMsg, rx_pckg.rx_size))
								{
									MYLOG("MESH", "Cannot forward message because send queue is full");
								}
							}
							else
							{
								MYLOG("MESH", "No route found for %lX", thisMsg->from);
							}
						}
						// Check if we know that node
						if (!check_node(thisDataMsg->from))
						{
							// Unknown node, force a map update
							MYLOG("MESH", "Unknown node, force map update");
							send_map_request();
						}
					}
					else if (thisDataMsg->type == LORA_BROADCAST)
					{
						MYLOG("MESH", "Broadcast message from %08lX", thisMsg->from);
#ifndef SHOW_MAP
						sprintf(line_str, "BRDC %08LX R %d S %d", thisMsg->from, rx_pckg.rx_rssi, rx_pckg.rx_snr);
						rak1921_add_line(line_str);
#endif
						// This is a broadcast. Forward to all direct nodes, but not to the one who sent it
						// MYLOG("MESH", "Handling broadcast with ID %08lX from %08lX", thisDataMsg->dest, thisDataMsg->from);
						// Check if this broadcast is coming from ourself
						if ((thisDataMsg->dest & 0xFFFFFF00) == (g_this_device_addr & 0xFFFFFF00))
						{
							// MYLOG("MESH", "We received our own broadcast, dismissing it");
							return;
						}
						// Check if we handled this broadcast already
						if (is_old_broadcast(thisDataMsg->dest))
						{
							// MYLOG("MESH", "Got an old broadcast, dismissing it");
							return;
						}

						// Put broadcast into send queue
						if (!add_send_request(thisDataMsg, rx_pckg.rx_size))
						{
							MYLOG("MESH", "Cannot forward broadcast because send queue is full");
						}

						// This is a broadcast, call user callback to handle the data
						// MYLOG("MESH", "Got data broadcast size %ld", tempSize);
						if ((_mesh_events != NULL) && (_mesh_events->data_avail_cb != NULL))
						{
							_mesh_events->data_avail_cb(thisDataMsg->from, thisDataMsg->data, rx_pckg.rx_size - DATA_HEADER_SIZE, rx_pckg.rx_rssi, rx_pckg.rx_snr, true);
						}
						// Check if we know that node
						if (!check_node(thisDataMsg->from))
						{
							// Unknown node, force a map update
							MYLOG("MESH", "Unknown node, force map update");
							send_map_request();
						}
					}
					else if (thisDataMsg->type == LORA_MAP_REQ)
					{
#ifndef SHOW_MAP
						sprintf(line_str, "MREQ %08LX R %d S %d", thisMsg->from, rx_pckg.rx_rssi, rx_pckg.rx_snr);
						rak1921_add_line(line_str);
#endif

						// This is a broadcast. Forward to all direct nodes, but not to the one who sent it
						MYLOG("MESH", "Handling Mesh map request with ID %08lX from %08lX", thisDataMsg->dest, thisDataMsg->from);
						// Check if this broadcast is coming from ourself
						if ((thisDataMsg->dest & 0xFFFFFF00) == (g_this_device_addr & 0xFFFFFF00))
						{
							MYLOG("MESH", "We received our own broadcast, dismissing it");
							return;
						}
						// Check if we handled this broadcast already
						if (is_old_broadcast(thisDataMsg->dest))
						{
							MYLOG("MESH", "Got an old broadcast, dismissing it");
							return;
						}

						// Put broadcast into send queue
						if (!add_send_request(thisDataMsg, rx_pckg.rx_size))
						{
							MYLOG("MESH", "Cannot forward broadcast because send queue is full");
						}
						// Wake up task to handle request for nodes map
						mesh_event |= SYNC_MAP;
					}
				}
				else
				{
					MYLOG("MESH", "Invalid package");
					for (int idx = 0; idx < rx_pckg.rx_size; idx++)
					{
						Serial.printf("%02X ", rx_pckg.rx_buffer[idx]);
					}
					Serial.println("");
				}
			}
			else
			{
				MYLOG("MESH", "Error getting msg from RX buffer");
				break;
			}
		}
	}
	else
	{
		MYLOG("MESH", "RX queue not initialized");
	}
}

/**
 * @brief Callback after a package was sent
 *
 */
void mesh_check_tx(void)
{
	// MYLOG("MESH", "LoRa send finished");
	lora_state = MESH_IDLE;

	mesh_event |= CHECK_QUEUE;
}

/**
 * Add a data package to the queue
 * @param package
 * 			dataPckg * to the package data
 * @param msg_size
 * 			Size of the data package
 * @return result
 * 			TRUE if task could be added to queue
 * 			FALSE if queue is full or not initialized
 */
bool add_send_request(data_msg_s *package, uint8_t msg_size)
{
	data_msg_s temp;

	if (mesh_tx_queue.isInitialized())
	{
		if (!mesh_tx_queue.isFull())
		{
			// Found an empty entry!
			memcpy((void *)&temp.mark1, (void *)&package->mark1, msg_size);
			temp.data[242] = msg_size;

			// Try to add to cloudTaskQueue
			if (!mesh_tx_queue.push((void *)&temp.mark1))
			{
				MYLOG("MESH", "Failed to add packet to TX queue");
				return false;
			}
			else
			{
				// MYLOG("MESH", "Queued msg #%d with len %d", next, msg_size);
				mesh_event |= CHECK_QUEUE;
				return true;
			}
		}
		else
		{
			MYLOG("MESH", "Send queue is full");
			// Queue is already full!
			return false;
		}
	}
	else
	{
		MYLOG("MESH", "Send queue not yet initialized");
		return false;
	}
}

bool add_rx_packet(int16_t rssi, int8_t snr, uint8_t size, uint8_t *buffer)
{
	rx_msg_s temp;

	if (mesh_rx_queue.isInitialized())
	{
		if (!mesh_rx_queue.isFull())
		{
			// Found an empty entry!
			memcpy((void *)temp.rx_buffer, buffer, size);

			temp.rx_rssi = rssi;
			temp.rx_snr = snr;
			temp.rx_size = size;

			// Try to add to cloudTaskQueue
			if (!mesh_rx_queue.push((void *)&temp.rx_size))
			{
				MYLOG("MESH", "Failed to add packet to RX queue");
				return false;
			}
			else
			{
				// MYLOG("MESH", "Queued msg #%d with len %d", next, msg_size);
				mesh_event |= CHECK_RX;
				return true;
			}
		}
		else
		{
			MYLOG("MESH", "RX queue is full");
			// Queue is already full!
			return false;
		}
	}
	else
	{
		MYLOG("MESH", "RX queue not yet initialized");
		return false;
	}
}

/**
 * @brief Print the current Mesh Map to Serial and BLE
 *
 */
void print_mesh_map(void)
{
	/** Node ID of the selected receiver node */
	uint32_t node_id[48];
	/** First hop ID of the selected receiver node */
	uint32_t first_hop[48];
	/** Number of hops to the selected receiver node */
	uint8_t num_hops[48];
	/** Number of nodes in the map */
	uint8_t num_elements;

	MYLOG("MESH", "---------------------------------------------");
	/** Number of nodes in the map */
	num_elements = nodes_in_map();

	for (int idx = 0; idx < num_elements; idx++)
	{
		get_node(idx, node_id[idx], first_hop[idx], num_hops[idx]);
	}
	// Display the nodes
	MYLOG("MESH", "%d nodes in the map", num_elements + 1);
	MYLOG("MESH", "Node #01 id: %08lX this node", g_this_device_addr);
	for (int idx = 0; idx < num_elements; idx++)
	{
		if (first_hop[idx] == 0)
		{
			MYLOG("MESH", "Node #%02d id: %08lX direct", idx + 2, node_id[idx]);
		}
		else
		{
			MYLOG("MESH", "Node #%02d id: %08lX first hop %08lX #hops %d", idx + 2, node_id[idx], first_hop[idx], num_hops[idx]);
		}
	}
	MYLOG("MESH", "---------------------------------------------");
}

void print_mesh_map_oled(void)
{
#ifdef SHOW_MAP
	if (has_rak1921)
	{
		MYLOG("MESH", "Print map to OLED");
		/** Node ID of the selected receiver node */
		uint32_t node_id[48];
		/** First hop ID of the selected receiver node */
		uint32_t first_hop[48];
		/** Number of hops to the selected receiver node */
		uint8_t num_hops[48];
		/** Number of nodes in the map */
		uint8_t num_elements;

		/** Number of nodes in the map */
		num_elements = nodes_in_map();

		for (int idx = 0; idx < num_elements; idx++)
		{
			get_node(idx, node_id[idx], first_hop[idx], num_hops[idx]);
		}

		// Display the nodes
		rak1921_clear();

		sprintf(line_str, "Node %08lX - B %.1fV", g_this_device_addr, api.system.bat.get());
		rak1921_write_header(line_str);

		if (num_elements > 10)
		{
			num_elements = 10;
		}

		int16_t line = 0;
		for (int idx = 0; idx < num_elements; idx += 2)
		{
			// sprintf(line_str, "%08lX %s", node_id[idx], (first_hop[idx] == 0) ? "d" : "h");

			if ((idx + 1) < num_elements)
			{
				// sprintf(line_str, "%08lX %s - %08lX %s", node_id[idx], (first_hop[idx] == 0) ? "<" : ">",
				// 		node_id[idx + 1], (first_hop[idx + 1] == 0) ? "<" : ">");
				sprintf(line_str, "%08lX %s", node_id[idx], (first_hop[idx] == 0) ? "^" : ">");
				rak1921_write_line(line, 0, line_str);
				sprintf(line_str, "%08lX %s", node_id[idx + 1], (first_hop[idx + 1] == 0) ? "^" : ">");
				rak1921_write_line(line, 64, line_str);
			}
			else
			{
				sprintf(line_str, "%08lX %s", node_id[idx], (first_hop[idx] == 0) ? "^" : ">");
				rak1921_write_line(line, 0, line_str);
			}
			line++;
		}
		rak1921_display();
	}
#endif
}

/**
 * @brief Enqueue data to be send over the Mesh Network
 *
 * @param is_broadcast if true, send as broadcast
 * @param target_addr used for direct message
 * @param tx_data pointer to data packet
 * @param data_size size of data packet
 * @return true packet is enqueued for sending
 * @return false
 */
bool send_to_mesh(bool is_broadcast, uint32_t target_addr, uint8_t *tx_data, uint8_t data_size)
{
	char data_buffer[512];
	// Check if data fits into buffer
	if (data_size > 243)
	{
		return false;
	}

	// Check if it is a broadcast
	if (is_broadcast)
	{
		// Send a broadcast
		// Prepare data
		out_data_buffer.mark1 = 'L';
		out_data_buffer.mark2 = 'o';
		out_data_buffer.mark3 = 'R';

		// Setup broadcast
		out_data_buffer.dest = get_next_broadcast_id();
		out_data_buffer.from = g_this_device_addr;
		out_data_buffer.type = LORA_BROADCAST;
		// MYLOG("MESH", "Queuing broadcast with id %08lX", out_data_buffer.dest);

		memcpy(out_data_buffer.data, tx_data, data_size);

		int dataLen = DATA_HEADER_SIZE + data_size;

		// Add package to send queue
		if (!add_send_request(&out_data_buffer, dataLen))
		{
			MYLOG("MESH", "Sending package failed");
		}
	}
	else // direct message
	{
		// Prepare data
		out_data_buffer.mark1 = 'L';
		out_data_buffer.mark2 = 'o';
		out_data_buffer.mark3 = 'R';

		// Prepare direct message
		out_data_buffer.dest = target_addr;
		out_data_buffer.from = g_this_device_addr;
		out_data_buffer.type = LORA_DIRECT;

		memcpy(out_data_buffer.data, tx_data, data_size);

		int dataLen = DATA_HEADER_SIZE + data_size;

		// Add package to send queue
		if (!add_send_request(&out_data_buffer, dataLen))
		{
			MYLOG("MESH", "Sending package failed");
			return false;
		}
		// else
		// {
		// 	MYLOG("MESH", "Queued msg direct to %08lX", out_data_buffer.dest);
		// }
	}
	return true;
}

/**
 * @brief Send a map sync request as a broadcast message
 *
 * @return true if enqueued
 * @return false if queue is full
 */
bool send_map_request()
{
	// Send a broadcast
	// Prepare data
	out_data_buffer.mark1 = 'L';
	out_data_buffer.mark2 = 'o';
	out_data_buffer.mark3 = 'R';

	// Setup broadcast
	out_data_buffer.dest = get_next_broadcast_id();
	out_data_buffer.from = g_this_device_addr;
	out_data_buffer.type = LORA_MAP_REQ;
	// MYLOG("MESH", "Queuing map request with id %08lX", out_data_buffer.dest);

	int dataLen = DATA_HEADER_SIZE;

	// Add package to send queue
	if (!add_send_request(&out_data_buffer, dataLen))
	{
		MYLOG("MESH", "Sending package failed");
		return false;
	}
	return true;
}