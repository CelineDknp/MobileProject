#ifndef ENUM_H
#define ENUM_H
#define NBR_SUBJECTS 2

enum PACKET_TYPE {
	ROUTING = 1,    /* Routing messages to create the tree */
	DATA = 2,       /* Data messages that go towards the root */
	CMD = 3         /* Command messages from the gateway */
};

enum CMD_TYPE {
	SENDING = 1, 	/* Change sending mode */
	MUTE = 2,		/* Mute subject */
	UNMUTE = 3		/* Unmute subject */
};

enum SENDING_MODE {
	PERIOD = 1,	    /* Send every X seconds */
	CHANGE = 2,	    /* Send only when there is a change */
	MUTE = 3
};

enum SUBJECTS {
	TEMP = 1,
	HUMIDITY = 2
};

enum TIMINGS {
	PARENT = 15,        /* Time after which parent is considered disconnected */
	SEND_ROUTING = 5,   /* Time frame to send keep alive messages */
	SEND_DATA = 10,     /* Time frame to send data periodically */
};

#endif /* ENUM_H */