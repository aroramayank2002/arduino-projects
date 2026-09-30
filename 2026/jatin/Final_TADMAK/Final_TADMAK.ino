String function = "";
// MQ gas sensor pins
#define mq_gasD 13
#define mq_gasA A0
// Read Switch pins
#define read_in 12
#define read_out 2
// L298N Motor Driver Pins
#define motor_pwm 3
#define motor_out1 4
#define motor_out2 5
// PCA9685 Servo Motor Driver
#include <Wire.h>                                             // I2C library
#include <Adafruit_PWMServoDriver.h>                          // PCA9685 Servo module library
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // PCA9685 module address
// MAX6675 Thermocouple Sensor
#include <max6675.h>
#define thermoSO 6  // set pin configration
#define thermoCS 7
#define thermoCLK 8
MAX6675 thermocouple(thermoCLK, thermoCS, thermoSO);  // make object of max6675 named thermocouple
// SSD1306 OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128                                                   // Define screen dimensions for the OLED panel
#define SCREEN_HEIGHT 64                                                   // Define screen dimensions for the OLED panel
#define OLED_RESET -1                                                      // The parameter -1 indicates that the display does not share an external reset pin
#define SCREEN_ADDRESS 0x3C                                                // Typical I2C address for 0.96" OLED modules
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // create object named display
// HC-SR04 Ultrasonic Sensor
const int trigPin = 9;  // ultrasonic sensor's pin
const int echoPin = 10;
// BH-1750 Light(lux) Sensor
#include <BH1750.h>
BH1750 lightMeter;  // create an object named lightMeter
// TTP-223 Touch Switch
#define TTP_OUT 22
#define TTP_IN A1
int TTP_ON_OFF = 0;
// Rain(water) Sensor
#define alarm 23
#define rain_sensor_IN A2
// RCWL-0516 Radar Sensor
#define RADAR_IN 24
#define RADAR_OUT 25
// VL53L0X Laser Distance Sensor
#include <VL53L0X.h>  // Sensor library
VL53L0X sensor;       // create object named "sensor"
// INA219 Current Sensor
#include <Adafruit_INA219.h>  // add INA219 library
Adafruit_INA219 ina219;
// HW-220 Tem+Hum Sensor
#include "Adafruit_HTU21DF.h"
Adafruit_HTU21DF htu = Adafruit_HTU21DF();  // Initialize the sensor
// DHT-11 Tem+Hum Sensor
#include <DHT.h>       // Requires the "DHT sensor library" by Adafruit
#define DHTPIN 26      // use degital pin no 26 of Arduino Mega
#define DHTTYPE DHT11  // Hum DHT11 sensor ka use kar rahe hain
DHT dht(DHTPIN, DHTTYPE);

int d = 0, e = 0, a = 0, b = 0, c = 0, f = 0, g = 0, h = 0, i = 0, j = 0, k = 0, l = 0, m = 0, n = 0, o = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  // MQ gas sensor
  pinMode(mq_gasD, INPUT_PULLUP);
  // Read Switch
  pinMode(read_in, INPUT);
  pinMode(read_out, OUTPUT);
  // L298N Motor Driver
  pinMode(motor_pwm, OUTPUT);
  pinMode(motor_out1, OUTPUT);
  pinMode(motor_out2, OUTPUT);
  // PCA9685 Servo Motor Driver
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);  // Servo = 50 Hz
  // SSD1306 OLED
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);  // Initialize the I2C communication and display
  // HC-SR04 Ultrasonic Sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // BM-1750 Light(lux) Sensor
  lightMeter.begin();  // start the sensor
  // TTP-223 Touch Switch Sensor
  pinMode(TTP_OUT, OUTPUT);
  // Rain(water) Sensor
  pinMode(alarm, OUTPUT);
  // RCWL-0516 Radar Sensor
  pinMode(RADAR_OUT, OUTPUT);
  pinMode(RADAR_IN, INPUT);
  // VL53L0X Laser Distance Sensor
  sensor.init();             // initialize the sensor
  sensor.startContinuous();  // sensor read measurement countinuously
  // INA219 Current Sensor
  ina219.begin();
  // HW-220 Tem+Hum Sensor
  htu.begin();
  // DHT-11 Tem+Hum Sensor
  dht.begin();

  Serial.println("===== MENU =====");
  Serial.println("1 - MQ GAS MODULE Testing");
  Serial.println("2 - Read Switch Testing");
  Serial.println("3 - L298N Motor Driver Testing");
  Serial.println("4 - PCA9685 Servo Motor Testing");
  Serial.println("5 - MAX6675 Thermocouple Testing");
  Serial.println("6 - SSD1306 OLED Testing");
  Serial.println("7 - HC-SR04 Ultrasonic Sensor Testing");
  Serial.println("8 - BH-1750 Light(lux) Sensor Testing");
  Serial.println("9 - TTP-223 Touch Switch Testing");
  Serial.println("10 - Rain(water) Sensor Testing");
  Serial.println("11 - RCWL-0516 Radar Sensor Testing");
  Serial.println("12 - VL53L0X Laser Distance Sensor Testing");
  Serial.println("13 - INA219 Current Sensor Testing");
  Serial.println("14 - HW-220 Tem+Hum Sensor Testing");
  Serial.println("15 - DHT-11 Tem+Hum Sensor Testing");
}

void loop() {

  function = Serial.readStringUntil('\n'); // store the number in string type function

  if (function == "1" || d == 1) {
    Serial.println("MQ GAS MODULE Testing...");
    float MQ = analogRead(mq_gasA);
    Serial.println(MQ);  // Analog output

    if (digitalRead(mq_gasD) < 1) {
      Serial.println("MQ Activated!");  // Digital output
      delay(1000);
    }
    d = 1;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "2" || e == 1) {
    Serial.println("Read Switch Testing...");
    if (digitalRead(read_in) < 1) {
      digitalWrite(read_out, HIGH);
    } else {
      digitalWrite(read_out, LOW);
    }
    d = 0;
    e = 1;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "3" || a == 1) {
    Serial.println("L298N Motor Driver Testing...");
    digitalWrite(motor_out1, HIGH);
    digitalWrite(motor_out2, LOW);
    for (int x = 70; x <= 255; x++) {
      analogWrite(3, x);
      delay(2);
    }
    for (int y = 255; y >= 70; y--) {
      analogWrite(3, y);
      delay(2);
    }
    d = 0;
    e = 0;
    a = 1;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "4" || b == 1) {
    Serial.println("PCA9685 Servo Motor Testing...");
    for (int x = 150; x <= 600; x++) {  // 0 to 180 degree smooth rotation
      for (int y = 0; y <= 15; y++) {   // 0 to 15 all servo working
        pwm.setPWM(y, 0, x);
      }
      delay(1);
    }

    delay(10);  // delay after complete 0 to 180 degree

    for (int x = 600; x >= 150; x--) {  // 180 to 0 degree smooth rotation
      for (int y = 0; y <= 15; y++) {   // 0 to 15 all servo working
        pwm.setPWM(y, 0, x);
      }
      delay(1);
    }

    delay(10);  // delay after complete 180 to 0 degree
    d = 0;
    e = 0;
    a = 0;
    b = 1;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "5" || c == 1) {
    Serial.println("MAX6675 Thermocouple Testing...");
    delay(500);
    float celsius = thermocouple.readCelsius();
    float fahrenheit = thermocouple.readFahrenheit();

    // print data on serial monitor
    Serial.print("Temperature: ");
    Serial.print(celsius);
    Serial.print(" C  |  ");
    Serial.print(fahrenheit);
    Serial.println(" F");

    // max6675 needs 250ms to read new temprature readings
    delay(1000);
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 1;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "6" || f == 1) {
    Serial.println("SSD1306 OLED Testing...");
    display.clearDisplay();               // Clear the buffer initialization screen (Adafruit splash screen)
    display.setTextSize(2);               // Normal 2:1 pixel scale (readable size)
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(42, 0);             // Set position coordinate (X, Y)
    display.println("OLED");              // Write message to local display buffer
    display.setCursor(20, 16);            // Set position coordinate (X, Y)
    display.println("SSD1306");           // Write message to local display buffer

    display.setCursor(20, 33);  // Set position coordinate (X, Y)
    display.println("TESTING");

    display.setCursor(25, 50);  // Set position coordinate (X, Y)
    display.println("128*64");
    display.display();  // Display massage on OLED display
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 1;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "7" || g == 1) {
    Serial.println("HC-SR04 Ultrasonic Sensor Testing...");
    long duration;
    float distance;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    Serial.println(distance);

    delay(1000);
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 1;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "8" || h == 1) {
    Serial.println("BH-1750 Light(lux) Sensor Testing...");
    float lux = lightMeter.readLightLevel();  // Read LUX value from sensor
    Serial.println(lux);
    delay(2000);
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 1;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "9" || i == 1) {
    Serial.println("TTP-223 Touch Switch Testing...");
    if (analogRead(TTP_IN) > 100 && TTP_ON_OFF == 0) {
      digitalWrite(TTP_OUT, HIGH);
      delay(1000);
      TTP_ON_OFF = 1;
    }
    if (analogRead(TTP_IN) > 100 && TTP_ON_OFF == 1) {
      digitalWrite(TTP_OUT, LOW);
      delay(1000);
      TTP_ON_OFF = 0;
    }
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 1;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "10" || j == 1) {
    Serial.println("Rain(water) Sensor Testing...");
    int moisture = analogRead(rain_sensor_IN);
    if (moisture > 100) {
      digitalWrite(alarm, HIGH);
      Serial.println("Raining !");
    } else {
      digitalWrite(alarm, LOW);
    }
    delay(2000);  // test in every 2 seconds
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 1;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "11" || k == 1) {
    Serial.println("RCWL-0516 Radar Sensor Testing...");
    if (digitalRead(RADAR_IN) > 0) {
      digitalWrite(RADAR_OUT, HIGH);
      Serial.println("MOTION DETECTED!");
      delay(2000);
    } else {
      digitalWrite(RADAR_OUT, LOW);
    }
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 1;
    l = 0;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "12" || l == 1) {
    Serial.println("VL53L0X Laser Distance Sensor Testing...");
    int distance = sensor.readRangeContinuousMillimeters();  // Sensor read distance in mm
    float Distance = distance / 10.00;                       // distance convert into cm

    Serial.print("Distance: ");
    Serial.print(Distance);
    Serial.println(" cm");

    delay(1000);
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 1;
    m = 0;
    n = 0;
    o = 0;
  }

  if (function == "13" || m == 1) {
    Serial.println("INA219 Current Sensor Testing...");
    float shuntVoltage = 0;  // main DC supply voltage - load par jane wali voltage
    float busVoltage = 0;    // load voltage
    float current_mA = 0;    // load current
    float power_mW = 0;      // power (W) = Voltage * Current
    float loadVoltage = 0;   // shunt voltage + bus voltage

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
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 1;
    n = 0;
    o = 0;
  }

  if (function == "14" || n == 1) {
    Serial.println("HW-220 Tem+Hum Sensor Testing...");
    float temp = htu.readTemperature();
    float rel_hum = htu.readHumidity();

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.print(" C | ");

    Serial.print("Humidity: ");
    Serial.print(rel_hum);
    Serial.println(" %");

    delay(2000);  // Wait 2 seconds before the next reading
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 1;
    o = 0;
  }

  if (function == "15" || o == 1) {
    Serial.println("DHT-11 Tem+Hum Sensor Testing...");
    delay(2000);  // delay 2 seconds for read next reading

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();  // Celsius mein reading

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C | ");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    d = 0;
    e = 0;
    a = 0;
    b = 0;
    c = 0;
    f = 0;
    g = 0;
    h = 0;
    i = 0;
    j = 0;
    k = 0;
    l = 0;
    m = 0;
    n = 0;
    o = 1;
  }
}
