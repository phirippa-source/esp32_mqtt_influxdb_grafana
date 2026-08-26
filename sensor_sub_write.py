import json
import paho.mqtt.client as mqtt
from influxdb_client_3 import InfluxDBClient3, Point

MQTT_BROKER = "192.168.2.4"    # BROKER의 IP 주소
MQTT_TOPIC = "Riatech/Line1/SensorData"

INFLUX_HOST = "http://192.168.2.4:8181"    # http://<influxdb 서버 주소>:8181
INFLUX_TOKEN = "<토큰>"   
INFLUX_DATABASE = "riatech_factory"

influx = InfluxDBClient3(
    host=INFLUX_HOST,
    token=INFLUX_TOKEN,
    database=INFLUX_DATABASE
)

def on_connect(client, userdata, flags, reason_code, properties):
    print("MQTT connected")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    print(msg.topic + ':' + str(msg.payload(), end='\t')
    data = json.loads(msg.payload.decode())

    lux = data["lux"]
    voltage = data["voltage_V"]
    current = data["current_mA"]

    print(lux, voltage, current)

    point = (
        Point("line1")
        .field("lux", lux)
        .field("voltage_V", voltage)
        .field("current_mA", current)
    )

    influx.write(point)

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set("ship", "1234")
client.on_connect = on_connect
client.on_message = on_message
client.connect(MQTT_BROKER, 1883)
client.loop_forever()
