#include <WiFi.h>

const char* WIFI_SSID = "<공유기 이름>";
const char* WIFI_PASSWORD = "<공유기 암호>";

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Wi-Fi 연결 테스트");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);    // 먼저 Wi-Fi 연결을 시작한다.
    WiFi.setTxPower(WIFI_POWER_8_5dBm);     // LOLIN C3 Mini의 Wi-Fi 안정성을 위해 송신 출력 제한
    
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

