// board: LOLIN(WEMOS) D1 R2 & mini
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SDA_PIN D2
#define SCL_PIN D1

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// =====================================================
// SG90 SETTINGS
// =====================================================
#define SERVOMIN 102    // ~500 us
#define SERVOMAX 492    // ~2400 us

// =====================================================
// MG946R ALTERNATIVE SETTINGS
// Uncomment these and comment the SG90 values above
// if you are testing MG946R servos.
//
// Typical MG946R pulse range:
// ~500 us  -> 0°
// ~2500 us -> 180°
// PCA9685 at 50 Hz:
// 500 us  ≈ 102
// 2500 us ≈ 512
//
// #define SERVOMIN 102
// #define SERVOMAX 512
//
// More conservative MG946R range:
// #define SERVOMIN 123    // ~600 us
// #define SERVOMAX 492    // ~2400 us
// =====================================================

#define NUM_CHANNELS 16

void setServo(uint8_t channel, int angle) {
  angle = constrain(angle, 10, 170);

  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);

  pwm.setPWM(channel, 0, pulse);
}

void setAllServos(int angle) {
  for (int channel = 0; channel < NUM_CHANNELS; channel++) {
    setServo(channel, angle);
  }
}

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pwm.begin();
  pwm.setPWMFreq(50);

  delay(500);

  Serial.println("PCA9685 - All 16 Servo Channels");
}

void loop() {

  Serial.println("Moving 10 -> 170");

  for (int angle = 10; angle <= 170; angle += 2) {
    setAllServos(angle);
    delay(20);
  }

  delay(500);

  Serial.println("Moving 170 -> 10");

  for (int angle = 170; angle >= 10; angle -= 2) {
    setAllServos(angle);
    delay(20);
  }

  delay(500);
}