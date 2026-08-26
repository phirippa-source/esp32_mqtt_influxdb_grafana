#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

#include <Wire.h>
#include <BH1750.h>
BH1750 lightMeter;

const char* ssid = "RiaSummer2G";
const char* password = "730124go";
const char* brokerAddress = "192.168.2.8";
const char* brokerUser = "ship";
const char* brokerPassword = "1234";
const char* clientId = "ria_lolin_c3_mini_sub_pub_01";
const char* topicSub = "Riatech/Line1/Led";
const char* topicPub = "Riatech/Line1/Lux";
unsigned long int pre_time, current_time;

#define LED_PIN     7
#define LED_COUNT   1
Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void onMessage(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Topic: " + String(topic));
    Serial.println("\tMessage: " + message);

    if (message == "on" || message == "On") {
        led.setPixelColor(0, led.Color(255,0,0));   // 빨강
        led.show();
    } else if (message == "off" || message == "Off") {
        led.setPixelColor(0, led.Color(0,0,0));     // 끄기
        led.show();
    } else {
        Serial.print("Wrong Message! : " + message );
    }
}

WiFiClient wifiClient;
PubSubClient mqttClient(brokerAddress, 1883, onMessage, wifiClient);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.print("Connecting to Wi-Fi: " + String(ssid));
    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");  delay(500);
    }
    Serial.println("Wi-Fi connected");

    Serial.println("Connecting to MQTT Broker: " + String(brokerAddress));
    while (!mqttClient.connect(clientId, brokerUser, brokerPassword)) {
        Serial.print(".");  delay(500);
    }

    Serial.println();
    Serial.println("MQTT Broker connected");
    mqttClient.subscribe(topicSub);
    Serial.println("Subscribed topic: " + String(topicSub));
    
    led.begin();
    Wire.begin(8, 10, 400000);
    lightMeter.begin();
    pre_time = millis();
}

void publish_lux(void) {
    char buf[16];
    float lux = lightMeter.readLightLevel();
    snprintf(buf, sizeof(buf), "%.1f", lux);
    bool result = mqttClient.publish(topicPub, buf);

    if (result) {
        Serial.printf("%s : %s\n", topicPub, buf);
    } else {
        Serial.println("Publish failed");
    }
}

void loop() {
    mqttClient.loop();

    current_time = millis();
    if (current_time - pre_time >= 4000) {
        pre_time = current_time;
        publish_lux();
    } 
}
