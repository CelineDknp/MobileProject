#ifndef PACKET_H
#define PACKET_H

typedef struct routing_packet {
    uint8_t message_type;
    uint8_t rank;
} routing_packet_t;

typedef struct command_packet {
    uint8_t message_type;
    uint8_t command_type;
    uint8_t command_value;
} command_packet_t;

typedef struct data_packet {
    uint8_t message_type;
    uint8_t data_type;
    uint8_t id1_sender;
    uint8_t id2_sender;
    uint16_t sensor_data;
} data_packet_t;

#endif /* PACKET_H */
