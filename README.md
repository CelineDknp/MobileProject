# MobileProject

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