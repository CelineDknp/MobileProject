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

typedef struct test_packet{
	uint8_t message_type;
	uint8_t rank;
} test_packet;
 
//-----------------------------------------------------------------   
PROCESS(timer_process, "timer with print example");
AUTOSTART_PROCESSES(&timer_process);
//-----------------------------------------------------------------
//Variables
uint8_t rank = 0;
uint8_t parent_id[2];
// Functions
static void broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  test_packet p;
  memcpy(&p, packetbuf_dataptr(), sizeof(test_packet));
  printf("broadcast message of type %d received from %d.%d at rank %d\n",  p.rank,
         from->u8[0], from->u8[1], p.rank);
  printf("my rank is %d\n", rank);
  if(rank == 0 || p.rank+1 < rank){ //if my rank is 0, I need a parent
	rank = p.rank+1;
	parent_id[0] = from->u8[0];
	parent_id[1] = from->u8[1];
	printf("I found a new parent !\n");
  	printf("my rank is now %d\n", rank);
  }
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
 
PROCESS_THREAD(timer_process, ev, data)
{   
   static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    uint8_t type = 1;
    if(rank != 0){
    test_packet p = {type, rank};

    packetbuf_copyfrom(&p, sizeof(test_packet));
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");
    }
  }

  PROCESS_END();	
}
