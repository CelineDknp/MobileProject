import paho.mqtt.client as mqtt
from time import sleep

client = mqtt.Client()
client.connect("127.0.0.1")

client.subscribe("lol/#")

sleep(5)

client.unsubscribe("lol/1")