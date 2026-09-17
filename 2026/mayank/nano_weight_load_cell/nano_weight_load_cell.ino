// Working, gives raw values
#include "HX711.h"

#define HX711_DOUT 12
#define HX711_SCK  11

HX711 scale;

void setup() {
  Serial.begin(9600);

  Serial.println("HX711 Load Cell Test");

  scale.begin(HX711_DOUT, HX711_SCK);

  // Wait for HX711
  if (!scale.is_ready()) {
    Serial.println("ERROR: HX711 not found!");
    while (1);
  }

  Serial.println("HX711 connected.");

  // First test: zero the scale
  Serial.println("Remove all weight...");
  delay(3000);

  scale.tare();

  Serial.println("Tare complete.");
  Serial.println("Place weight on the load cell.");
}

void loop() {
  delay(1000);
  if (scale.is_ready()) {

    // Raw reading for initial testing
    long reading = scale.read();

    Serial.print("Raw: ");
    Serial.print(reading);

    // After calibration, this can be changed to get_units()
    Serial.println();

  } else {
    Serial.println("HX711 not ready!");
  }

  delay(500);
}