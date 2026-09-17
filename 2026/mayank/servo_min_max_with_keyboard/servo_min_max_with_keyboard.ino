/*
**Servo Control Using Serial Communication**

This Arduino Nano program controls a servo motor connected to digital pin 3 through serial communication. 
At startup, the servo is initialized to 0°. 
The microcontroller continuously monitors the serial port for incoming commands at a baud rate of 115200.

Two commands are supported:

* **'s'**: Increases the servo position by 2°, up to a maximum angle of 66°.
* **'a'**: Decreases the servo position by 2°, down to a minimum angle of 12°.

After every valid command, the servo is moved to the updated position and the Nano 
transmits the current target angle back over the serial interface. 
This provides real-time feedback to the connected computer or external controller 
while ensuring that the servo motion remains within its allowable operating range.

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
const unsigned long START_DELAY = 2000;      // 2 seconds
const unsigned long BUTTON_TIMEOUT = 60000;  // 60 seconds

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