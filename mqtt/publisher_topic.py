import paho.mqtt.client as mqtt
import time

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.connect('localhost', 1883, 60)

client.loop_start()

for _ in range(3):
    client.publish('Riatech/Factory_A/Line1/Temp', 25.1)
    client.publish('Riatech/Factory_A/Line1/Humi', 33.5)
    client.publish('Riatech/Factory_A/Line1/Lux', 450.1)

    client.publish('Riatech/Factory_A/Line2/Temp', 35.8)
    client.publish('Riatech/Factory_A/Line2/Humi', 73.3)
    client.publish('Riatech/Factory_A/Line2/Lux', 120.1)

    time.sleep(3)

client.loop_stop()
client.disconnect()
