#ifndef PACKET_H
#define PACKET_H

enum PACKET_TYPE {
	ROUTING = 1,    /* Routing messages to create the tree */
	DATA = 2,       /* Data messages that go towards the root */
	CMD = 3         /* Command messages from the gateway */
};

typedef struct routing_packet {
	uint8_t message_type;
	uint8_t rank;
} routing_packet_t;

typedef struct data_packet {
	uint8_t message_type;
	uint8_t data_type;
	uint8_t id1_sender;
	uint8_t id2_sender;
	uint16_t sensor_data;
} data_packet_t;

#endif /* PACKET_H */
