from influxdb_client_3 import InfluxDBClient3

client = InfluxDBClient3(
        host="http:<YOUR SERVER IP ADDRESS>:8181",
        token="<YOUR TOKEN>",
        database="<YOUR DATABASE NAME"
        )

query = """
SELECT
    CAST(time AS VARCHAR) as time,
    location,
    value
FROM temperature
ORDER BY time DESC
LIMIT 10
"""

table = client.query(query=query)
rows = table.to_pylist()

for row in rows:
    print(row)
