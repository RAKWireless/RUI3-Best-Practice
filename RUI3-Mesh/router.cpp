/**
 * @file router.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief
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

/** The list with all known nodes */
g_nodes_list_s *g_nodes_map;
/** Index to the first free node entry */
int nodes_mapindex = 0;

/** Timeout to remove unresponsive nodes */
time_t in_active_timeout = 120000;

/** ID of received broadcast */
extern uint32_t g_broadcast_id;

/**
 * @brief Delete a node route by copying following routes on top of it.
 *
 * @param index The node to be deleted
 */
void delete_route(uint8_t index)
{
	// Delete a route by copying following routes on top of it
	memcpy(&g_nodes_map[index], &g_nodes_map[index + 1], sizeof(g_nodes_list_s) * (g_num_of_nodes - index - 1));
	nodes_mapindex--;
	g_nodes_map[nodes_mapindex].node_id = 0;
}

/**
 * @brief Find a route to a node
 *
 * @param id Node ID we need a route to
 * @param route g_nodes_list_s struct that will be filled with the route
 * @return true if route was found
 * @return false if no route was found
 */
bool get_route(uint32_t id, g_nodes_list_s *route)
{
	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].node_id == id)
		{
			route->first_hop = g_nodes_map[idx].first_hop;
			route->node_id = g_nodes_map[idx].node_id;
			// Node found in map
			return true;
		}
	}
	// Node not in map
	return false;
}

/**
 * @brief Add a node into the list.
 * 			Checks if the node already exists and
 * 			removes node if the existing entry has more hops
 *
 * @param id node mesh address
 * @param hop next hop node address
 * @param num_hops number of hops to the ndoe
 * @return boolean true if node was added, otherwise false
 */
boolean add_node(uint32_t id, uint32_t hop, uint8_t num_hops)
{
	boolean list_changed = false;
	g_nodes_list_s _new_node;
	_new_node.node_id = id;
	_new_node.first_hop = hop;
	_new_node.time_stamp = millis();
	_new_node.num_hops = num_hops;

	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].node_id == id)
		{
			if (g_nodes_map[idx].first_hop == 0)
			{
				if (hop == 0)
				{ // Node entry exist already as direct, update timestamp
					g_nodes_map[idx].time_stamp = millis();
				}
				MYLOG("ROUT", "Node %08lX already exists as direct", _new_node.node_id);
				return list_changed;
			}
			else
			{
				if (hop == 0)
				{
					// Found the node, but not as direct neighbor
					MYLOG("ROUT", "Node %08lX removed because it was a sub", _new_node.node_id);
					delete_route(idx);
					idx--;
					break;
				}
				else
				{
					// Node entry exists, check number of hops
					if (g_nodes_map[idx].num_hops <= num_hops)
					{
						// Node entry exist with smaller or equal # of hops
						MYLOG("ROUT", "Node %08lX exist with a lower number of hops", _new_node.node_id);
						return list_changed;
					}
					else
					{
						// Found the node, but with higher # of hops
						MYLOG("ROUT", "Node %08lX exist with a higher number of hops", _new_node.node_id);
						delete_route(idx);
						idx--;
						break;
					}
				}
			}
		}
	}

	if (nodes_mapindex == g_num_of_nodes)
	{
		// Map is full, remove the oldest entry
		delete_route(0);
		list_changed = true;
	}

	// New node entry
	memcpy(&g_nodes_map[nodes_mapindex], &_new_node, sizeof(g_nodes_list_s));
	nodes_mapindex++;

	list_changed = true;
	MYLOG("ROUT", "Added node %lX with hop %lX and num hops %d", id, hop, num_hops);
	return list_changed;
}

/**
 * @brief Remove all nodes that are a non direct and have a given node as first hop.
 * 			This is to clean up the nodes list from left overs of an unresponsive node
 *
 * @param id The node which is listed as first hop
 */
void clear_subs(uint32_t id)
{
	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].first_hop == id)
		{
			MYLOG("ROUT", "Removed node %lX with hop %lX", g_nodes_map[idx].node_id, g_nodes_map[idx].first_hop);
			delete_route(idx);
			idx--;
		}
	}
}

/**
 * @brief Check the list for nodes that did not be refreshed within a given timeout
 * 			Checks as well for nodes that have "impossible" number of hops (> number of max nodes)
 *
 * @return true if no changes were done
 * @return false if a node was removed
 */
bool clean_map(void)
{
	// Check active nodes list
	bool mapUpToDate = true;
	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].node_id == 0)
		{
			// Last entry found
			break;
		}

		/// \todo discuss what is best node timeout
		// Assume all nodes are on the same send interval
		in_active_timeout = g_custom_parameters.send_interval * 100;
		if (in_active_timeout == 0)
		{
			in_active_timeout = 3600000; 
		}

		if (((millis() > (g_nodes_map[idx].time_stamp + in_active_timeout))) || (g_nodes_map[idx].num_hops > 48))
		{
			// Node was not refreshed for in_active_timeout milli seconds
			MYLOG("ROUT", "Node %lX with hop %lX timed out or has too many hops", g_nodes_map[idx].node_id, g_nodes_map[idx].first_hop);
			if (g_nodes_map[idx].first_hop == 0)
			{
				clear_subs(g_nodes_map[idx].node_id);
			}
			delete_route(idx);
			idx--;
			mapUpToDate = false;
		}
	}
	return mapUpToDate;
}

/**
 * @brief Create a list of nodes and hops to be broadcasted as this nodes map
 *
 * @param subs Pointer to an array to hold the node IDs
 * @param hops Pointer to an array to hold the hops for the node IDs
 * @return uint8_t Number of nodes in the list
 */
uint8_t node_map(uint32_t subs[], uint8_t hops[])
{
	uint8_t subs_name_index = 0;

	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].node_id == 0)
		{
			// Last node found
			break;
		}
		hops[subs_name_index] = g_nodes_map[idx].num_hops;

		subs[subs_name_index] = g_nodes_map[idx].node_id;
		subs_name_index++;
	}

	return subs_name_index;
}

/**
 * @brief Create a list of nodes and hops to be broadcasted as this nodes map
 *
 * @param nodes Pointer to an two dimensional array to hold the node IDs and hops
 * @return uint8_t Number of nodes in the list
 */
uint8_t node_map(uint8_t nodes[][5])
{
	uint8_t subs_name_index = 0;
	MYLOG("ROUT", "Copy map");
	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].node_id == 0)
		{
			// Last node found
			break;
		}
		nodes[subs_name_index][0] = g_nodes_map[idx].node_id & 0x000000FF;
		nodes[subs_name_index][1] = (g_nodes_map[idx].node_id >> 8) & 0x000000FF;
		nodes[subs_name_index][2] = (g_nodes_map[idx].node_id >> 16) & 0x000000FF;
		nodes[subs_name_index][3] = (g_nodes_map[idx].node_id >> 24) & 0x000000FF;
		nodes[subs_name_index][4] = g_nodes_map[idx].num_hops;

		subs_name_index++;
	}
	MYLOG("ROUT", "Copied %d", subs_name_index);

	return subs_name_index;
}

/**
 * @brief Get number of nodes in the map
 *
 * @return uint8_t Number of nodes
 */
uint8_t nodes_in_map(void)
{
	return nodes_mapindex;
}

/**
 * @brief Get the information of a specific node
 *
 * @param node_num Index of the node we want to query
 * @param node_id Pointer to an uint32_t to save the node ID to
 * @param first_hop Pointer to an uint32_t to save the nodes first hop ID to
 * @param num_hops Pointer to an uint8_t to save the number of hops to
 * @return true if the data could be found
 * @return false if the requested index is out of range
 */
bool get_node(uint8_t node_num, uint32_t &node_id, uint32_t &first_hop, uint8_t &num_hops)
{
	if (node_num >= nodes_in_map())
	{
		return false;
	}

	node_id = g_nodes_map[node_num].node_id;
	first_hop = g_nodes_map[node_num].first_hop;
	num_hops = g_nodes_map[node_num].num_hops;
	return true;
}

/**
 * @brief Get the Node Addr
 *
 * @param node_num Index in the node map
 * @return uint32_t node address or 0x00 if node not found
 */
uint32_t get_node_addr(uint8_t node_num)
{
	if (node_num > nodes_in_map())
	{
		return 0x00;
	}

	return g_nodes_map[node_num].node_id;
}

/**
 * @brief Check if the node address is in the map
 * 
 * @param node_addr node address to check
 * @return true if node is in map
 * @return false if node is not in the map
 */
bool check_node(uint32_t node_addr)
{
	for (int idx = 0; idx < g_num_of_nodes; idx++)
	{
		if (g_nodes_map[idx].node_id == 0)
		{
			// Last node found
			break;
		}
		if (g_nodes_map[idx].node_id== node_addr)
		{
			return true;
		}
	}
	return false;
}

/**
 * @brief Get next broadcast ID
 *
 * @return uint32_t next broadcast ID
 */
uint32_t get_next_broadcast_id(void)
{
	uint32_t broadcast_num = g_broadcast_id & 0x000000FF;
	// Create new one by just counting up
	broadcast_num++;
	broadcast_num &= 0x000000FF;
	g_broadcast_id = (g_broadcast_id & 0xFFFFFF00) | broadcast_num;
	return g_broadcast_id;
}

byte broadcast_index = 0;
#define NUM_OF_LAST_BROADCASTS 10
uint32_t broadcastList[NUM_OF_LAST_BROADCASTS] = {0L};
/**
 * @brief Handle broadcast message ID's
 * 			to avoid circulating same broadcast over and over
 *
 * @param g_broadcast_id The broadcast ID to be checked
 * @return true if the broadcast is an old broadcast
 * @return false if the broadcast is a new broadcast
 */
bool is_old_broadcast(uint32_t g_broadcast_id)
{
	for (int idx = 0; idx < NUM_OF_LAST_BROADCASTS; idx++)
	{
		if (g_broadcast_id == broadcastList[idx])
		{
			// Broadcast ID is already in the list
			return true;
		}
	}
	// This is a new broadcast ID
	broadcastList[broadcast_index] = g_broadcast_id;
	broadcast_index++;
	if (broadcast_index == NUM_OF_LAST_BROADCASTS)
	{
		// Index overflow, reset to 0
		broadcast_index = 0;
	}
	return false;
}