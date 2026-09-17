/*
**Servo Position Control Using a Push Button and Timeout**

This Arduino Nano program controls a servo motor connected to digital pin 3 and uses a push button connected to 
digital pin 2 (configured with the internal pull-up resistor). The onboard LED (LED_BUILTIN) is used as a visual indicator for button activation.

After power-up, the program waits for 2 seconds and then moves the servo to an initial position of 66°. 
It then enters a waiting state where it monitors the push button for up to 60 seconds.

If the push button is pressed during this period, the onboard LED blinks once to acknowledge the button press, 
and the servo immediately moves to 12°. If no button press is detected within 60 seconds, 
the servo automatically returns to 12° without user intervention.

Once the servo reaches 12°, either due to a button press or the timeout, the program completes its operation and remains idle. 
This implementation provides both manual and automatic return mechanisms while offering visual confirmation of user input through the onboard LED.

*/
#include <Servo.h>

Servo doorServo;

// -------- PIN DEFINITIONS --------
const byte SERVO_PIN = 3;
const byte BUTTON_PIN = 2;
const byte LED_PIN = LED_BUILTIN;

// -------- SERVO POSITIONS --------
const int START_ANGLE = 66;
const int END_ANGLE = 12;

// -------- TIMING --------
const unsigned long START_DELAY = 5000;      // 2 seconds
const unsigned long BUTTON_TIMEOUT = 20000;  // 20 seconds

bool finished = false;
unsigned long waitStartTime = 0;

void blinkLED()
{
  digitalWrite(LED_PIN, HIGH);
  delay(150);
  digitalWrite(LED_PIN, LOW);
}

void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  doorServo.attach(SERVO_PIN);

  // Start at 12 degrees
  doorServo.write(END_ANGLE);

  // Wait after power-up
  delay(START_DELAY);

  // Move to 66 degrees
  doorServo.write(START_ANGLE);

  // Start timeout timer
  waitStartTime = millis();
}

void loop()
{
  if (finished)
    return;

  // Button pressed (active LOW)
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    blinkLED();
    doorServo.write(END_ANGLE);
    finished = true;
    return;
  }

  // Timeout after 60 seconds
  if (millis() - waitStartTime >= BUTTON_TIMEOUT)
  {
    doorServo.write(END_ANGLE);
    finished = true;
  }
}