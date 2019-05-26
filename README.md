# MobileProject

Installation:

```bash
sudo apt-get install mosquitto mosquitto-clients
```

Python packages:

```bash
pip3 install paho-mqtt pyserial argparse
```

for setup mosquitto:

```bash
sudo cp mosquitto.conf /etc/mosquitto/conf.d
```

for launch mosquitto:

```bash
sudo mosquitto -v -c /etc/mosquitto/mosquitto.conf
```

for broker help:

```bash
python3 broker.py -h
```

if the server is local, then it is 127.0.0.1