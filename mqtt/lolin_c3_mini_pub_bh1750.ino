#include <WiFi.h>
#include <PubSubClient.h>

// BH1750 ---------------------------------------
#include <Wire.h>
#include <BH1750.h>
BH1750 lightMeter;
// ----------------------------------------------

const char* ssid = "RiaSummer2G";
const char* password = "730124go";
const char* serverIPAddr = "192.168.2.6";
const char* userId = "ship";
const char* userPw = "1234";
const char* clientId = "ria_lolin_c3_mini_01";
const char* topic = "Riatech/A/Line1/Lux";

WiFiClient wifiClient; 
PubSubClient client(serverIPAddr, 1883, wifiClient);

void setup() {
    Serial.begin(115200);
    Serial.print("\nConnecting to " + String(ssid));
    // 지정한 공유기에 접속 시도
    WiFi.begin(ssid, password);
    WiFi.setTxPower(WIFI_POWER_8_5dBm); 
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println(" WiFi Connected");   

    Serial.printf("\nConnecting to the Broker(%s)\n", serverIPAddr);
    while ( !client.connect(clientId, userId, userPw) ){ 
        Serial.print("*");
        delay(500);
    }
    Serial.println("Connected to the Broker!!!");
    
    // BH1750 ---------------------------------
    Wire.begin(8, 10, 400000);
    lightMeter.begin();
}

void loop() {
    client.loop();
    float lux = lightMeter.readLightLevel();
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", lux);
    bool result = client.publish(topic, buf);

    if (result) {
        Serial.print(topic);
        Serial.print(" : ");
        Serial.println(buf);
    } else {
        Serial.println("Publish failed");
    }
    delay(1000);
}
