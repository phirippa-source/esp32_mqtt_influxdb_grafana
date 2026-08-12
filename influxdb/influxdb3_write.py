from influxdb_client_3 import InfluxDBClient3, Point
import random
import time

client = InfluxDBClient3(
        host="http://192.168.2.8:8181",
        token="apiv3_xicSuKg7aBprci2PnxRFgKwXD0fRh995RhL78U-voI6cukfqCV4zNZJ1Ns6zDaPQd8GjePqJZy4Kqxqcz3hVuA",
        database="dbtest"
        )

while True:
    temperature = random.uniform(20.0, 30.0)

    point = Point("temperature")\
            .tag("location", "living_room")\
            .field("value", round(temperature, 1))
    client.write(point)
    print(f"temperature = {temperature:.1f}")
    time.sleep(2)
