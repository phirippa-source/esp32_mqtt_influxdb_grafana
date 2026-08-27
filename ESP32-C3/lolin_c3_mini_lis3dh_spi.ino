#include <SPI.h>
#include <SparkFunLIS3DH.h>

LIS3DH lis3dh(SPI_MODE, 5);

void setup() {
    Serial.begin(230400);

    SPI.begin(2, 0, 4, 5);  // SCK=2, MISO=0, MOSI=4, CS=5

    // LIS3DH 설정
    lis3dh.settings.accelSampleRate = 400;  // 출력 데이터 속도(ODR) = 400 Hz
    lis3dh.settings.accelRange = 2;         // 측정 범위 = ±2 g
    lis3dh.settings.xAccelEnabled = 1;
    lis3dh.settings.yAccelEnabled = 1;
    lis3dh.settings.zAccelEnabled = 1;

    if (lis3dh.begin() != 0) {
        Serial.println("LIS3DH init failed");
        while (1);
    }
}

void loop() {
    Serial.print(lis3dh.readFloatAccelX(), 3);
    Serial.print(", ");
    Serial.print(lis3dh.readFloatAccelY(), 3);
    Serial.print(", ");
    Serial.println(lis3dh.readFloatAccelZ(), 3);

    delayMicroseconds(2500);   // 400 Hz 목표: 1 / 400 s = 2.5 ms
}
