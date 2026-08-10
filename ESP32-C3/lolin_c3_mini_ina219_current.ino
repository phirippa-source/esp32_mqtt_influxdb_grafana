#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

void setup() {
    Serial.begin(115200);
    Wire.begin(8, 10);
    Wire.setClock(400000);

    if (!ina219.begin()) {
        Serial.println("INA219 init failed");
        while (1);
    }
}

void loop() {
    float current = ina219.getCurrent_mA();

    Serial.print("current:");
    Serial.println(current, 1);

    delay(10);
}
