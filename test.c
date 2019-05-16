#include "contiki.h"
#include <stdio.h> /* For printf() */
#include <stdlib.h>
#include <stdint.h>
#include "net/rime.h"
#include "net/netstack.h"
#include "net/rime/rimeaddr.h"
#include "net/rime/chameleon.h"
#include "net/rime/route.h"
#include "net/rime/announcement.h"
#include "net/rime/broadcast-announcement.h"
#include "net/mac/mac.h"
//#include "dev/sht11.h"
#include "dev/light-ziglet.h"
#include "random.h"

enum PACKET_TYPE {
	ROUTING = 1,
	DATA = 2
};

typedef struct routing_packet{
	uint8_t message_type;
	uint8_t rank;
} routing_packet;

typedef struct data_packet{
	uint8_t message_type;
	uint8_t data_type;
	uint16_t sensor_data;
} data_packet;
 
//-----------------------------------------------------------------   
PROCESS(node_routing_check_process, "Routing check process for node of the network");
PROCESS(node_routing_send_process, "Routing send process for node of the network");
PROCESS(node_data_process, "Data sending process for node of the network");
AUTOSTART_PROCESSES(&node_routing_send_process, &node_routing_check_process, &node_data_process);
//-----------------------------------------------------------------
//Variables
uint8_t rank = 0;
uint8_t parent_id[2];
rimeaddr_t parent_addr;
static struct etimer parent_timer;

//Functions
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from);
static void runicast_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqnbr);
static void send_routing_infos();
static void create_data_packet();

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct runicast_callbacks runicast_call = {runicast_recv};
static struct runicast_conn runicast;

//Processes
PROCESS_THREAD(node_routing_check_process, ev, data)
{
	static struct etimer et;
  	PROCESS_BEGIN();
	
	
	/* Delay 6 secondes */
    	etimer_set(&parent_timer, CLOCK_SECOND * 15);

  	while(1) {

		etimer_set(&et, CLOCK_SECOND);
    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    		
    		if(etimer_expired(&parent_timer) != 0 && rank != 0){ //If timer has expired
			etimer_reset(&parent_timer);
			rank = 0;
			printf("I lost my parent !\n");
		}	
  	}
  	PROCESS_END();	
}

PROCESS_THREAD(node_routing_send_process, ev, data)
{
	static struct etimer broad_delay;
  	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
  	PROCESS_BEGIN();
	
  	broadcast_open(&broadcast, 129, &broadcast_call);
	
	/* Delay 3 secondes */
    	etimer_set(&broad_delay, CLOCK_SECOND * 3);

  	while(1) {

    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&broad_delay));
    	
    		if(rank != 0){ //If we have a parent
			send_routing_infos();
		}
		etimer_reset(&broad_delay);
  	}
  	PROCESS_END();	
}

PROCESS_THREAD(node_data_process, ev, data)
{   
	static struct etimer et;

  	PROCESS_EXITHANDLER(runicast_close(&runicast);)
  	PROCESS_BEGIN();
  	runicast_open(&runicast, 144, &runicast_call);

  	while(1) {

    	/* Delay 2-4 seconds */
    	etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

 	   	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    	
 	   	if(rank != 0){ //If rank != 0, has a parent and can send data
			create_data_packet();
 	   	}
  	}
  	PROCESS_END();	
}

// Functions
static void send_routing_infos(){
	uint8_t type = ROUTING;
    routing_packet p = {type, rank};

    packetbuf_copyfrom(&p, sizeof(routing_packet));
    broadcast_send(&broadcast);
    printf("broadcast routing message sent\n"); //Send routing infos
}

static void create_data_packet(){
	uint8_t type_packet = DATA;
	uint8_t type_data = (rand() % (2 - 1 + 1)) + 1; //Randomly pick a data type
	uint16_t value = rand(); //Randomy create a value

	data_packet p = {type_packet, type_data, value};
	packetbuf_copyfrom(&p, sizeof(data_packet));
	runicast_send(&runicast, &parent_addr, 0);
	printf("unicast message send\n"); 
}

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	routing_packet p;
	memcpy(&p, packetbuf_dataptr(), sizeof(routing_packet));
	printf("broadcast message of type %d received from %d.%d at rank %d\n",  p.message_type,
         from->u8[0], from->u8[1], p.rank);
	printf("my rank is %d\n", rank);
    etimer_restart(&parent_timer);
	if(rank == 0 || p.rank+1 < rank){ //if my rank is 0, I need a parent
		rank = p.rank+1;
		parent_id[0] = from->u8[0];
		parent_id[1] = from->u8[1];
		memcpy(&parent_addr, from, sizeof(rimeaddr_t));
		printf("I found a new parent !\n");
  		printf("my rank is now %d\n", rank);
		
		//Found a new parent, broadcast it !
		send_routing_infos();
  	}
}
static void runicast_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqnbr)
{
	data_packet p;
	memcpy(&p, packetbuf_dataptr(), sizeof(data_packet));
	printf("unicast message of type %d received from %d.%d saying %d\n",  p.message_type,
         from->u8[0], from->u8[1], p.sensor_data);
	packetbuf_copyfrom(&p, sizeof(data_packet));
	runicast_send(&runicast, &parent_addr, 0);
	printf("unicast message passed along\n");
}
