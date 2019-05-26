#include <stdlib.h>
#include <stdint.h>
#include <stdio.h> /* For printf() */
#include <string.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/serial-line.h"
#include "random.h"

#include "packet.h"
#include "enum.h"

/*---------------------------------------------------------------------------*/   
PROCESS(broadcast_process, "Process to send routing infos");
PROCESS(test_serial, "Serial line test process");
AUTOSTART_PROCESSES(&broadcast_process, &test_serial);
/*---------------------------------------------------------------------------*/
/* Variables */
uint8_t rank = 0; /* Root's rank is always 0 and it has no parent */
uint8_t verbose = 1; /* Activate/deactivate prints when working */

/* Functions declarations */
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);
static void runicast_recv(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqnbr);
static void send_cmd(uint8_t type_cmd, uint8_t value, uint8_t value_extra);

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct runicast_callbacks runicast_call = {runicast_recv};
static struct runicast_conn runicast;

PROCESS_THREAD(test_serial, ev, data)
{
    PROCESS_BEGIN();
 
    while (1) {
        PROCESS_YIELD();

        if (ev == serial_line_event_message && data != NULL) {
            if (verbose == 1) {
                printf("Input received: %s\n", (char *) data);
            }
            char i = ((char *) data)[0];
            
            if (i == '0') { /* Sending modes */
                char i = ((char *) data)[2];
                if (i == '1') /* noSend */
                    send_cmd(SENDING, MUTE, 0);
                else if (i == '2') /* periodically */
                    send_cmd(SENDING, PERIOD, 0);
                else if (i == '3') /* onChange */
                    send_cmd(SENDING, CHANGE, 0);
                else
                    printf("[ERROR] Unknown input from gateway\n");

            } else if (i == '1') { /* Unmute topics */
                char node = ((char *) data)[2];
                char subject = ((char *) data)[4];
                int i_node = node - '0';
                int i_subject = subject - '0';
                send_cmd(UNMUTE_SUBJECT, i_subject, i_node);

            } else if (i == '2') { /* Mute topics */
                char node = ((char *) data)[2];
                char subject = ((char *) data)[4];
                int i_node = node - '0';
                int i_subject = subject - '0';
                send_cmd(MUTE_SUBJECT, i_subject, i_node);

            } else {
                printf("[ERROR] Unknown input from gateway\n");
            }
        }
    }
    PROCESS_END();
}
 
PROCESS_THREAD(broadcast_process, ev, data)
{   
    static struct etimer et;

    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

    PROCESS_BEGIN();

    broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
    runicast_open(&runicast, RUNICAST_CHANNEL, &runicast_call);

    while(1) {
        /* Delay 2-4 seconds */
        etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        
        uint8_t type = 1;
        routing_packet_t p = {type, rank};

        packetbuf_copyfrom(&p, sizeof(routing_packet_t));
        broadcast_send(&broadcast);

        if (verbose == 1) {
            printf("Broadcast message sent\n");
        }
    }

    PROCESS_END();	
}

/* Functions bodies */
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    routing_packet_t p;
    memcpy(&p, packetbuf_dataptr(), sizeof(routing_packet_t));

    if (verbose == 1) {
        printf("Broadcast message of type %d received from %d.%d at rank %d\n",
            p.message_type,
            from->u8[0], 
            from->u8[1], 
            p.rank);
        printf("Ignored, I am root\n");
    }
}

static void runicast_recv(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqnbr)
{
    data_packet_t p;
    memcpy(&p, packetbuf_dataptr(), sizeof(data_packet_t));

    if (verbose == 1) {
        printf("Unicast message of type %d received from %d.%d saying %d\n",  
            p.message_type,
            p.id1_sender, 
            p.id2_sender, 
            p.sensor_data);
    }

    switch (p.data_type) {
        case TEMP:
            printf("%d.%d-%s-%d\n", p.id1_sender, p.id2_sender, "temperature", p.sensor_data);
            break;
        case HUMIDITY:
            printf("%d.%d-%s-%d\n", p.id1_sender, p.id2_sender, "humidity", p.sensor_data);
            break;
        default:
            printf("[ERROR] Unknown data packet\n");
            break;
    }
}

static void send_cmd(uint8_t type_cmd, uint8_t value, uint8_t value_extra)
{
    uint8_t type = CMD;
    command_packet_t p = {type, type_cmd, value, value_extra}; /* Create command packet */

    packetbuf_copyfrom(&p, sizeof(command_packet_t));
    broadcast_send(&broadcast);

    if (verbose == 1) {
        printf("Command command message sent\n"); /* Send routing infos */
    }
}
