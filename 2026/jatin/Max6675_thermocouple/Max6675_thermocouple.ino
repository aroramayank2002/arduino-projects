#include <max6675.h>

// set pin configration
int thermoSO = 6;
int thermoCS = 7;
int thermoCLK = 8;
// make object of max6675 named thermocouple
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);

void setup() {
  Serial.begin(9600);
  Serial.println("MAX6675 Test Processing...");
  // give some time to stable sensor
  delay(500);
}

void loop() {
  float celsius = thermocouple.readCelsius();
  float fahrenheit = thermocouple.readFahrenheit();

  // सीरियल मॉनिटर पर डेटा प्रिंट करें
  Serial.print("Temperature: ");
  Serial.print(celsius);
  Serial.print(" C  |  ");
  Serial.print(fahrenheit);
  Serial.println(" F");

  // max6675 needs 250ms to read new temprature readings
  delay(1000);
}
