#include <WiFi.h>

const char* WIFI_SSID = "RiaSummer2G";
const char* WIFI_PASSWORD = "730124go";

void setup() {
    Serial.begin(115200);

    Serial.println("Wi-Fi 연결 테스트");

    // 먼저 Wi-Fi 연결을 시작한다.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWi-Fi 연결 성공");
    Serial.print("IP address : ");
    Serial.println(WiFi.localIP());

    Serial.print("RSSI       : ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
}


void loop() {
}

