#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <Adafruit_INA219.h>
#include <SparkFunLIS3DH.h>
#include <arduinoFFT.h>


// ============================================================
// 회로 연결 설정
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
const uint16_t samples = 256;
const double sampling_frequency = 400.0;


// ============================================================
// AI 추론 설정
// 동일한 AI 추론 결과가 3회 연속되면 Fan State 변경
const int confirm_count = 3;


// ============================================================
// Wi-Fi 설정
const char *wifi_ssid = "RiaSummer2G";
const char *wifi_password = "730124go";


// ============================================================
// MQTT 설정
const char *mqtt_server = "192.168.2.5";
const int mqtt_port = 1883;
const char *mqtt_user = "ship";
const char *mqtt_password = "1234";
const char *mqtt_client_id = "lolin_c3_fan_ai_01";
const char *mqtt_topic = "Riatech/Line1/FanAI";


// ============================================================
// AI / MQTT 기능
// ============================================================

#include "fan_model.h"
#include "fan_ai_core.h"
#include "mqtt_core.h"


// ============================================================
// setup()
// ============================================================

void setup() {
    Serial.begin(230400);
    delay(1000);
    sensor.begin();
    mqtt.begin();
}


// ============================================================
// loop()
//
// Sensor
//   ↓
// FFT Feature
//   ↓
// Decision Tree Inference
//   ↓
// Fan State
//   ↓
// MQTT Publish
// ============================================================

void loop() {
    sensor.read_data();

    fft.make_features();

    print_features();

    dt.predict_state();

    fan_state.update(dt.get_prediction());

    mqtt.publish_data();

    Serial.println("========================================");
    Serial.println();
}
