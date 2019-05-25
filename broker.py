import paho.mqtt.client as mqtt
import serial as serial_dump 
from threading import Thread 
import sys 
import os 

class DataInput(Thread): 
    def __init__(self, server, serial, verbose): 
        super(DataInput, self).__init__() 
        self.serial = serial 
        self.verbose = verbose 
        self.server = server 
        self.client = mqtt.Client() 
        self.flag = True 
        
    def run(self): 
        self.client.connect(self.server) 
        self.client.loop_start() 
        while self.flag: 
            line = self.serial.readline().decode() 
            data = line.split("-") 
            if len(data) == 3: 
                sender = data[0].strip() 
                mesure = data[1].strip() 
                value = data[2].strip() 
                if mesure == "temperature" or mesure == "humidity" or mesure == "light": 
                    topic = "/" + sender + "/" + mesure 
                    self.client.publish(topic, value, qos=2) 
                    if self.verbose: 
                        print() 
                        print("Received data :") 
                        print(topic) 
                        print(value) 
                        print(">>", end=" ", flush=True) 
            elif self.verbose: 
                print() 
                print("Received unknown data :") 
                print(line) 
                print(">>", end=" ", flush=True) 
                self.client.loop_stop() 
                self.client.disconnect() 
                
    def join(self): 
        self.flag = False 
        super(DataInput, self).join(1) 

class Muter(Thread):
    def __init__(self, serial, server, verbose):
        Thread.__init__(self)
        self.serial = serial
        self.verbose = verbose
        self.flag = True
        self.server = server
        self.client = mqtt.Client()
        #TODO dico for nodes

    def run(self):
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

        self.client.on_connect = on_connect
        self.client.on_message = on_message
        self.client.connect(self.server)

        while self.flag:
            self.client.loop()

        
class CmdOutput(Thread): 
    def __init__(self, serial, verbose): 
        Thread.__init__(self) 
        self.serial = serial 
        self.verbose = verbose 
        self.flag = True 
        
    def run(self): 
        if self.verbose: 
            print("Gateway started !") 
            print("Commands available :") 
            print("- noSend") 
            print("- periodically") 
            print("- onChange") 
            print("- stop") 
        while self.flag: 
            line = input(">> ") 
            if line == "noSend" or line == "periodically" or line == "onChange": 
                if self.verbose: 
                    print("Received command :") 
                    print(line) 
                if line == "noSend": 
                    self.serial.write("1\n".encode()) 
                if line == "periodically": 
                    self.serial.write("2\n".encode()) 
                if line == "onChange": 
                    self.serial.write("3\n".encode()) 
                elif line == "stop": 
                    self.flag = False 
                    if self.verbose: 
                        print("Goodbye!") 
            elif self.verbose: 
                print("Received unknown command :") 
                print(line) 
                            
                            
if __name__ == '__main__': 
    if sys.version_info[0] < 3 or len(sys.argv) < 3 or len(sys.argv) > 4: 
        print("Usage : sudo python3 gateway.py server port [-v]") 
        exit() 

    server = sys.argv[1] 
    serial = serial_dump.Serial(port=sys.argv[2],baudrate=115200) 
    verbose = True if len(sys.argv) == 4 and sys.argv[3] == "-v" else False 
    dataThread = DataInput(server, serial, verbose) 
    cmdThread = CmdOutput(serial, verbose) 
    dataThread.start() 
    cmdThread.start() 
    cmdThread.join() 
    dataThread.join() 
    serial.close()