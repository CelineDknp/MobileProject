#include "contiki.h"
#include <stdio.h> /* For printf() */
 
//-----------------------------------------------------------------   
PROCESS(timer_process, "timer with print example");
AUTOSTART_PROCESSES(&timer_process);
//-----------------------------------------------------------------
 
 
PROCESS_THREAD(timer_process, ev, data)
{   
   PROCESS_EXITHANDLER();
   PROCESS_BEGIN();
 
  /* Initialize stuff here. */ 
 
	printf("\n++++++++++++++++++++++++++++++\n");
	printf("+    		HELLO WORDL		   +\n");
	printf("++++++++++++++++++++++++++++++\n");
 
    while(1) {
		/* Do the rest of the stuff here. */
		static uint32_t seconds = 5;
		static struct etimer et; // Define the timer
		etimer_set(&et, CLOCK_SECOND*seconds);  // Set the timer
	 
		PROCESS_WAIT_EVENT();  // Waiting for a event, don't care which
	 
		if(etimer_expired(&et)) {  // If the event it's provoked by the timer expiration, then...
			printf("Timer has expired !");
		}
	}	
	
	PROCESS_END();
}
