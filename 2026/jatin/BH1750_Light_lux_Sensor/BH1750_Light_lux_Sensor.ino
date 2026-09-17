#include <Wire.h>
#include <BH1750.h>
// create an object
BH1750 lightMeter;


void setup() {
Serial.begin(9600);
Wire.begin();
lightMeter.begin(); // start the sensor
}

void loop() {
  // Read LUX value from sensor
  float lux = lightMeter.readLightLevel(); 
  Serial.println(lux);
  delay(2000);
}
