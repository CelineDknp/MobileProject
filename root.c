#include <stdlib.h>
#include <stdint.h>
#include <stdio.h> /* For printf() */
#include <string.h>
#include "contiki.h"
#include "net/netstack.h"
#include "net/rime.h"
#include "random.h"

#include "packet.h"
#include "enum.h"

/*---------------------------------------------------------------------------*/   
PROCESS(broadcast_process, "Process to send routing infos");
AUTOSTART_PROCESSES(&broadcast_process);
/*---------------------------------------------------------------------------*/

/* Variables */
uint8_t rank = 0; /* Root's rank is always 0 and it has no parent */

/* Functions declarations */
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from);
static void runicast_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqnbr);
static void send_cmd(uint8_t type_cmd, uint8_t value);

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct runicast_callbacks runicast_call = {runicast_recv};
static struct runicast_conn runicast;
 
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
    	printf("Broadcast message sent\n");
  	}

	PROCESS_END();	
}

/* Functions bodies */
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	routing_packet_t p;
	memcpy(&p, packetbuf_dataptr(), sizeof(routing_packet_t));
	printf("Broadcast message of type %d received from %d.%d at rank %d\n",
		p.message_type,
        from->u8[0], 
		from->u8[1], 
		p.rank);
	printf("Ignored, I am root\n");
}

static void runicast_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqnbr)
{
	data_packet_t p;
	memcpy(&p, packetbuf_dataptr(), sizeof(data_packet_t));
	printf("Unicast message of type %d received from %d.%d saying %d\n",  
		p.message_type,
        p.id1_sender, 
		p.id2_sender, 
		p.sensor_data);
	printf("I am root, I need to give it to the gateway!\n");
}

static void send_cmd(uint8_t type_cmd, uint8_t value)
{
	uint8_t type = CMD;
    routing_packet_t p = {type, type_cmd, value}; /* Create routing packet */

    packetbuf_copyfrom(&p, sizeof(command_packet_t));
    broadcast_send(&broadcast);
    printf("Broadcast command message sent\n"); /* Send routing infos */
}
