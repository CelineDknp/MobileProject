CONTIKI_PROJECT = node root
all: $(CONTIKI_PROJECT)

#UIP_CONF_IPV6=1

CONTIKI = /home/user/contiki
CONTIKI_WITH_RIME = 1
include $(CONTIKI)/Makefile.include
