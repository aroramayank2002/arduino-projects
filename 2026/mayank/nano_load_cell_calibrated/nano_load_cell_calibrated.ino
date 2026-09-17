
/*
I have 2 programs.
1. To find weight using weight sensor.
2. Turn servo on startup that opens a valve to pour rice for few seconds, then closes it.
   The same can be repeated with a button.

I want to combine the functionality like:

1. Nano checks in memory if a value is stored, it reads it.
2. Otherwise it starts in programming mode, where inbuild led keeps blinking.
3. We pour rice on boul placed on weight sensor. 
4. Then press button and value is stored in memory.
5. Program goes to run mode.
6. Here it checks if the weight is greater then value stored in memory then it turns servo to 12 degree (closed).
7. If not it turns the servo to 66 degree ( open ), so rice could pour.
8. Upon restart if value is found in memory. It will check weight and if weight is less it will open servo to 66 degree until weight crossed the stored value.
9. Upon restart before above step, there is a 5 seconds timer. If within this timer button is pressed.
10. Arduino goes to programming mode so one could reset the weight value.
11. All button presses should be indicated by led blink.
12. Led should keep blinking when in programming mode. 
13. Led should turn on when servo is at 66 degree.
14. After weight is reached and servo is back to 12 degree.
15. Button should work as toggle switch to turn servo back and forth ( 66 degree to 12 degree). For manual pouring of rice.
16. Add these comments in the code wherever they are implemented.
*/

#include <HX711.h>
#include <Servo.h>
#include <EEPROM.h>

// --------------------------------------------------
// Pin definitions
// --------------------------------------------------

#define HX711_DOUT 12
#define HX711_SCK  11

#define SERVO_PIN 3
#define BUTTON_PIN 2
#define LED_PIN LED_BUILTIN

// --------------------------------------------------
// HX711
// --------------------------------------------------

HX711 scale;

float calibration_factor = -1080.20;

// --------------------------------------------------
// Servo positions
// --------------------------------------------------

#define OPEN_ANGLE       66
#define HALF_OPEN_ANGLE  39
#define CLOSED_ANGLE     12

// When approximately 40g is left, slow down the pouring
#define SLOWDOWN_WEIGHT 40.0

// Maximum time allowed for servo to reach its target.
// Weight checking continues during this time.
#define SERVO_MOVE_TIME 2500

// --------------------------------------------------
// Startup button window
// --------------------------------------------------

#define STARTUP_WINDOW 5000

// LED blink interval.
// 250 ms ON + 250 ms OFF = 2 cycles per second.
#define LED_BLINK_INTERVAL 250

// --------------------------------------------------
// Button debounce
// --------------------------------------------------

#define BUTTON_DEBOUNCE 50

// --------------------------------------------------
// EEPROM
// --------------------------------------------------

#define EEPROM_MAGIC 0xABCD1234UL

struct StoredData {
  unsigned long magic;
  float targetWeight;
};

StoredData storedData;

// --------------------------------------------------
// Program modes
// --------------------------------------------------

enum ProgramMode {
  STARTUP,
  PROGRAMMING,
  RUNNING
};

ProgramMode mode;

// --------------------------------------------------
// Variables
// --------------------------------------------------

float targetWeight = 0;

bool servoAttached = false;
int currentServoPosition = CLOSED_ANGLE;

bool ledState = false;
unsigned long lastLedBlink = 0;

bool lastButtonState = HIGH;
unsigned long lastButtonChange = 0;

bool pouringFinished = false;

// --------------------------------------------------
// Servo object
// --------------------------------------------------

Servo valveServo;

// --------------------------------------------------
// Blink LED once
// --------------------------------------------------

void blinkLED() {

  digitalWrite(LED_PIN, HIGH);
  delay(100);

  digitalWrite(LED_PIN, LOW);
  delay(100);
}

// --------------------------------------------------
// Programming mode LED
//
// 12. Led should keep blinking when in programming mode.
// --------------------------------------------------

void updateProgrammingLED() {

  unsigned long now = millis();

  if (now - lastLedBlink >= LED_BLINK_INTERVAL) {

    lastLedBlink = now;

    ledState = !ledState;

    digitalWrite(LED_PIN, ledState);
  }
}

// --------------------------------------------------
// Start servo movement
//
// The servo is attached and commanded to the target
// position.
//
// IMPORTANT:
// This function does NOT wait 2.5 seconds.
// It returns immediately so the HX711 can continue
// being checked continuously.
// --------------------------------------------------

void startServoMove(int position) {

  Serial.print("SERVO: Moving to ");
  Serial.print(position);
  Serial.println(" degrees");

  if (!servoAttached) {

    valveServo.attach(SERVO_PIN);
    servoAttached = true;
  }

  valveServo.write(position);

  currentServoPosition = position;

  // ------------------------------------------------
  // 13. Led should turn on when servo is at 66 degree.
  // ------------------------------------------------

  if (position == OPEN_ANGLE) {

    digitalWrite(LED_PIN, HIGH);
  }

  // ------------------------------------------------
  // 14. After weight is reached and servo is back
  //     to 12 degree.
  // ------------------------------------------------

  else if (position == CLOSED_ANGLE) {

    digitalWrite(LED_PIN, LOW);
  }
}

// --------------------------------------------------
// Wait for servo to reach its target.
//
// This is used only when we actually need to wait
// for the servo, such as manual operation.
//
// After the maximum movement time, servo is detached
// to stop buzzing.
// --------------------------------------------------

void finishServoMove() {

  if (!servoAttached) {
    return;
  }

  unsigned long startTime = millis();

  while (millis() - startTime < SERVO_MOVE_TIME) {
    delay(10);
  }

  valveServo.detach();

  servoAttached = false;

  Serial.print("SERVO: Target ");
  Serial.print(currentServoPosition);
  Serial.println(" degrees reached");

  if (currentServoPosition == OPEN_ANGLE) {
    digitalWrite(LED_PIN, HIGH);
  }
  else if (currentServoPosition == CLOSED_ANGLE) {
    digitalWrite(LED_PIN, LOW);
  }
}

// --------------------------------------------------
// Move servo and wait.
//
// Used for initial/manual movements where continuous
// automatic pouring monitoring is not required.
// --------------------------------------------------

void moveServo(int position) {

  startServoMove(position);

  finishServoMove();
}

// --------------------------------------------------
// Read weight
// --------------------------------------------------

float getWeight() {

  float weight = scale.get_units(3);

  // Avoid negative values caused by small sensor drift.
  if (weight < 0) {
    weight = 0;
  }

  Serial.print("Weight: ");
  Serial.print(weight, 1);
  Serial.println(" g");

  return weight;
}

// --------------------------------------------------
// Check EEPROM for stored target weight
// --------------------------------------------------

bool loadStoredWeight() {

  EEPROM.get(0, storedData);

  if (storedData.magic == EEPROM_MAGIC) {

    targetWeight = storedData.targetWeight;

    Serial.print("Stored target weight found: ");
    Serial.print(targetWeight, 1);
    Serial.println(" g");

    return true;
  }

  Serial.println("No stored target weight found.");

  return false;
}

// --------------------------------------------------
// Store target weight in EEPROM
// --------------------------------------------------

void saveStoredWeight(float weight) {

  storedData.magic = EEPROM_MAGIC;
  storedData.targetWeight = weight;

  EEPROM.put(0, storedData);

  targetWeight = weight;

  Serial.print("Target weight stored: ");
  Serial.print(targetWeight, 1);
  Serial.println(" g");
}

// --------------------------------------------------
// Check button
//
// 11. All button presses should be indicated by led blink.
// --------------------------------------------------

bool buttonPressed() {

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {

    lastButtonChange = millis();
    lastButtonState = reading;
  }

  if ((millis() - lastButtonChange) > BUTTON_DEBOUNCE) {

    if (reading == LOW) {

      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }

      // Indicate button press with LED blink.
      blinkLED();

      return true;
    }
  }

  return false;
}

// --------------------------------------------------
// Programming mode
//
// 2. Otherwise it starts in programming mode,
//    where inbuild led keeps blinking.
//
// 3. We pour rice on boul placed on weight sensor.
//
// 4. Then press button and value is stored in memory.
//
// 5. Program goes to run mode.
// --------------------------------------------------

void programmingMode() {

  Serial.println();
  Serial.println("PROGRAMMING MODE");
  Serial.println("Place the desired amount of rice on the bowl.");
  Serial.println("Press the button to store the weight.");

  mode = PROGRAMMING;

  lastLedBlink = millis();
  ledState = false;

  while (true) {

    // ------------------------------------------------
    // 12. Led should keep blinking when in programming
    //     mode.
    // ------------------------------------------------

    updateProgrammingLED();

    if (buttonPressed()) {

      float weight = getWeight();

      saveStoredWeight(weight);

      digitalWrite(LED_PIN, LOW);

      Serial.println("Weight saved.");
      Serial.println("Switching to RUNNING mode.");

      mode = RUNNING;

      delay(500);

      break;
    }
  }
}

// --------------------------------------------------
// Automatic pouring
//
// Before servo opens for the first time, check weight.
//
// 6. Here it checks if the weight is greater then value
//    stored in memory then it turns servo to 12 degree
//    (closed).
//
// 7. If not it turns the servo to 66 degree ( open ),
//    so rice could pour.
//
// 8. Upon restart if value is found in memory. It will
//    check weight and if weight is less it will open
//    servo to 66 degree until weight crossed the
//    stored value.
//
// When approximately 40g remains, servo goes halfway
// between OPEN and CLOSED to slow down pouring.
//
// Servo closes when measured weight reaches or crosses
// the target weight.
//
// IMPORTANT:
// Weight is checked continuously while the servo is
// moving. There is no 2.5 second blocking delay here.
// --------------------------------------------------

void automaticPouring() {

  Serial.println();
  Serial.println("Checking weight before opening valve...");

  // ------------------------------------------------
  // Before servo opens for the first time to pour
  // item, check the weight.
  // ------------------------------------------------

  float weight = getWeight();

  // ------------------------------------------------
  // If target weight has already been reached,
  // keep valve closed.
  // ------------------------------------------------

  if (weight >= targetWeight) {

    Serial.println("Target weight already reached.");
    Serial.println("Keeping valve CLOSED.");

    moveServo(CLOSED_ANGLE);

    pouringFinished = true;

    return;
  }

  // ------------------------------------------------
  // Weight is below target.
  // Open valve.
  // ------------------------------------------------

  Serial.println("Weight is below target.");
  Serial.println("Opening valve.");

  startServoMove(OPEN_ANGLE);

  // ------------------------------------------------
  // Continue checking weight continuously while
  // rice is pouring and while servo is moving.
  // ------------------------------------------------

  bool halfOpenCommandSent = false;

  while (!pouringFinished) {

    // ----------------------------------------------
    // Continuously check weight.
    // ----------------------------------------------

    weight = getWeight();

    float remainingWeight = targetWeight - weight;

    Serial.print("Remaining: ");
    Serial.print(remainingWeight, 1);
    Serial.println(" g");

    // ----------------------------------------------
    // Target has been reached/crossed.
    //
    // Close immediately.
    // ----------------------------------------------

    if (weight >= targetWeight) {

      Serial.println("TARGET WEIGHT REACHED!");
      Serial.println("Closing valve immediately.");

      startServoMove(CLOSED_ANGLE);

      // Give the servo time to physically close.
      // This is the only movement delay, while the
      // decision to close happens immediately.
      unsigned long closeStart = millis();

      while (millis() - closeStart < SERVO_MOVE_TIME) {
        delay(10);
      }

      if (servoAttached) {
        valveServo.detach();
        servoAttached = false;
      }

      Serial.print("SERVO: Target ");
      Serial.print(CLOSED_ANGLE);
      Serial.println(" degrees reached");

      digitalWrite(LED_PIN, LOW);

      pouringFinished = true;

      break;
    }

    // ----------------------------------------------
    // Approximately 40g remaining.
    //
    // Move servo halfway between:
    //
    // OPEN   = 66 degrees
    // CLOSED = 12 degrees
    // HALF   = 39 degrees
    //
    // This slows the pouring.
    // ----------------------------------------------

    if (remainingWeight <= SLOWDOWN_WEIGHT &&
        !halfOpenCommandSent) {

      Serial.println("Approximately 40g remaining.");
      Serial.println("Slowing pouring.");

      startServoMove(HALF_OPEN_ANGLE);

      halfOpenCommandSent = true;
    }

    // ----------------------------------------------
    // Small delay only between weight readings.
    // This is NOT a servo movement delay.
    // ----------------------------------------------

    delay(50);
  }
}

// --------------------------------------------------
// Manual servo toggle
//
// 15. Button should work as toggle switch to turn
//     servo back and forth ( 66 degree to 12 degree).
//     For manual pouring of rice.
// --------------------------------------------------

void manualToggle() {

  if (currentServoPosition == CLOSED_ANGLE) {

    Serial.println("Manual button: OPEN valve.");

    moveServo(OPEN_ANGLE);

  } else {

    Serial.println("Manual button: CLOSE valve.");

    moveServo(CLOSED_ANGLE);
  }
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.println("Rice Dispenser Starting...");
  Serial.println();

  // ------------------------------------------------
  // HX711 connection
  //
  // Retry 3 times with a delay if HX711 is not found.
  // ------------------------------------------------

  bool hx711Ready = false;

  for (int attempt = 1; attempt <= 3; attempt++) {

    Serial.print("Checking HX711 - attempt ");
    Serial.print(attempt);
    Serial.println(" of 3");

    scale.begin(HX711_DOUT, HX711_SCK);

    if (scale.is_ready()) {

      hx711Ready = true;

      Serial.println("HX711 connected.");
      break;
    }

    Serial.println("HX711 not ready.");

    if (attempt < 3) {

      Serial.println("Retrying in 1 second...");
      delay(1000);
    }
  }

  // ------------------------------------------------
  // If HX711 cannot be connected after 3 attempts,
  // stop here.
  // ------------------------------------------------

  if (!hx711Ready) {

    Serial.println("ERROR: HX711 not found after 3 attempts!");

    while (1) {

      digitalWrite(LED_PIN, HIGH);
      delay(250);

      digitalWrite(LED_PIN, LOW);
      delay(250);
    }
  }

  // ------------------------------------------------
  // Tare and calibration
  // ------------------------------------------------

  Serial.println("Taring load cell...");
  delay(500);

  scale.tare();

  Serial.println("Tare complete.");


  scale.set_scale(calibration_factor);

  // ------------------------------------------------
  // Servo starts CLOSED.
  // ------------------------------------------------

  moveServo(CLOSED_ANGLE);

  // ------------------------------------------------
  // 1. Nano checks in memory if a value is stored,
  //    it reads it.
  // ------------------------------------------------

  bool storedWeightFound = loadStoredWeight();

  // ------------------------------------------------
  // 9. Upon restart before above step, there is a
  //    5 seconds timer. If within this timer button
  //    is pressed.
  //
  // LED blinks twice per second during this window.
  // ------------------------------------------------

  Serial.println();
  Serial.println("5 second startup window.");
  Serial.println("Press button to enter programming mode.");

  unsigned long startupStart = millis();

  lastLedBlink = millis();
  ledState = false;

  while (millis() - startupStart < STARTUP_WINDOW) {

    // ------------------------------------------------
    // LED blinks twice per second while waiting
    // for the button.
    // ------------------------------------------------

    updateProgrammingLED();

    if (buttonPressed()) {

      digitalWrite(LED_PIN, LOW);

      Serial.println("Button pressed during startup.");
      Serial.println("Entering programming mode.");

      programmingMode();

      return;
    }

    delay(10);
  }

  digitalWrite(LED_PIN, LOW);

  // ------------------------------------------------
  // 2. If no value is stored, enter programming mode.
  // ------------------------------------------------

  if (!storedWeightFound) {

    programmingMode();

    return;
  }

  // ------------------------------------------------
  // Stored value found.
  // Go to running mode.
  // ------------------------------------------------

  mode = RUNNING;

  Serial.println();
  Serial.println("RUNNING MODE");

  // ------------------------------------------------
  // 8. Upon restart if value is found in memory,
  //    check weight and open if weight is less.
  // ------------------------------------------------

  automaticPouring();
}

// --------------------------------------------------
// Main loop
// --------------------------------------------------

void loop() {

  // ------------------------------------------------
  // RUNNING MODE
  // ------------------------------------------------

  if (mode == RUNNING) {

    // ------------------------------------------------
    // 15. Button works as toggle switch for manual
    //     pouring of rice.
    // ------------------------------------------------

    if (buttonPressed()) {

      manualToggle();
    }
  }

  // ------------------------------------------------
  // PROGRAMMING MODE
  // ------------------------------------------------

  else if (mode == PROGRAMMING) {

    // ------------------------------------------------
    // 12. Led should keep blinking when in
    //     programming mode.
    // ------------------------------------------------

    updateProgrammingLED();
  }

  delay(10);
}
