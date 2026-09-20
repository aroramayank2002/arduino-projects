#include <Wire.h>
#include "Adafruit_HTU21DF.h"

// Initialize the sensor
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

void setup() {
  Serial.begin(9600);
  htu.begin();
}

void loop() {
  float temp = htu.readTemperature();
  float rel_hum = htu.readHumidity();
  
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" C | ");
  
  Serial.print("Humidity: ");
  Serial.print(rel_hum);
  Serial.println(" %");
  
  delay(2000); // Wait 2 seconds before the next reading
}

