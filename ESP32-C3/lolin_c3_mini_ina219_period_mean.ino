#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

float voltageSum = 0;
float currentSum = 0;
int sampleCount = 0;

unsigned long previousTime = 0;

void setup() {
    Serial.begin(115200);
    Wire.begin(8, 10);
    Wire.setClock(400000);

    if (!ina219.begin()) {
        Serial.println("INA219 init failed");
        while (1);
    }

    previousTime = millis();
}

void loop() {
    float voltage = ina219.getBusVoltage_V();
    float current = ina219.getCurrent_mA();

    voltageSum += voltage;
    currentSum += current;
    sampleCount++;

    if (millis() - previousTime >= 1000) {
        float voltageMean = voltageSum / sampleCount;
        float currentMean = currentSum / sampleCount;

        Serial.print("voltage_mean_V:");
        Serial.print(voltageMean, 3);
        Serial.print(",");

        Serial.print("current_mean_mA_div10:");
        Serial.println(currentMean / 10.0, 2);

        voltageSum = 0;
        currentSum = 0;
        sampleCount = 0;

        previousTime += 1000;
    }

    delay(10);
}
