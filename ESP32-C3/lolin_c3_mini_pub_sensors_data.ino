#include <WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_INA219.h>

// ------------------------------------------------------------
// Sensor objects
// ------------------------------------------------------------
BH1750 lightMeter;
Adafruit_INA219 ina219;

// ------------------------------------------------------------
// Wi-Fi settings
// ------------------------------------------------------------
const char* ssid = "RiaSummer2G";
const char* password = "730124go";

// ------------------------------------------------------------
// MQTT settings
// ------------------------------------------------------------
const char* serverIPAddr = "192.168.2.8";
const int mqttPort = 1883;

const char* userId = "ship";
const char* userPw = "1234";
const char* clientId = "ria_lolin_c3_mini_01";

const char* topicSensorData = "Riatech/A/Line1/SensorData";

// ------------------------------------------------------------
// I2C settings
// ------------------------------------------------------------
const int I2C_SDA = 8;
const int I2C_SCL = 10;

// ------------------------------------------------------------
// Sampling / Average settings
// ------------------------------------------------------------
const unsigned long SAMPLE_INTERVAL_MS = 200;
const unsigned long AVERAGE_INTERVAL_MS = 1000;

unsigned long previousSampleTime = 0;
unsigned long averageStartTime = 0;

// ------------------------------------------------------------
// Average data
// ------------------------------------------------------------
float luxSum = 0.0;
float voltageSum = 0.0;
float currentSum = 0.0;
unsigned int sampleCount = 0;

// ------------------------------------------------------------
// MQTT objects
// ------------------------------------------------------------
WiFiClient wifiClient;
PubSubClient client(serverIPAddr, mqttPort, wifiClient);

void connectWiFi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("Wi-Fi connected");

    Serial.print("Local IP address: ");
    Serial.println(WiFi.localIP());
}

void connectMQTT() {
    Serial.printf("Connecting to Broker (%s:%d)\n", serverIPAddr, mqttPort);

    while (!client.connected()) {
        if (client.connect(clientId, userId, userPw)) {
            Serial.println("Connected to Broker");
        } else {
            Serial.print("MQTT connection failed, state = ");
            Serial.println(client.state());
            delay(1000);
        }
    }
}

void initializeSensors() {
    Wire.begin(I2C_SDA, I2C_SCL, 400000);

    if (!lightMeter.begin()) {
        Serial.println("BH1750 initialization failed");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("BH1750 initialized");

    if (!ina219.begin()) {
        Serial.println("INA219 initialization failed");

        while (true) {
            delay(1000);
        }
    }

    Serial.println("INA219 initialized");
}

void sampleSensorData() {
    float lux = lightMeter.readLightLevel();
    float voltage_V = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();

    luxSum += lux;
    voltageSum += voltage_V;
    currentSum += current_mA;

    sampleCount++;
}

void publishAverageSensorData() {
    if (sampleCount == 0) {
        return;
    }

    float luxAvg = luxSum / sampleCount;
    float voltageAvg = voltageSum / sampleCount;
    float currentAvg = currentSum / sampleCount;

    char payload[128];

    snprintf(payload, sizeof(payload),
             "{\"lux\":%.1f,\"voltage_V\":%.3f,\"current_mA\":%.1f}",
             luxAvg, voltageAvg, currentAvg);

    bool published = client.publish(topicSensorData, payload);

    Serial.println();
    Serial.print(topicSensorData);
    Serial.print(" : ");
    Serial.println(payload);

    Serial.print("Samples : ");
    Serial.println(sampleCount);

    if (!published) {
        Serial.println("MQTT publish failed");
    }

    luxSum = 0.0;
    voltageSum = 0.0;
    currentSum = 0.0;
    sampleCount = 0;
}

void setup() {
    Serial.begin(115200);

    connectWiFi();
    connectMQTT();
    initializeSensors();

    unsigned long currentTime = millis();

    previousSampleTime = currentTime;
    averageStartTime = currentTime;
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (!client.connected()) {
        connectMQTT();
    }

    client.loop();

    unsigned long currentTime = millis();

    if (currentTime - previousSampleTime >= SAMPLE_INTERVAL_MS) {
        previousSampleTime = currentTime;
        sampleSensorData();
    }

    if (currentTime - averageStartTime >= AVERAGE_INTERVAL_MS) {
        averageStartTime = currentTime;
        publishAverageSensorData();
    }
}
