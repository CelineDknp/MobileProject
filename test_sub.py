import paho.mqtt.client as mqtt
import serial as serial_dump 
from threading import Thread 
import sys 
import os 

def on_connect(client, userdata, flags, rc):
    print("connected with the result code "+str(rc))
    client.subscribe("$SYS/broker/log/M/subscribe/#")
    client.subscribe("$SYS/broker/log/M/unsubscribe/#")

def on_message(client, userdata, msg):
    if msg.topic == "$SYS/broker/log/M/subscribe":
        sub = str(msg.payload).split(" ")[3]
        sub = sub[0:len(sub)-1] # rm '
        print(sub)
    elif msg.topic == "$SYS/broker/log/M/unsubscribe":
        unsub = str(msg.payload).split(" ")[2]
        unsub = unsub[0:len(unsub)-1]
        print(unsub)
    else:
        print("unknown subscription")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("127.0.0.1")

client.loop_forever()