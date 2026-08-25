#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "RiaSummer2G";
const char* password = "730124go";
const char* brokerAddress = "192.168.2.8";
const char* brokerUser = "ship";
const char* brokerPassword = "1234";
const char* clientId = "ria_lolin_c3_mini_sub_01";
const char* topic = "Riatech/Line1/Led";

void onMessage(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Topic: " + String(topic));
    Serial.println("\tMessage: " + message);
}

WiFiClient wifiClient;
PubSubClient mqttClient(brokerAddress, 1883, onMessage, wifiClient);


void setup() {
    Serial.begin(115200);
    delay(1000); // 시리얼 모니터 연결을 기다림
    Serial.print("Connecting to Wi-Fi: " + String(ssid));

    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    Serial.println("Wi-Fi connected");
    Serial.println("Connecting to MQTT Broker: " + String(brokerAddress));

    while (!mqttClient.connect(clientId, brokerUser, brokerPassword)) {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("MQTT Broker connected");
    mqttClient.subscribe(topic);
    Serial.println("Subscribed topic: " + String(topic));
}


void loop() {
    mqttClient.loop();
}
