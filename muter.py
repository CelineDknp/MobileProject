import paho.mqtt.client as mqtt
import serial as serial_dump 
from threading import Thread 
import sys 
from time import sleep
import os

l = []

class Muter(Thread):
    def __init__(self, serial, server, nb_motes, verbose):
        def init_dico():
            """
            Initialize dictionary of subscriptions for each mote and topic
            """
            mote_dict = {}
            for i in range(2,nb_motes+1):
                mote_dict[i] = {
                    "temperature": 0,
                    "humidity": 0
                }
            return mote_dict
        Thread.__init__(self)
        self.serial = serial
        self.verbose = verbose
        self.flag = True
        self.server = server
        self.client = mqtt.Client()
        self.motes = init_dico()

    def handle_data(self, content, sub):
        def unmute(mote, topic):
            """
            Send an unmute message to the serial
            """
            if self.verbose:
                print("unmuting " + str(mote) + " " + topic)
            #print("1/{}/{}".format(mote,'0' if topic == "temperature" else '1'))
            self.serial.write("1/{}/{}\n".format(mote,0 if topic == "temperature" else 1).encode())
        
        def mute(mote, topic):
            """
            Send a mute message
            """
            if self.verbose:
                print("muting " + str(mote) + " " + topic)
            #print("2/{}/{}".format(str(mote),'0' if topic == "temperature" else '1'))
            self.serial.write("2/{}/{}\n".format(mote,0 if topic == "temperature" else 1).encode())

        if sub: # received a subscription
            if content[0] == "#": # all motes and topics
                if self.verbose:
                    print("Subscription to all motes and topics")
                for k in self.motes:
                    for l in self.motes[k]:
                        if self.motes[k][l] == 0:
                            unmute(k,l)     
                        self.motes[k][l]+= 1
                        
            else:
                try:
                    mote_id = int(content[0])
                    if content[1] == "#":
                        if self.verbose:
                            print("Subscription to mote " + str(mote_id) + " all its topics")
                        for k in self.motes[mote_id]:
                            if self.motes[mote_id][k] == 0:
                                unmute(mote_id, k)
                            self.motes[mote_id][k] += 1
                    else:
                        try:
                            topic = content[1]
                            if self.verbose:
                                print("Subscription to mote " + str(mote_id) + " topic " + topic)
                            if self.motes[mote_id][topic] == 0:
                                unmute(mote_id, topic)
                            self.motes[mote_id][topic] += 1
                        except Exception as e:
                            print("incorrect topic " + topic + " " + e)
                except Exception as e:
                    print("not correct subscription " + content[0] + " " + e)
        else: # received unsubscription
            if content[0] == "#":
                if self.verbose:
                    print("Unsubscription to all motes and topics")
                for k in self.motes:
                    for l in self.motes[k]:
                        self.motes[k][l]-= 1
                        if self.motes[k][l] <= 0:
                            self.motes[k][l] = 0
                            mute(k,l)
            else:
                try:
                    mote_id = int(content[0])
                    if content[1] == "#":
                        if self.verbose:
                            print("Unsubscription to mote " + str(mote_id) + " all its topics")
                        for k in self.motes[mote_id]:
                            self.motes[mote_id][k] -= 1
                            if self.motes[mote_id][k] <= 0:
                                self.motes[mote_id][k] = 0
                                mute(mote_id, k)
                    else:
                        try:
                            topic = content[1]
                            if self.verbose:
                                print("Unsubscription to mote " + str(mote_id) + " topic " + topic)
                            self.motes[mote_id][topic] -= 1
                            if self.motes[mote_id][topic] <= 0:
                                self.motes[mote_id][topic] = 0
                                mute(mote_id, topic)
                        except Exception as e:
                            print("incorrect topic " + topic + " " + e)
                except Exception as e:
                    print("not correct unsubscription " + content[0] +  " " + e)

    def run(self):
        def on_connect(client, userdata, flags, rc):
            """
            connect callback function
            """
            if self.verbose:
                print("connected with the result code "+str(rc))
                print("Subscription to $SYS logs")
            client.subscribe("$SYS/broker/log/M/subscribe/#")
            client.subscribe("$SYS/broker/log/M/unsubscribe/#")

        def on_message(client, userdata, msg):
            """
            message callback function
            """
            if msg.topic == "$SYS/broker/log/M/subscribe":
                sub = str(msg.payload).split(" ")[3]
                sub = sub[0:len(sub)-1] # rm '
                if self.verbose:
                    print("Subscription received " + sub)
                content = sub.split("/")
                self.handle_data(content, True)
            elif msg.topic == "$SYS/broker/log/M/unsubscribe":
                unsub = str(msg.payload).split(" ")[2]
                unsub = unsub[0:len(unsub)-1]
                if self.verbose:
                    print("Unsubscription received " + unsub)
                content = unsub.split("/")
                self.handle_data(content, False)
            else:
                print("unknown subscription")

        self.client.on_connect = on_connect
        self.client.on_message = on_message
        self.client.connect(self.server)

        if self.verbose:
            print("Muter connected on server")

        while self.flag:
            self.client.loop()

    def join(self):
        self.flag = False
        if self.verbose:
            print("Muter stopping")
        super(Muter, self).join(1)

if __name__ == "__main__":
    # Main for testing
    muter = Muter(None, "127.0.0.1", 5, True)
    muter.start()
    client = mqtt.Client()
    client.connect("127.0.0.1")
    
    sleep(2)

    client.subscribe("#")

    
    sleep(2)

    client.unsubscribe("#")

    sleep(2)
    
    client.subscribe("1/#")

    sleep(2)
    
    client.subscribe("2/#")

    sleep(2)

    client.subscribe("2/humidity")

    sleep(2)

    client.unsubscribe("2/temperature")

    sleep(2)

    client.unsubscribe("2/humidity")

    sleep(2)

    client.subscribe("lol")

    sleep(2)

    muter.join()
    
