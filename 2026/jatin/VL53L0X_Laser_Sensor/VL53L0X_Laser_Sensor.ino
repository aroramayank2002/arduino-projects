// I2c Based VL53L0X TOF(Time Of Flight) Sensor Testing Code Dated - 25/09/2026
// Sensor distance measure in mm and i converted into cm
// Distance display on serial monitor
// Maximum testing Distance = aprox 120 cm with excellent accuracy

#include <Wire.h> // I2C library
#include <VL53L0X.h> // Sensor library

VL53L0X sensor; // create object named "sensor"

void setup()
{
  Serial.begin(9600); // for Serial monitor 
  Wire.begin(); // I2C communication start
  sensor.init(); // initialize the sensor
  sensor.startContinuous(); // sensor read measurement countinuously
}

void loop()
{
  int distance = sensor.readRangeContinuousMillimeters(); // Sensor read distance in mm
  float Distance = distance/10.00; // distance convert into cm

  Serial.print("Distance: ");
  Serial.print(Distance);
  Serial.println(" cm");

  delay(1000);
}
