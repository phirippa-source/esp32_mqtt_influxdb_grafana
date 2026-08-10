#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

const char* ssid = "RiaSummer2G";
const char* password = "730124go";
const char* brokerAddress = "192.168.2.8";
const char* brokerUser = "ship";
const char* brokerPassword = "1234";
const char* clientId = "ria_lolin_c3_mini_sub_01";
const char* topic = "Riatech/A/Line1/Led";

#define LED_PIN     7
#define LED_COUNT   1
Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void onMessage(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.println("Topic: " + String(topic));
    Serial.println("Message: " + message);

    if (message == "on" || message == "On") {
        led.setPixelColor(0, led.Color(255,0,0));   // 빨강
        led.show();
    } else if (message == "off" || message == "Off") {
        led.setPixelColor(0, led.Color(0,0,0));     // 끄기
        led.show();
    } else {
        Serial.print("Wring Message ! : " + message );
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
    
    led.begin();
}


void loop() {
    mqttClient.loop();
}
