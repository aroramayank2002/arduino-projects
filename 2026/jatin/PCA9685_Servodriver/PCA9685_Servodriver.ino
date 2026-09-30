#include <Wire.h>                     // I2C library
#include <Adafruit_PWMServoDriver.h>  // PCA9685 Servo module library

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // PCA9685 module address

void setup() {
   Serial.begin(9600);
   Wire.begin();
   pwm.begin();
   pwm.setPWMFreq(50);  // Servo = 50 Hz

}

void loop() {
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
