#include <Wire.h>                     // I2C library
#include <Adafruit_PWMServoDriver.h>  // PCA9685 Servo module library
#include <max6675.h>  // MAX6675 Thermocouple library

// set pin configration of MAX6675
#define thermo_so 46
#define thermo_cs 47
#define thermo_sck 48

// set pin configration of L298 motor driver Motor 1 & 2
#define M1_IN1 45
#define M1_IN2 43
#define M2_IN1 41
#define M2_IN2 39

// set pin configration of L298 motor driver Motor 3 & 4
#define M3_IN1 38
#define M3_IN2 40
#define M4_IN1 42
#define M4_IN2 44


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // PCA9685 module address
MAX6675 thermocouple(thermo_sck, thermo_cs, thermo_so); // make object of max6675

void setup() {
   Serial.begin(9600);
   Wire.begin();
   pwm.begin();
   pwm.setPWMFreq(50);  // Servo = 50 Hz

  // define L298 Motor Driver Pin Configration
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);
  pinMode(M3_IN1, OUTPUT);
  pinMode(M3_IN2, OUTPUT);
  pinMode(M4_IN1, OUTPUT);
  pinMode(M4_IN2, OUTPUT);

  Serial.println("===== MENU =====");
  Serial.println("1 - L298 Motor Driver Testing");
  Serial.println("2 - MAX6675 Thermocouple Testing");
  Serial.println("3 - PCA9685 Servo Motor Testing");

}

void loop() {
  if (Serial.available() > 0) {

    char command = Serial.read();

    switch (command) {

      case '1':
        function1();
        break;
        
        case '2':
        function2();
        break;

      case '3':
        function3();
        break;

    }
  }
}

void function1(){
  Serial.println("L298 Motor Driver Testing...");
  // all motors ON for 5 seconds
  digitalWrite(M1_IN1, HIGH);
  digitalWrite(M1_IN2, LOW);
  digitalWrite(M2_IN1, HIGH);
  digitalWrite(M2_IN2, LOW);

  digitalWrite(M3_IN1, HIGH);
  digitalWrite(M3_IN2, LOW);
  digitalWrite(M4_IN1, HIGH);
  digitalWrite(M4_IN2, LOW);
  delay(5000);

  // all motors OFF
  digitalWrite(M1_IN1, LOW);
  digitalWrite(M1_IN2, LOW);
  digitalWrite(M2_IN1, LOW);
  digitalWrite(M2_IN2, LOW);

  digitalWrite(M3_IN1, LOW);
  digitalWrite(M3_IN2, LOW);
  digitalWrite(M4_IN1, LOW);
  digitalWrite(M4_IN2, LOW);

}

void function2(){
  Serial.println("MAX6675 Thermocouple Testing...");
  delay(1000);  // max6675 needs sometime to adjust temprature
  float celsius = thermocouple.readCelsius();        // temprature in degree celsius
  float fahrenheit = thermocouple.readFahrenheit();  // temprature in fahrenheit

  Serial.print("Temperature: ");
  Serial.print(celsius);
  Serial.print(" C  |  ");
  Serial.print(fahrenheit);
  Serial.println(" F");
}

void function3(){
  Serial.println("PCA9685 Servo Motor Testing...");
  for (int x = 150; x<=600; x++){ // 0 to 180 degree smooth rotation
    for(int y = 0; y<=15; y++){ // 0 to 15 all servo working 
    pwm.setPWM(y, 0, x);
    }
    delay(10);
  }

  delay(1000); // 1 sec. delay after complete 0 to 180 degree

  for (int x = 600; x>=150; x--){ // 180 to 0 degree smooth rotation
    for(int y = 0; y<=15; y++){ // 0 to 15 all servo working
    pwm.setPWM(y, 0, x);
    }
    delay(10);
  }

  delay(1000); // 1 sec. delay after complete 180 to 0 degree
}
