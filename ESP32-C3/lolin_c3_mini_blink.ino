#include <Adafruit_NeoPixel.h>

#define LED_PIN 7
#define LED_COUNT 1

Adafruit_NeoPixel led(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    led.begin();
}

void loop() {
    led.setPixelColor(0, led.Color(255, 0, 0));  // 빨강
    led.show();
    delay(1000);

    led.setPixelColor(0, led.Color(0, 0, 0));    // 끄기
    led.show();
    delay(1000);
}
