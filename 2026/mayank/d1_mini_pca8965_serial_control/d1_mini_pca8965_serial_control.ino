// board: LOLIN(WEMOS) D1 R2 & mini
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define SDA_PIN D2
#define SCL_PIN D1

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// =====================================================
// SERVO SETTINGS
// =====================================================

// SG90
#define SERVOMIN 102    // ~500 us
#define SERVOMAX 492    // ~2400 us

// MG946R alternative:
// #define SERVOMIN 102    // ~500 us
// #define SERVOMAX 512    // ~2500 us

// Conservative MG946R:
// #define SERVOMIN 123    // ~600 us
// #define SERVOMAX 492    // ~2400 us

#define MIN_ANGLE 10
#define MAX_ANGLE 170
#define START_ANGLE 90

#define NUM_SERVOS 6

// Remember last position of each servo
int servoPosition[NUM_SERVOS];

// Currently selected servo
int selectedServo = -1;


// =====================================================
// SET SERVO ANGLE
// =====================================================

void setServoAngle(int channel, int angle) {

  angle = constrain(angle, MIN_ANGLE, MAX_ANGLE);

  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);

  pwm.setPWM(channel, 0, pulse);
}


// =====================================================
// TURN OFF SERVO SIGNAL
// =====================================================

void stopServo(int channel) {
  pwm.setPWM(channel, 0, 0);
}


// =====================================================
// PRINT MENU
// =====================================================

void printMenu() {

  Serial.println();
  Serial.println("==============================");
  Serial.println(" PCA9685 SERVO CONTROL");
  Serial.println("==============================");
  Serial.println();
  Serial.println("Enter 1-6 to select servo");
  Serial.println();
  Serial.println("1 = CH0"); //base 10-170
  Serial.println("2 = CH1"); //boom 30-170, default 90
  Serial.println("3 = CH2"); //arm 10-170, default 94
  Serial.println("4 = CH3"); //wrist pitch 10-170, default 62
  Serial.println("5 = CH4"); //wrist roll 20-170, default 74
  Serial.println("6 = CH5"); //grip 80-138, default 110
  Serial.println();
  Serial.println("q = -2 degrees");
  Serial.println("a = +2 degrees");
  Serial.println("e = exit");
  Serial.println();
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  pwm.begin();
  pwm.setPWMFreq(50);

  delay(100);

  // No servo signal at startup
  for (int i = 0; i < 16; i++) {
    pwm.setPWM(i, 0, 0);
  }

  // Initial position for each servo
  for (int i = 0; i < NUM_SERVOS; i++) {
    servoPosition[i] = START_ANGLE;
  }

  printMenu();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // SERVO SELECTION MENU
  // ===================================================

  if (selectedServo == -1) {

    if (Serial.available()) {

      char command = Serial.read();

      // Select servo 1-6
      if (command >= '1' && command <= '6') {

        selectedServo = command - '1';

        Serial.println();
        Serial.print("Selected servo ");
        Serial.println(selectedServo + 1);

        Serial.print("PCA9685 channel: ");
        Serial.println(selectedServo);

        Serial.print("Current position: ");
        Serial.print(servoPosition[selectedServo]);
        Serial.println(" degrees");

        Serial.println();
        Serial.println("q = -2 degrees");
        Serial.println("a = +2 degrees");
        Serial.println("e = exit");
        Serial.println();

        // Start signal for selected servo
        // at its remembered position
        setServoAngle(
          selectedServo,
          servoPosition[selectedServo]
        );
      }
    }

    return;
  }


  // ===================================================
  // SERVO CONTROL
  // ===================================================

  if (Serial.available()) {

    char command = Serial.read();


    // -------------------------------------------------
    // q = MOVE -2 DEGREES
    // -------------------------------------------------

    if (command == 'q') {

      servoPosition[selectedServo] -= 2;

      servoPosition[selectedServo] =
        constrain(
          servoPosition[selectedServo],
          MIN_ANGLE,
          MAX_ANGLE
        );

      setServoAngle(
        selectedServo,
        servoPosition[selectedServo]
      );

      Serial.print("Servo ");
      Serial.print(selectedServo + 1);
      Serial.print(": ");
      Serial.print(servoPosition[selectedServo]);
      Serial.println(" degrees");
    }


    // -------------------------------------------------
    // a = MOVE +2 DEGREES
    // -------------------------------------------------

    else if (command == 'a') {

      servoPosition[selectedServo] += 2;

      servoPosition[selectedServo] =
        constrain(
          servoPosition[selectedServo],
          MIN_ANGLE,
          MAX_ANGLE
        );

      setServoAngle(
        selectedServo,
        servoPosition[selectedServo]
      );

      Serial.print("Servo ");
      Serial.print(selectedServo + 1);
      Serial.print(": ");
      Serial.print(servoPosition[selectedServo]);
      Serial.println(" degrees");
    }


    // -------------------------------------------------
    // e = EXIT
    // -------------------------------------------------

    else if (command == 'e') {

      Serial.println();
      Serial.print("Servo ");
      Serial.print(selectedServo + 1);
      Serial.print(" position saved at ");
      Serial.print(servoPosition[selectedServo]);
      Serial.println(" degrees.");

      // Stop PWM signal
      stopServo(selectedServo);

      // Return to selection menu
      selectedServo = -1;

      printMenu();
    }
  }
}