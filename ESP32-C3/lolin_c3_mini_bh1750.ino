#include <BH1750.h>
#include <Wire.h>

// BH1750 조도 센서 객체 생성
BH1750 lightMeter;

void setup() {
  // 시리얼 통신 시작
  Serial.begin(115200);

  // I2C 통신 시작
  // SDA = GPIO8, SCL = GPIO10
  Wire.begin(8, 10);

  // I2C 통신 속도를 400 kHz로 설정
  Wire.setClock(400000);

  // BH1750 센서 초기화
  lightMeter.begin();

  // 센서 테스트 시작 메시지 출력
  Serial.println("BH1750 Test begin");
}

void loop() {
  // BH1750 센서에서 조도값을 읽음
  // 단위는 lux(lx)
  float lux = lightMeter.readLightLevel();

  // 측정한 조도값을 시리얼 모니터에 출력
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  // 1초 대기
  delay(1000);
}
