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
    Serial.print("Voltage: ");
    Serial.print(ina219.getBusVoltage_V());
    Serial.print(" V, Current: ");
    Serial.print(ina219.getCurrent_mA());
    Serial.println(" mA");

    delay(500);
}
