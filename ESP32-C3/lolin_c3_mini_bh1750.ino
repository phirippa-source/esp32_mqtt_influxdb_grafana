#include <BH1750.h>
#include <Wire.h>
BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 10);    // SDA = GPIO8, SCL = GPIO10, f= 400kHz
  lightMeter.begin();
  Serial.println(F("BH1750 Test begin"));
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
}
