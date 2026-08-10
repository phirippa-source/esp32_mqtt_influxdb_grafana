#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "RiaSummer2G";
const char* password = "730124go";
const char* serverIPAddr = "test.mosquitto.org";

const char* clientId = "ria_lolin_c3_mini_01";
const char * topic = "Riatech/A/Line1/Temp";

WiFiClient wifiClient; 
PubSubClient client(serverIPAddr, 1883, wifiClient);

void setup() {
    Serial.begin(115200);
    Serial.print("\nConnecting to " + String(ssid));
    WiFi.begin(ssid, password);         // 지정한 공유기에 접속 시도
    WiFi.setTxPower(WIFI_POWER_8_5dBm); 
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println(" WiFi Connected");   
    // ESP32 보드가 공유기로부터 할당 받은 "(사설)IP 주소"
    Serial.println("Local IP address : " + WiFi.localIP().toString());

    Serial.printf("\nConnecting to the Broker(%s)\n", serverIPAddr);
    while ( !client.connect(clientId) ){ 
        Serial.print("*");
        delay(500);
    }
    Serial.println("Connected to the Broker!!!");
}

void loop() {
    client.publish(topic, "27.32");
    Serial.println(String(topic) + ": 27.32");
    delay(2000);
 }
