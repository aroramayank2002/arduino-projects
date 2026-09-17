#include <Servo.h>

Servo doorServo;

// -------- PIN DEFINITIONS --------
const byte RED_PIN = 4;
const byte YELLOW_PIN = 18;   // A4
const byte GREEN_PIN = 19;    // A5
const byte BUTTON_PIN = 2;
const byte SERVO_PIN = 3;

// -------- SERVO POSITIONS --------
const int SERVO_CLOSED = 0;
const int SERVO_OPEN = 90;

// -------- TIMING --------
const unsigned long RED_TIME = 3000;
const unsigned long YELLOW_TIME = 3000;
const unsigned long GREEN_TIME = 10000;

const unsigned long SERVO_CLOSE_TIME = 4000;

// -------- STATES --------
enum State {
  IDLE,
  RED1,
  YELLOW1,
  GREEN,
  YELLOW2,
  RED2
};

State state = IDLE;

unsigned long stateStartTime = 0;

// -------- SERVO CONTROL --------
bool servoClosing = false;
unsigned long servoCloseStart = 0;
float servoPosition = 0;

// -------- LED CONTROL --------
void setLights(bool r, bool y, bool g)
{
  digitalWrite(RED_PIN, r);
  digitalWrite(YELLOW_PIN, y);
  digitalWrite(GREEN_PIN, g);
}

// -------- CHANGE STATE --------
void changeState(State newState)
{
  state = newState;
  stateStartTime = millis();

  switch (state)
  {
    case RED1:
      setLights(true, false, false);
      break;

    case YELLOW1:
      setLights(false, true, false);
      break;

    case GREEN:
      setLights(false, false, true);

      doorServo.write(SERVO_OPEN);
      servoPosition = SERVO_OPEN;
      break;

    case YELLOW2:
      setLights(false, true, false);
      break;

    case RED2:
      setLights(true, false, false);

      servoClosing = true;
      servoCloseStart = millis();
      break;

    case IDLE:
      setLights(false, false, false);
      break;
  }
}

// -------- SMOOTH SERVO CLOSE --------
void updateServo()
{
  if (!servoClosing) return;

  unsigned long elapsed = millis() - servoCloseStart;

  if (elapsed >= SERVO_CLOSE_TIME)
  {
    doorServo.write(SERVO_CLOSED);
    servoClosing = false;
    return;
  }

  float t = (float)elapsed / SERVO_CLOSE_TIME;

  // Ease-out motion
  float ease = 1 - (1 - t) * (1 - t);

  float pos = SERVO_OPEN - (SERVO_OPEN * ease);

  doorServo.write((int)pos);
}

// -------- SETUP --------
void setup()
{
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  doorServo.attach(SERVO_PIN);
  doorServo.write(SERVO_CLOSED);

  setLights(false, false, false);
}

// -------- LOOP --------
void loop()
{
  updateServo();

  unsigned long now = millis();

  // Start sequence
  if (state == IDLE && digitalRead(BUTTON_PIN) == LOW)
  {
    changeState(RED1);
  }

  switch (state)
  {
    case RED1:
      if (now - stateStartTime >= RED_TIME)
        changeState(YELLOW1);
      break;

    case YELLOW1:
      if (now - stateStartTime >= YELLOW_TIME)
        changeState(GREEN);
      break;

    case GREEN:
      if (now - stateStartTime >= GREEN_TIME)
        changeState(YELLOW2);
      break;

    case YELLOW2:
      if (now - stateStartTime >= YELLOW_TIME)
        changeState(RED2);
      break;

    case RED2:
      if (now - stateStartTime >= RED_TIME)
        changeState(IDLE);
      break;

    case IDLE:
      break;
  }
}