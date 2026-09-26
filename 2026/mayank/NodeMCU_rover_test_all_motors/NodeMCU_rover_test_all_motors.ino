// ============================================================
// NodeMCU 1.0 (ESP8266)
// 4 DC Motor Sequential Test
//
// M1: D0, D1
// M2: D2, D3
// M3: D5, D6
// M4: D7, D8
//
// Each motor:
//   Forward 2 seconds
//   Stop
//   Reverse 2 seconds
//   Stop
//
// Only ONE motor runs at a time.
// ============================================================

struct Motor {
  const char* name;
  uint8_t pin1;
  uint8_t pin2;
};

Motor motors[] = {
  {"M1", D0, D1},
  {"M2", D2, D3},
  {"M3", D5, D6},
  {"M4", D7, D8}
};

const int MOTOR_COUNT = 4;

const unsigned long RUN_TIME = 2000;
const unsigned long STOP_TIME = 500;


// ------------------------------------------------------------
// Stop motor
// ------------------------------------------------------------
void stopMotor(Motor &m) {
  digitalWrite(m.pin1, LOW);
  digitalWrite(m.pin2, LOW);
}


// ------------------------------------------------------------
// Run motor in one direction
// ------------------------------------------------------------
void runMotor(Motor &m, bool forward) {

  // Make sure motor is stopped before changing direction
  stopMotor(m);
  delay(100);

  Serial.print("START ");
  Serial.print(m.name);
  Serial.print(" | pins D");
  Serial.print(m.pin1);
  Serial.print(", D");
  Serial.print(m.pin2);
  Serial.print(" | ");

  if (forward) {
    Serial.println("FORWARD");
  } else {
    Serial.println("REVERSE");
  }

  // Set direction
  if (forward) {
    digitalWrite(m.pin1, HIGH);
    digitalWrite(m.pin2, LOW);
  } else {
    digitalWrite(m.pin1, LOW);
    digitalWrite(m.pin2, HIGH);
  }

  // Run for 2 seconds
  delay(RUN_TIME);

  // Stop
  stopMotor(m);

  Serial.print("END   ");
  Serial.print(m.name);
  Serial.print(" | pins D");
  Serial.print(m.pin1);
  Serial.print(", D");
  Serial.print(m.pin2);
  Serial.println(" | STOP");

  delay(STOP_TIME);
}


// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" NODEMCU 1.0 - 4 MOTOR TEST");
  Serial.println("================================");

  Serial.println("M1 = D0, D1");
  Serial.println("M2 = D2, D3");
  Serial.println("M3 = D5, D6");
  Serial.println("M4 = D7, D8");
  Serial.println();

  // Configure all motor pins
  for (int i = 0; i < MOTOR_COUNT; i++) {

    pinMode(motors[i].pin1, OUTPUT);
    pinMode(motors[i].pin2, OUTPUT);

    stopMotor(motors[i]);
  }

  Serial.println("All motors stopped.");
  Serial.println("Starting...");
}


// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------
void loop() {

  for (int i = 0; i < MOTOR_COUNT; i++) {

    Serial.println();
    Serial.print("========== ");
    Serial.print(motors[i].name);
    Serial.println(" ==========");

    // Forward 2 seconds
    runMotor(motors[i], true);

    // Reverse 2 seconds
    runMotor(motors[i], false);

    Serial.print(motors[i].name);
    Serial.println(" COMPLETE");

    delay(500);
  }

  Serial.println();
  Serial.println("================================");
  Serial.println(" ALL MOTORS COMPLETE");
  Serial.println("================================");

  delay(2000);
  
}