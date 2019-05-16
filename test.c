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


#define NBR_SUBJECTS 2

enum PACKET_TYPE {
	ROUTING = 1, //Routing messages to create the tree
	DATA = 2, //Data messages that go towards the root
	CMD = 3 //Command messages from the gateway
};

enum SENDING_MODE { //Mode of sending the data
	PERIOD = 1, //Send every X seconds
	CHANGE = 2 //Send only when there is a change
};

enum SUBJECTS {
	TEMP = 1,
	HUMIDITY = 2
};

enum TIMINGS {
	PARENT = 15, //Time after which parent is considered disconnected
	SEND_ROUTING = 5, //Time frame to send keep alive messages
	SEND_DATA = 10, //Time frame to send data periodically
};

typedef struct routing_packet{
	uint8_t message_type;
	uint8_t rank;
} routing_packet;

typedef struct data_packet{
	uint8_t message_type;
	uint8_t data_type;
	uint8_t id1_sender;
	uint8_t id2_sender;
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
uint8_t *my_id;
//Sending configuration
uint8_t sending_mode = PERIOD; //Start of with sending mode periodically
int send_permissions[NBR_SUBJECTS] = {1, 1}; //Strat by allowing to send all data type

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
	static struct etimer et; //Timer needed to awake process
  	PROCESS_BEGIN();
    	etimer_set(&parent_timer, CLOCK_SECOND * PARENT); //After PARENT seconds, we will consider the parent disconnected

  	while(1) {
		etimer_set(&et, CLOCK_SECOND);
    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    		
    		if(etimer_expired(&parent_timer) != 0 && rank != 0){ //If timer has expired and I had a parent
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
    	etimer_set(&broad_delay, CLOCK_SECOND * SEND_ROUTING); //Timer for sending routing infos
	
	my_id = rimeaddr_node_addr.u8; //Get my address
	printf("My address is : %d.%d\n", my_id[0], my_id[1]);

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
	etimer_set(&et, CLOCK_SECOND * SEND_DATA);

  	while(1) {
 	   	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    	
 	   	if(rank != 0){ //If rank != 0, has a parent
			if(sending_mode == PERIOD){// If  sending data periodically, send
				create_data_packet();
			}
			else if(sending_mode == CHANGE){ //If we should only send when there is a change
				uint8_t change = rand() % 18; //Randomly pick if there is a change
				if(change % 2 == 0){
					create_data_packet();
				}
			}
 	   	}
		etimer_reset(&et);
  	}
  	PROCESS_END();	
}

// Functions
static void send_routing_infos(){
	uint8_t type = ROUTING;
    	routing_packet p = {type, rank}; //Create routing packet

    	packetbuf_copyfrom(&p, sizeof(routing_packet));
    	broadcast_send(&broadcast);
    	printf("broadcast routing message sent\n"); //Send routing infos
}

static void create_data_packet(){
	uint8_t type_packet = DATA;
	uint8_t type_data = (rand() % NBR_SUBJECTS) + 2; //Randomly pick a data type
	uint16_t value = rand(); //Randomy create a value
	printf("Decided on subject %d\n", type_data);

	data_packet p = {type_packet, type_data, my_id[0], my_id[1], value};
	packetbuf_copyfrom(&p, sizeof(data_packet));
	if(send_permissions[type_data-1] == 1){ //Send only if allowed
		runicast_send(&runicast, &parent_addr, 0);
		printf("unicast message send\n");
	}else{
		printf("didn't send unicast message, subject was muted\n");
	}
}

static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	routing_packet p;
	memcpy(&p, packetbuf_dataptr(), sizeof(routing_packet));
	
	printf("broadcast message of type %d received from %d.%d at rank %d\n",  p.message_type,
         from->u8[0], from->u8[1], p.rank);
	printf("my rank is %d\n", rank);
    	
	if(rank == 0 || p.rank+1 < rank){ //if my rank is 0, I need a parent
		rank = p.rank+1; //Update my rank
		parent_id[0] = from->u8[0]; //Save parent infos
		parent_id[1] = from->u8[1];
		memcpy(&parent_addr, from, sizeof(rimeaddr_t));
		
		printf("I found a new parent !\n");
  		printf("my rank is now %d\n", rank);
		
		send_routing_infos(); //Found a new parent, broadcast it !
		etimer_restart(&parent_timer); //Found a parent, reset the timer
  	}
	else if(p.rank+1 == rank && parent_id[0] == from->u8[0] && parent_id[1] == from->u8[1]){ //If it was my parent
		etimer_restart(&parent_timer); //Reset the timer
	}
}
static void runicast_recv(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqnbr)
{
	data_packet p;
	memcpy(&p, packetbuf_dataptr(), sizeof(data_packet));
	printf("unicast message of type %d received from %d.%d saying %d\n",  p.message_type,
         p.id1_sender, p.id2_sender, p.sensor_data);
	if(rank != 0){ //If I have a parent
		packetbuf_copyfrom(&p, sizeof(data_packet)); //Need to pass message along
		runicast_send(&runicast, &parent_addr, 0);
		printf("unicast message passed along\n");
	}else{
		printf("ERROR : Got a unicast message but have no parent to give it to\n");
	}
}
