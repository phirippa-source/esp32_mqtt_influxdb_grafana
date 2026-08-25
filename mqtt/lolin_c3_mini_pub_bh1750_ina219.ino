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

const char* topicLux = "Riatech/Line1/Lux";
const char* topicVoltage = "Riatech/Line1/Voltage";
const char* topicCurrent = "Riatech/Line1/Current";


// ------------------------------------------------------------
// I2C settings
// ------------------------------------------------------------
const int I2C_SDA = 8;
const int I2C_SCL = 10;

// ------------------------------------------------------------
// Publish settings
// ------------------------------------------------------------
const unsigned long PUBLISH_INTERVAL_MS = 1000;
unsigned long previousPublishTime = 0;


// ------------------------------------------------------------
// MQTT objects
// ------------------------------------------------------------
WiFiClient wifiClient;
PubSubClient client(serverIPAddr, mqttPort, wifiClient);

void connectWiFi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

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


void publishSensorData() {
    float lux = lightMeter.readLightLevel();
    float voltage_V = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();

    char luxBuffer[16];
    char voltageBuffer[16];
    char currentBuffer[16];

    snprintf(luxBuffer, sizeof(luxBuffer), "%.1f", lux);
    snprintf(voltageBuffer, sizeof(voltageBuffer), "%.3f", voltage_V);
    snprintf(currentBuffer, sizeof(currentBuffer), "%.1f", current_mA);

    bool luxPublished = client.publish(topicLux, luxBuffer);
    bool voltagePublished = client.publish(topicVoltage, voltageBuffer);
    bool currentPublished = client.publish(topicCurrent, currentBuffer);

    Serial.println();

    Serial.print(topicLux);
    Serial.print(" : ");
    Serial.print(luxBuffer);
    Serial.println(" lx");

    Serial.print(topicVoltage);
    Serial.print(" : ");
    Serial.print(voltageBuffer);
    Serial.println(" V");

    Serial.print(topicCurrent);
    Serial.print(" : ");
    Serial.print(currentBuffer);
    Serial.println(" mA");

    if (!luxPublished || !voltagePublished || !currentPublished) {
        Serial.println("MQTT publish failed");
    }
}


void setup() {
    Serial.begin(115200);
    connectWiFi();
    connectMQTT();
    initializeSensors();
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

    if (currentTime - previousPublishTime >= PUBLISH_INTERVAL_MS) {
        previousPublishTime = currentTime;
        publishSensorData();
    }
}
