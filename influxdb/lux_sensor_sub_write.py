import paho.mqtt.client as mqtt
# --------------------
from influxdb_client_3 import InfluxDBClient3, Point
INFLUX_HOST ='http://192.168.2.4:8181'
INFLUX_TOKEN = 'apiv3__4Q53wwusBHZb2dKRUVPJjrZwgqdkTM2kgmzvCUU'
INFLUX_DATABASE ='riatech_factory'

influx_client = InfluxDBClient3(
    host = INFLUX_HOST,
    token = INFLUX_TOKEN,
    database = INFLUX_DATABASE
)
# ---------------------
def on_connect(client, userdata, flag, reason_code, properties):
    print('Connect with result code : ' + str(reason_code))
    client.subscribe('Riatech/Line1/Lux')

def on_message(client, userdata, msg):
    print(msg.topic + ':' + str(msg.payload))
    lux = float(msg.payload.decode())
    point = Point('line1')\
            .field('lux', lux)
    influx_client.write(point)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(username='ship', password='1234')

client.on_connect = on_connect
client.on_message = on_message
client.connect('localhost', 1883, 60)
client.loop_forever()

