import paho.mqtt.client as mqtt
import time

# MQTT 클라이언트 객체 생성
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# 로컬 MQTT 브로커에 연결
client.connect('localhost', 1883, 60)

# MQTT 네트워크 처리 시작
client.loop_start()

# 'temp' 토픽으로 25.1 전송
client.publish('temp', 25.1)

# 메시지가 전송될 시간을 잠시 기다림
time.sleep(1)

# MQTT 네트워크 처리 종료
client.loop_stop()

# MQTT 브로커 연결 종료
client.disconnect()

