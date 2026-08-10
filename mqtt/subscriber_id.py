import paho.mqtt.client as mqtt

def on_connect(client, userdata, flag, reason_code, properties):
    print('Connect with result code : ' + str(reason_code))
    client.subscribe('Riatech/Factory_A/+/Lux')

def on_message(client, userdata, msg):
    print(msg.topic + ':' + str(msg.payload))

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(username='ship', password='1234')

client.on_connect = on_connect
client.on_message = on_message

client.connect('localhost', 1883, 60)
client.loop_forever()
