#include <Wire.h>
#include <SPI.h>

#include <Adafruit_INA219.h>
#include <SparkFunLIS3DH.h>
#include <arduinoFFT.h>


// ============================================================
// 회로 연결 설정
// ============================================================

// INA219 - I2C
const int i2c_sda = 8;
const int i2c_scl = 10;

// LIS3DH - SPI
const int lis3dh_sck  = 2;
const int lis3dh_miso = 0;
const int lis3dh_mosi = 4;
const int lis3dh_cs   = 5;


// ============================================================
// 데이터 수집 설정
// ============================================================

// FFT Sample 개수
const uint16_t samples = 256;

// LIS3DH Sampling Frequency
const double sampling_frequency = 400.0;


// ============================================================
// AI 추론 설정
// ============================================================

// 동일한 AI 추론 결과가 연속으로 나타나야 하는 횟수
// 3회 × 약 0.64초 = 약 2초
const int confirm_count = 3;


// ============================================================
// AI 모델 및 기능
// ============================================================

#include "fan_model.h"
#include "fan_ai_core.h"


// ============================================================
// setup()
// ============================================================

void setup() {
    Serial.begin(230400);
    delay(1000);

    Serial.println();
    Serial.println("LOLIN C3 MINI - Fan AI Inference");
    Serial.println("========================================");

    sensor.begin();

    Serial.println("Sensor Ready");
    Serial.println("Decision Tree Model Ready");
    Serial.println();
}


// ============================================================
// loop()
//
// Sensor Data
//     ↓
// FFT Feature
//     ↓
// Decision Tree AI Inference
//     ↓
// Fan State
// ============================================================

void loop() {
    sensor.read_data();
    fft.make_features();
    print_features();
    dt.predict_state();
    fan_state.update(dt.get_prediction());
    Serial.println("========================================");
    Serial.println();
}
