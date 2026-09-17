/*
Behavior
| Situation                        | Result                            |
| -------------------------------- | --------------------------------- |
| First-ever startup               | Programming mode, LED blinks      |
| Stored value exists              | 5-second startup window           |
| Button during first 5 sec        | Programming mode                  |
| No button during first 5 sec     | Automatic run mode                |
| Short button press, valve closed | Valve opens, manual mode          |
| Short button press, valve open   | Valve closes, automatic mode      |
| Hold button 3 sec                | Programming mode                  |
| Programming mode + button        | Current weight saved              |
| Weight below `target - 5g`       | Valve opens automatically         |
| Weight reaches target            | Valve closes                      |
| Weight fluctuates around target  | Hysteresis prevents rapid cycling |
| Valve open                       | LED ON                            |
| Valve closed                     | LED OFF                           |
| Programming mode                 | LED continuously blinks           |

*/

#include <Servo.h>
#include <HX711.h>
#include <EEPROM.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================

const byte HX711_DOUT = 12;
const byte HX711_SCK = 11;

const byte SERVO_PIN = 3;
const byte BUTTON_PIN = 2;
const byte LED_PIN = LED_BUILTIN;


// ============================================================
// SERVO POSITIONS
// ============================================================

const int OPEN_ANGLE = 66;    // Valve OPEN
const int CLOSED_ANGLE = 12;  // Valve CLOSED


// ============================================================
// HX711 CALIBRATION
// ============================================================
//
// Calibration measurements:
//
// 0 g   =  227752
// 351 g = -151397
//
// Calibration factor:
//
// (-151397 - 227752) / 351
// = -1080.20
//
// scale.tare() automatically handles the zero offset.
//

const float CALIBRATION_FACTOR = -1080.20;


// ============================================================
// WEIGHT HYSTERESIS
// ============================================================
//
// Hysteresis is used while the valve is filling.
//
// Example:
//
// Target = 351 g
// Hysteresis = 5 g
//
// The valve closes when target is reached.
//
// IMPORTANT:
// Once the target has been reached, the automatic dispensing
// cycle is COMPLETE.
//
// The valve will NOT reopen if rice is subsequently removed.
//
// A new automatic cycle only starts after RESET / POWER ON.
//

const float HYSTERESIS = 5.0;


// ============================================================
// EEPROM SETTINGS
// ============================================================
//
// EEPROM stores:
//
// Address 0:
//     Magic number
//
// Address 4:
//     Target weight in grams
//

const int EEPROM_MAGIC_ADDRESS = 0;
const int EEPROM_WEIGHT_ADDRESS = 4;

const uint32_t EEPROM_MAGIC = 0xABCD1234;


// ============================================================
// BUTTON SETTINGS
// ============================================================

// Hold button for 3 seconds during normal operation to enter
// programming mode.

const unsigned long LONG_PRESS_TIME = 3000;


// First 5 seconds after startup can be used to enter
// programming mode.

const unsigned long STARTUP_WINDOW = 5000;


// Button debounce time.

const unsigned long BUTTON_DEBOUNCE = 50;


// ============================================================
// LED SETTINGS
// ============================================================

const unsigned long LED_BLINK_INTERVAL = 500;


// ============================================================
// OBJECTS
// ============================================================

HX711 scale;
Servo doorServo;


// ============================================================
// GLOBAL VARIABLES
// ============================================================

float targetWeight = 0;


// True when programming mode is active.

bool programmingMode = false;


// True when servo is at OPEN_ANGLE.

bool valveOpen = false;


// True when user has manually opened the valve.
//
// In manual mode, the weight sensor does NOT automatically
// close the valve.

bool manualMode = false;


// ============================================================
// AUTOMATIC DISPENSING CYCLE
// ============================================================
//
// This is the important new variable.
//
// false:
//     Automatic dispensing is still allowed.
//
// true:
//     Automatic dispensing has already completed.
//     Removing rice will NOT reopen the valve.
//
// This variable resets to false whenever Arduino restarts.
//

bool automaticCycleComplete = false;


// ============================================================
// BUTTON VARIABLES
// ============================================================

bool lastButtonState = HIGH;

bool buttonCurrentlyPressed = false;

unsigned long buttonPressStart = 0;

unsigned long lastButtonChange = 0;


// ============================================================
// LED VARIABLES
// ============================================================

unsigned long lastLEDBlink = 0;

bool ledState = false;


// ============================================================
// OPEN VALVE
// ============================================================

void openValve() {
  doorServo.write(OPEN_ANGLE);

  valveOpen = true;

  // LED is ON whenever valve is open.
  digitalWrite(LED_PIN, HIGH);
}


// ============================================================
// CLOSE VALVE
// ============================================================

void closeValve() {
  doorServo.write(CLOSED_ANGLE);

  valveOpen = false;

  // LED OFF whenever valve is closed.
  digitalWrite(LED_PIN, LOW);
}


// ============================================================
// BUTTON PRESS LED INDICATION
// ============================================================

void blinkButton() {
  digitalWrite(LED_PIN, HIGH);

  delay(150);

  // If valve is open, LED must remain ON.
  if (valveOpen) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}


// ============================================================
// EEPROM - CHECK STORED VALUE
// ============================================================

bool hasStoredWeight() {
  uint32_t magic;

  EEPROM.get(EEPROM_MAGIC_ADDRESS, magic);

  return (magic == EEPROM_MAGIC);
}


// ============================================================
// EEPROM - READ TARGET WEIGHT
// ============================================================

void readStoredWeight() {
  EEPROM.get(EEPROM_WEIGHT_ADDRESS, targetWeight);

  Serial.print("Stored target weight: ");
  Serial.print(targetWeight, 1);
  Serial.println(" g");
}


// ============================================================
// EEPROM - SAVE TARGET WEIGHT
// ============================================================

void saveWeight(float weight) {
  EEPROM.put(EEPROM_MAGIC_ADDRESS, EEPROM_MAGIC);

  EEPROM.put(EEPROM_WEIGHT_ADDRESS, weight);

  targetWeight = weight;

  Serial.println();
  Serial.println("================================");
  Serial.println("TARGET WEIGHT SAVED");
  Serial.print("Target: ");
  Serial.print(targetWeight, 1);
  Serial.println(" g");
  Serial.println("================================");
  Serial.println();
}


// ============================================================
// EEPROM - CLEAR TARGET
// ============================================================

void clearStoredWeight() {
  uint32_t invalidMagic = 0;

  EEPROM.put(EEPROM_MAGIC_ADDRESS, invalidMagic);

  Serial.println("Stored target weight cleared.");
}


// ============================================================
// ENTER PROGRAMMING MODE
// ============================================================

void enterProgrammingMode() {
  programmingMode = true;

  manualMode = false;

  // Always close valve when entering programming mode.
  closeValve();


  Serial.println();
  Serial.println("================================");
  Serial.println("PROGRAMMING MODE");
  Serial.println("================================");
  Serial.println();

  Serial.println("Put the desired amount of rice");
  Serial.println("in the bowl.");

  Serial.println();
  Serial.println("Press the button to save the weight.");
  Serial.println();


  // Start LED blinking.

  lastLEDBlink = millis();

  ledState = false;

  digitalWrite(LED_PIN, LOW);
}


// ============================================================
// PROGRAMMING MODE LOOP
// ============================================================

void programmingModeLoop() {
  // ----------------------------------------------------------
  // LED continuously blinks in programming mode.
  // ----------------------------------------------------------

  if (millis() - lastLEDBlink >= LED_BLINK_INTERVAL) {
    lastLEDBlink = millis();

    ledState = !ledState;

    digitalWrite(LED_PIN, ledState);
  }


  // ----------------------------------------------------------
  // Check button.
  // ----------------------------------------------------------

  bool buttonState = digitalRead(BUTTON_PIN);


  if (buttonState == LOW && lastButtonState == HIGH && millis() - lastButtonChange > BUTTON_DEBOUNCE) {
    lastButtonChange = millis();

    blinkButton();

    Serial.println();
    Serial.println("Button pressed.");
    Serial.println("Measuring weight...");

    delay(500);


    // --------------------------------------------------------
    // Measure current rice weight.
    // --------------------------------------------------------

    float measuredWeight = scale.get_units(10);

    Serial.print("Measured weight: ");
    Serial.print(measuredWeight, 1);
    Serial.println(" g");


    // --------------------------------------------------------
    // Store target weight in EEPROM.
    // --------------------------------------------------------

    saveWeight(measuredWeight);


    // --------------------------------------------------------
    // Programming finished.
    // --------------------------------------------------------

    programmingMode = false;

    manualMode = false;

    // A newly programmed target starts a NEW automatic cycle.

    automaticCycleComplete = false;


    closeValve();


    Serial.println();
    Serial.println("Programming complete.");
    Serial.println("Returning to automatic mode.");
    Serial.println();


    // --------------------------------------------------------
    // Wait for button release.
    // --------------------------------------------------------

    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }

    lastButtonState = HIGH;
  }


  lastButtonState = buttonState;
}


// ============================================================
// STARTUP 5-SECOND PROGRAMMING WINDOW
// ============================================================

bool checkStartupProgrammingWindow() {
  Serial.println();
  Serial.println("================================");
  Serial.println("STARTUP WINDOW: 5 SECONDS");
  Serial.println("Press button to reprogram.");
  Serial.println("================================");


  unsigned long startTime = millis();


  while (millis() - startTime < STARTUP_WINDOW) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      blinkButton();

      Serial.println();
      Serial.println("Startup button pressed.");

      // Remove old target.

      clearStoredWeight();


      // Enter programming mode.

      enterProgrammingMode();


      // Wait for button release.

      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }

      lastButtonState = HIGH;

      return true;
    }

    delay(10);
  }


  Serial.println("Startup window expired.");

  return false;
}


// ============================================================
// RUN MODE BUTTON HANDLING
// ============================================================
//
// SHORT PRESS:
//
//     Valve CLOSED -> OPEN
//     Valve OPEN   -> CLOSED
//
// A short press gives the user manual control.
//
// When manually opened:
//     Weight sensor does NOT close the valve.
//
// When manually closed:
//     System returns to automatic mode.
//
// LONG PRESS (3 seconds):
//
//     Enter programming mode.
//

void checkRunButton() {
  bool buttonState = digitalRead(BUTTON_PIN);


  // ==========================================================
  // BUTTON PRESSED
  // ==========================================================

  if (buttonState == LOW && lastButtonState == HIGH && millis() - lastButtonChange > BUTTON_DEBOUNCE) {
    lastButtonChange = millis();

    buttonCurrentlyPressed = true;

    buttonPressStart = millis();

    Serial.println("Button pressed.");
  }


  // ==========================================================
  // BUTTON HELD
  // ==========================================================

  if (buttonCurrentlyPressed && buttonState == LOW) {
    // --------------------------------------------------------
    // Check for 3-second long press.
    // --------------------------------------------------------

    if (millis() - buttonPressStart >= LONG_PRESS_TIME) {
      buttonCurrentlyPressed = false;

      Serial.println();
      Serial.println("3-second long press detected.");

      // ------------------------------------------------------
      // Enter programming mode.
      // ------------------------------------------------------

      clearStoredWeight();

      enterProgrammingMode();


      // ------------------------------------------------------
      // Wait for button release.
      // ------------------------------------------------------

      while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
      }

      lastButtonState = HIGH;

      return;
    }
  }


  // ==========================================================
  // BUTTON RELEASED
  // ==========================================================

  if (buttonState == HIGH && lastButtonState == LOW && buttonCurrentlyPressed) {
    buttonCurrentlyPressed = false;

    unsigned long pressDuration =
      millis() - buttonPressStart;


    // --------------------------------------------------------
    // SHORT PRESS
    // --------------------------------------------------------

    if (pressDuration < LONG_PRESS_TIME) {
      blinkButton();


      // ------------------------------------------------------
      // VALVE CLOSED -> OPEN
      // ------------------------------------------------------

      if (!valveOpen) {
        manualMode = true;

        openValve();

        Serial.println("SHORT PRESS:");
        Serial.println("Manual VALVE OPEN.");
        Serial.println("Weight control disabled.");
        Serial.println("Press button again to close.");
      }


      // ------------------------------------------------------
      // VALVE OPEN -> CLOSED
      // ------------------------------------------------------

      else {
        closeValve();

        // Return to automatic mode after manual close.

        manualMode = false;

        Serial.println("SHORT PRESS:");
        Serial.println("Manual VALVE CLOSED.");
        Serial.println("Returning to automatic mode.");
      }
    }
  }


  lastButtonState = buttonState;
}


// ============================================================
// AUTOMATIC WEIGHT CONTROL
// ============================================================
//
// This is a ONE-SHOT automatic cycle.
//
// On startup:
//
//     If weight < target:
//         Open valve.
//
//     When weight >= target:
//         Close valve.
//
//     Then:
//         automaticCycleComplete = true
//
// After that:
//
//     Removing rice does NOTHING.
//
// The valve will NOT automatically reopen.
//
// A new automatic cycle only happens after RESET / POWER ON.
//

void automaticWeightControl() {
  // ----------------------------------------------------------
  // If automatic cycle has already completed, do nothing.
  // ----------------------------------------------------------

  if (automaticCycleComplete) {
    return;
  }


  // ----------------------------------------------------------
  // Read current weight.
  // ----------------------------------------------------------

  float currentWeight = scale.get_units(5);


  Serial.print("Weight: ");
  Serial.print(currentWeight, 1);

  Serial.print(" g | Target: ");
  Serial.print(targetWeight, 1);

  Serial.println(" g | Mode: AUTO");


  // ----------------------------------------------------------
  // WEIGHT BELOW TARGET
  // ----------------------------------------------------------

  if (currentWeight < targetWeight) {
    // --------------------------------------------------------
    // Open valve.
    // --------------------------------------------------------

    if (!valveOpen) {
      openValve();

      Serial.println("Weight below target -> VALVE OPEN");
    }
  }


  // ----------------------------------------------------------
  // TARGET REACHED
  // ----------------------------------------------------------

  else {
    // --------------------------------------------------------
    // Close valve.
    // --------------------------------------------------------

    if (valveOpen) {
      closeValve();

      Serial.println("Target reached -> VALVE CLOSED");
    }


    // --------------------------------------------------------
    // IMPORTANT:
    //
    // Automatic cycle is now COMPLETE.
    //
    // Even if the rice is removed later, the valve will NOT
    // reopen.
    //
    // Only a RESET / POWER ON starts a new automatic cycle.
    // --------------------------------------------------------

    automaticCycleComplete = true;

    Serial.println("Automatic dispensing cycle COMPLETE.");
    Serial.println("Valve will remain closed until next reset/power-up.");
  }
}


// ============================================================
// RUN MODE
// ============================================================

void runModeLoop() {
  // ----------------------------------------------------------
  // Handle button.
  // ----------------------------------------------------------

  checkRunButton();


  // ----------------------------------------------------------
  // Manual mode:
  //
  // Do NOT allow weight sensor to control the valve.
  // ----------------------------------------------------------

  if (manualMode) {
    delay(50);

    return;
  }


  // ----------------------------------------------------------
  // Automatic mode.
  // ----------------------------------------------------------

  automaticWeightControl();

  delay(200);
}


// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(9600);


  Serial.println();
  Serial.println("================================");
  Serial.println("RICE DISPENSER");
  Serial.println("================================");


  // ==========================================================
  // PIN SETUP
  // ==========================================================

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_PIN, OUTPUT);


  // ==========================================================
  // SERVO SETUP
  // ==========================================================

  doorServo.attach(SERVO_PIN);

  // Start with valve closed.

  closeValve();



  // ==========================================================
  // HX711 SETUP WITH RETRIES
  // ==========================================================

  scale.begin(HX711_DOUT, HX711_SCK);

  bool hx711Ready = false;

  Serial.println("Initializing HX711...");

  // Try up to 5 times.
  // This allows extra time for the HX711 to power up.

  for (byte attempt = 1; attempt <= 5; attempt++) {
    Serial.print("HX711 attempt ");
    Serial.print(attempt);
    Serial.print(" / 5: ");

    if (scale.is_ready()) {
      Serial.println("READY");

      hx711Ready = true;
      break;
    }

    Serial.println("NOT READY");

    // Wait before trying again.
    delay(500);
  }


  // ==========================================================
  // HX711 FAILED AFTER ALL RETRIES
  // ==========================================================

  if (!hx711Ready) {
    Serial.println();
    Serial.println("ERROR: HX711 not responding after 5 attempts.");
    Serial.println("Check HX711 power, GND, DOUT and SCK.");
    Serial.println();

    // Keep retrying every 2 seconds instead of permanently
    // stopping the Arduino.

    while (!scale.is_ready()) {
      Serial.println("Retrying HX711...");

      digitalWrite(LED_PIN, HIGH);
      delay(200);

      digitalWrite(LED_PIN, LOW);
      delay(1800);
    }

    Serial.println("HX711 is now READY!");
  }



  // ==========================================================
  // TARE
  // ==========================================================
  //
  // IMPORTANT:
  //
  // Place the EMPTY bowl on the load cell before powering
  // the Arduino.
  //
  // The bowl weight will then be treated as zero.
  //

  Serial.println();
  Serial.println("Taring scale...");
  Serial.println("Make sure the bowl is empty.");

  delay(2000);

  scale.set_scale(CALIBRATION_FACTOR);

  scale.tare(10);

  Serial.println("Tare complete.");


  // ==========================================================
  // EEPROM CHECK
  // ==========================================================

  if (hasStoredWeight()) {
    // --------------------------------------------------------
    // STORED TARGET EXISTS
    // --------------------------------------------------------

    readStoredWeight();


    // --------------------------------------------------------
    // 5 SECOND STARTUP PROGRAMMING WINDOW
    // --------------------------------------------------------

    if (checkStartupProgrammingWindow()) {
      // Programming mode selected.

      return;
    }


    // --------------------------------------------------------
    // No button press.
    //
    // Start a NEW automatic dispensing cycle.
    // --------------------------------------------------------

    programmingMode = false;

    manualMode = false;

    automaticCycleComplete = false;

    closeValve();


    Serial.println();
    Serial.println("================================");
    Serial.println("RUN MODE");
    Serial.println("================================");

    Serial.println("Starting automatic dispensing cycle.");
    Serial.println();
  }


  // ==========================================================
  // NO STORED VALUE
  // ==========================================================

  else {
    Serial.println();
    Serial.println("NO STORED TARGET WEIGHT.");

    // No target exists, so start programming mode.

    enterProgrammingMode();
  }
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  if (programmingMode) {
    // --------------------------------------------------------
    // PROGRAMMING MODE
    // --------------------------------------------------------

    programmingModeLoop();
  } else {
    // --------------------------------------------------------
    // NORMAL RUN MODE
    // --------------------------------------------------------

    runModeLoop();
  }
}
