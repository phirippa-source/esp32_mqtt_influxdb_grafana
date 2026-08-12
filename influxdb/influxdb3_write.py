from influxdb_client_3 import InfluxDBClient3, Point
import random
import time

client = InfluxDBClient3(
        host="http://<YOUR SERVER IP ADDRESS>:8181",
        token="<YOUR TOKEN>",
        database="<YOUR DATABASE"
        )

while True:
    temperature = random.uniform(20.0, 30.0)

    point = Point("temperature")\
            .tag("location", "living_room")\
            .field("value", round(temperature, 1))
    client.write(point)
    print(f"temperature = {temperature:.1f}")
    time.sleep(2)
