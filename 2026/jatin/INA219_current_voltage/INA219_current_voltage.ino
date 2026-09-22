#include <Wire.h>
#include <Adafruit_INA219.h> // add INA219 library

Adafruit_INA219 ina219;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  ina219.begin();
  }

void loop() {
  float shuntVoltage = 0; // main DC supply voltage - load par jane wali voltage
  float busVoltage = 0; // load voltage
  float current_mA = 0; // load current
  float power_mW = 0; // power (W) = Voltage * Current
  float loadVoltage = 0; // shunt voltage + bus voltage

  shuntVoltage = ina219.getShuntVoltage_mV();
  busVoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();

  loadVoltage = busVoltage + (shuntVoltage / 1000.0);
  
  Serial.print("Bus Voltage : ");
  Serial.print(busVoltage);
  Serial.println(" V");

  Serial.print("Shunt Voltage : ");
  Serial.print(shuntVoltage);
  Serial.println(" mV");

  Serial.print("Load Voltage : ");
  Serial.print(loadVoltage);
  Serial.println(" V");

  Serial.print("Current : ");
  Serial.print(current_mA);
  Serial.println(" mA");

  Serial.print("Power : ");
  Serial.print(power_mW);
  Serial.println(" mW");

  Serial.println("---------------------------------------");

  delay(2000);
}
