import paho.mqtt.client as mqtt
# --------------------
from influxdb_client_3 import InfluxDBClient3, Point
INFLUX_HOST ='http://192.168.52.136:8181'
INFLUX_TOKEN = 'apiv3_hJxBxRi1VCOmEUtjqG0aUVTc-zw'
INFLUX_DATABASE ='riatech'

influx_client = InfluxDBClient3(
    host = INFLUX_HOST,
    token = INFLUX_TOKEN,
    database = INFLUX_DATABASE
)
# ---------------------
def on_connect(client, userdata, flag, reason_code):
    print('Connect with result code : ' + str(reason_code))
    client.subscribe('riatech/production_equipment/+/lux')

def on_message(client, userdata, msg):
    print(msg.topic + ':' + str(msg.payload))
    topic_split = msg.topic.split('/')
    lux = float(msg.payload.decode())
    point = Point('production_equipment')\
            .tag('# Equipment',topic_split[2])\
            .field(topic_split[3],lux)
    influx_client.write(point)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect('broker.emqx.io', 1883, 60)
client.loop_forever()
