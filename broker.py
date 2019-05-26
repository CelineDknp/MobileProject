import paho.mqtt.client as mqtt
import serial as serial_dump 
from threading import Thread 
from muter import Muter
import sys 
import os 
import argparse

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
            data = line.split("/") 
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
                    self.serial.write("0/1\n".encode()) 
                if line == "periodically": 
                    self.serial.write("0/2\n".encode()) 
                if line == "onChange": 
                    self.serial.write("0/3\n".encode()) 
                elif line == "stop": 
                    self.flag = False 
                    if self.verbose: 
                        print("Goodbye!") 
            elif self.verbose: 
                print("Received unknown command :") 
                print(line) 
                            
                            
if __name__ == '__main__': 
    parser = argparse.ArgumentParser(description="Gateway between mosquitto broker and a Rime sensor network")
    parser.add_argument("server", help="Mosquitto server")
    parser.add_argument("serial", help="Serial connection with a sensor")
    parser.add_argument("nb_motes", help="Number of motes")
    parser.add_argument("--verbose", '-v', help="enables logging", action="store_true")
    
    args = parser.parse_args()

    server = args.server
    serial = serial_dump.Serial(port=args.serial,baudrate=115200) 
    nb_motes = args.nb_motes
    verbose = args.verbose
    dataThread = DataInput(server, serial, verbose) 
    cmdThread = CmdOutput(serial, verbose) 
    muterThread = Muter(serial, server, nb_motes, verbose)
    dataThread.start() 
    cmdThread.start() 
    muterThread.start()
    cmdThread.join() 
    dataThread.join() 
    muterThread.join()
    serial.close()
