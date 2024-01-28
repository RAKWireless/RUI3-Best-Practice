/**
 * @file mesh.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Definitions and globals for Mesh network
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */
/*********************************************/
/* Could move to WisBlock-API-V2             */
/*********************************************/

#pragma pack(push, 1)
struct map_msg_s
{
	uint8_t mark1 = 'L'; // 1
	uint8_t mark2 = 'o'; // 2
	uint8_t mark3 = 'R'; // 3
	uint8_t type = 5;	 // 4
	uint32_t dest = 0;	 // 5,6,7,8
	uint32_t from = 0;	 // 9,10,11,12
	uint8_t nodes[48][5];
};
#pragma pack(pop)
#pragma pack(push, 1)
struct data_msg_s
{
	uint8_t mark1 = 'L'; // 1
	uint8_t mark2 = 'o'; // 2
	uint8_t mark3 = 'R'; // 3
	uint8_t type = 0;	 // 4
	uint32_t dest = 0;	 // 5,6,7,8
	uint32_t from = 0;	 // 9,10,11,12
	uint32_t orig = 0;	 // 13,14,15,16
	uint8_t data[243];	 // 17
};
#pragma pack(pop)
#pragma pack(push, 1)
struct rx_msg_s
{
	uint8_t rx_size;
	int8_t rx_snr;
	int16_t rx_rssi;
	uint8_t rx_buffer[256];
};
#pragma pack(pop)

/**
 * Mesh callback functions
 */
typedef struct
{
	/**
	 * Data available callback prototype.
	 *
	 * @param from_id
	 * 			Node address of the sender
	 * @param payload
	 * 			Received buffer pointer
	 * @param size
	 * 			Received buffer size
	 * @param rssi
	 * 			RSSI value computed while receiving the frame [dBm]
	 * @param snr
	 * 			SNR value computed while receiving the frame [dB]
	 *                     FSK : N/A ( set to 0 )
	 *                     LoRa: SNR value in dB
	 */
	void (*data_avail_cb)(uint32_t from_id, uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

	/**
	 * Nodes list change callback prototype.
	 */
	void (*map_changed_cb)(void);

} mesh_events_s;

/** Time interval for handling mesh events */
#define EVENT_HANDLER_TIME 1000

/** Size of map message buffer without subnode */
#define MAP_HEADER_SIZE 12
/** Size of data message buffer without subnode */
#define DATA_HEADER_SIZE 16

/** LoRa package types */
#define LORA_INVALID 0
#define LORA_DIRECT 1
#define LORA_FORWARD 2
#define LORA_BROADCAST 3
#define LORA_NODEMAP 4
#define LORA_MAP_REQ 5

// LoRa Mesh functions & variables
void init_mesh(mesh_events_s *events);
void mesh_task(void *pvParameters);
void mesh_check_rx(void);
void mesh_check_tx(void);
bool add_send_request(data_msg_s *package, uint8_t msg_size);
bool add_rx_packet(int16_t rssi, int8_t snr, uint8_t size, uint8_t *buffer);
void print_mesh_map(void);
bool send_to_mesh(bool is_broadcast, uint32_t target_addr, uint8_t *tx_data, uint8_t data_size);
bool send_map_request();
extern mesh_events_s g_mesh_events;

/** Wake up events for Mesh Task */
#define NO_EVENT 0
#define NODE_CHANGE   0b00000001
#define N_NODE_CHANGE 0b11111110
#define SYNC_MAP      0b00000010
#define N_SYNC_MAP    0b11111101
#define CHECK_QUEUE   0b00000100
#define N_CHECK_QUEUE 0b11111011
#define CHECK_RX      0b00001000
#define N_CHECK_RX    0b11110111

struct g_nodes_list_s
{
	uint32_t node_id;
	uint32_t first_hop;
	time_t time_stamp;
	uint8_t num_hops;
};

// Mesh Router
bool get_route(uint32_t id, g_nodes_list_s *route);
boolean add_node(uint32_t id, uint32_t hop, uint8_t num_hops);
void clear_subs(uint32_t id);
bool clean_map(void);
uint8_t node_map(uint32_t subs[], uint8_t hops[]);
uint8_t node_map(uint8_t nodes[][5]);
uint8_t nodes_in_map();
bool get_node(uint8_t node_num, uint32_t &nodeId, uint32_t &firstHop, uint8_t &numHops);
uint32_t get_node_addr(uint8_t node_num);
uint32_t get_next_broadcast_id(void);
bool is_old_broadcast(uint32_t g_broadcast_id);
bool check_node(uint32_t node_addr);

extern g_nodes_list_s *g_nodes_map;
extern int g_num_of_nodes;
extern uint32_t g_this_device_addr;
