#include <stdlib.h>
#include <stdint.h>
#include <stdio.h> /* For printf() */
#include <string.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"

#include "packet.h"
#include "enum.h"

/*---------------------------------------------------------------------------*/
PROCESS(node_routing_check_process, "Routing check process for node of the network");
PROCESS(node_routing_send_process, "Routing send process for node of the network");
PROCESS(node_data_process, "Data sending process for node of the network");
AUTOSTART_PROCESSES(&node_routing_send_process, &node_routing_check_process, &node_data_process);
/*---------------------------------------------------------------------------*/

/* Variables */
uint8_t rank = 0;
uint8_t parent_id[2];
linkaddr_t parent_addr;
uint8_t *my_id;
static struct etimer parent_timer;
uint8_t verbose = 1; /* Activate/deactivate prints when working */

/* Sending configuration */
uint8_t sending_mode = PERIOD;
int send_permissions[NBR_SUBJECTS] = {1, 1}; /* Start by allowing to send all data type */

/* Functions declarations */
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from);
static void process_routing(struct broadcast_conn *c, const linkaddr_t *from);
static void runicast_recv(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqnbr);
static void send_routing_infos();
static void create_data_packet();

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct runicast_callbacks runicast_call = {runicast_recv};
static struct runicast_conn runicast;

/* Processes */
PROCESS_THREAD(node_routing_check_process, ev, data)
{
    static struct etimer et;    /* Timer needed to awake process */

    PROCESS_BEGIN();

    /* After PARENT seconds, consider the parent disconnected */
    etimer_set(&parent_timer, CLOCK_SECOND * PARENT);

    while(1) {
        etimer_set(&et, CLOCK_SECOND);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

        if (etimer_expired(&parent_timer) != 0 && rank != 0) {
            /* Timer expired and the node had a parent */
            etimer_reset(&parent_timer);
            rank = 0;
	    if(verbose == 1)
            	printf("I lost my parent!\n");
        }
    }

    PROCESS_END();	
}

PROCESS_THREAD(node_routing_send_process, ev, data)
{
    static struct etimer broad_delay;

    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    PROCESS_BEGIN();

    broadcast_open(&broadcast, BROADCAST_CHANNEL, &broadcast_call);
    etimer_set(&broad_delay, CLOCK_SECOND * SEND_ROUTING); /* Timer for sending routing infos */

    my_id = linkaddr_node_addr.u8; /* Get the node address */

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&broad_delay));
        
        if (rank != 0) { 
            /* The node has a parent */
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
    
    runicast_open(&runicast, RUNICAST_CHANNEL, &runicast_call);
    etimer_set(&et, CLOCK_SECOND * SEND_DATA);

    while(1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
        if (rank != 0) { 
        /* The node has a parent */
            if (sending_mode == PERIOD) {
                /* Data are sent periodically */
                create_data_packet();
            
            } else if (sending_mode == CHANGE) { 
                /* Data are sent when a change occurs */
                uint8_t change = rand() % 18; /* Randomly pick whether a change occurs */
                if (change % 2 == 0) {
                    create_data_packet();
                }
            } /* If we get here, the node is mute, don't send */
        }
        etimer_reset(&et);
    }

    PROCESS_END();	
}

/* Functions bodies */
static void send_routing_infos() 
{
    uint8_t type = ROUTING;
    routing_packet_t p = {type, rank}; /* Create routing packet */

    packetbuf_copyfrom(&p, sizeof(routing_packet_t));
    broadcast_send(&broadcast);
    if(verbose == 1)
    	printf("Broadcast routing message sent\n"); /* Send routing infos */
}

static void create_data_packet() 
{
    uint8_t type_packet = DATA;
    uint8_t type_data = (rand() % NBR_SUBJECTS) + 2; /* Randomly pick the data type */
    uint16_t value = rand(); /* Randomly create a value */

    data_packet_t p = {type_packet, type_data, my_id[0], my_id[1], value};
    packetbuf_copyfrom(&p, sizeof(data_packet_t));

    if (send_permissions[type_data-1] == 1) { 
        /* Send only if allowed */
        runicast_send(&runicast, &parent_addr, 0);
	if(verbose == 1)
        	printf("Unicast message send\n");
    } else if(verbose == 1){
        printf("Did not send unicast message, subject was muted\n");
    }
}

static void process_routing(struct broadcast_conn *c, const linkaddr_t *from)
{
    routing_packet_t p;
    memcpy(&p, packetbuf_dataptr(), sizeof(routing_packet_t));
    
    if(verbose == 1){
	printf("Broadcast message of type %d received from %d.%d at rank %d\n",  
        p.message_type,
        from->u8[0], 
        from->u8[1], 
        p.rank);
    printf("My rank is %d\n", rank);
    }
        
    if (rank == 0 || p.rank + 1 < rank) { 
        /* A parent has to be found */
        /* Update rank and save parent ID */
        rank = p.rank + 1;
        parent_id[0] = from->u8[0];
        parent_id[1] = from->u8[1];
        memcpy(&parent_addr, from, sizeof(linkaddr_t));

        if(verbose == 1)
        	printf("My rank is now %d\n", rank);

        /* Broadcast information about the new parent and reset the timer */
        send_routing_infos();
        etimer_restart(&parent_timer);

    } else if (p.rank + 1 == rank && parent_id[0] == from->u8[0] && parent_id[1] == from->u8[1]) { 
        /* This is the node's parent */
        etimer_restart(&parent_timer);
    }
}

static void process_cmd(struct broadcast_conn *c, const linkaddr_t *from)
{
    command_packet_t p;
    memcpy(&p, packetbuf_dataptr(), sizeof(command_packet_t));
    if(verbose == 1)
    	printf("Broadcast command message received from %d.%d type : %d, value : %d (extra value : %d)\n",
        from->u8[0], 
        from->u8[1],
        p.command_type, 
        p.command_value,
	p.command_value_extra);
    int changed = 0;
    switch(p.command_type) { //Process command
        case SENDING:
	    if(sending_mode != p.command_value){
            	sending_mode = p.command_value;
		if(verbose == 1)
	    		printf("Sending mode changed to %d\n", sending_mode);
		changed = 1;
	    }
	    break;
        case MUTE_SUBJECT:
	    if(send_permissions[p.command_value - 1] != 0 && p.command_value_extra == my_id[0]){
            	send_permissions[p.command_value - 1] = 0;
		if(verbose == 1)
	    		printf("Muted subject %d\n", p.command_value);
		changed = 1;
	    }
	    break;
        case UNMUTE_SUBJECT:
            if(send_permissions[p.command_value - 1] != 1 && p.command_value_extra == my_id[0]){
            	send_permissions[p.command_value - 1] = 1;
	    	if(verbose == 1)
	    		printf("Unmuted subject %d\n", p.command_value);
		changed = 1;
	    }
	    break;
    }
    if(changed == 1){
    	packetbuf_copyfrom(&p, sizeof(command_packet_t)); //Broadcast command
    	broadcast_send(&broadcast);
    }
}

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
    uint8_t type;
    memcpy(&type, packetbuf_dataptr(), sizeof(uint8_t));

    switch(type) {
        case ROUTING:
            process_routing(c, from);
            break;
        case CMD:
            process_cmd(c, from);
            break;
    }	
    
}

static void runicast_recv(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqnbr)
{
    data_packet_t p;
    memcpy(&p, packetbuf_dataptr(), sizeof(data_packet_t));
    if(verbose == 1)
	printf("Unicast message of type %d received from %d.%d saying %d\n",  
        p.message_type,
        p.id1_sender, 
        p.id2_sender, 
        p.sensor_data);

    if (rank != 0) {
        /* The node has a parent */
        packetbuf_copyfrom(&p, sizeof(data_packet_t)); /* Need to pass message along */
        runicast_send(&runicast, &parent_addr, 0);
        if(verbose == 1)
	    	printf("Unicast message passed along\n");
    } else {
        /* The node has no parent */
        printf("ERROR: Got a unicast message but have no parent to give it to\n");
    }
}
