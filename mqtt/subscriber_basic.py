import paho.mqtt.client as mqtt

# MQTT 브로커에 연결되었을 때 자동으로 호출되는 함수
def on_connect(client, userdata, flag, reason_code, properties):
    # 브로커 연결 결과를 화면에 출력
    print('Connect with result code : ' + str(reason_code))

    # 'temp' 토픽을 구독
    # 이후 이 토픽으로 메시지가 수신되면 on_message() 함수가 호출됨
    client.subscribe('temp')


# 구독 중인 토픽으로 메시지가 수신되었을 때 자동으로 호출되는 함수
def on_message(client, userdata, msg):
    # 수신한 토픽 이름과 메시지 내용을 화면에 출력
    print(msg.topic + ':' + str(msg.payload))


# MQTT 클라이언트 객체 생성
# Callback API VERSION2 방식 사용
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# MQTT 연결 성공 시 실행할 콜백 함수 등록
client.on_connect = on_connect

# MQTT 메시지 수신 시 실행할 콜백 함수 등록
client.on_message = on_message

# MQTT 브로커에 연결
# localhost : 현재 Ubuntu 컴퓨터
# 1883      : MQTT 기본 포트 번호
# 60        : Keep Alive 시간(초)
client.connect('localhost', 1883, 60)

# MQTT 네트워크 처리를 계속 반복하면서
# 메시지 수신을 대기
client.loop_forever()


