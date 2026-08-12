import requests
import random
import time

url = "http://YOUR_SERVER_IP_ADDRESS/api/v3/write_lp"
database = "YOUR_DATABASE_NAME"
token = "YOUR_TOKEN"  
headers = {
    "Authorization": f"Bearer {token}"
}

params = {
    "db": database
}

while True:
  temperature = random.uniform(20.0, 30.0)
  line = f"temperature,location=living_room value={temperature:.1f}"
  response = requests.post(
        url,
        params=params,
        headers=headers,
        data=line
    )
    if response.status_code == 204:
        print(line)
    else:
        print("Write failed:", response.status_code, response.text)

    time.sleep(2)
