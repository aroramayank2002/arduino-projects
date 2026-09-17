#include <Servo.h>
Servo myServo;
// ultrasonic sensor's pin
const int trigPin = 9;
const int echoPin = 8;
int y = 0;
/*const int greenLED = 2;
const int blueLED  = 3;
const int redLED   = 4;

const int buzzer   = 5;*/

void setup() {
  myServo.attach(10);  // servo motor connected with pin no - 10
  myServo.write(0);    // Lock position
  delay(1000);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  /*pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzer, OUTPUT);*/

  Serial.begin(9600);
}

void loop() {
  long duration;
  float distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  //digitalWrite(greenLED, LOW);
  //digitalWrite(blueLED, LOW);
  //digitalWrite(redLED, LOW);
  //noTone(buzzer);

  /*if ((distance < 40) && (distance > 25)) {
    digitalWrite(greenLED, HIGH);
    delay(1000);
  }
   if ((distance < 25) && (distance > 10)) {
    digitalWrite(blueLED, HIGH);
    digitalWrite(greenLED, LOW);
    delay(200);
  }*/
  if (distance < 10 && y == 0) {
    /*digitalWrite(redLED, HIGH);
    digitalWrite(blueLED, LOW);
    delay(100);
    tone(buzzer, 1000);
    delay(200);
    noTone(buzzer);
    delay(200);*/
    for (int x = 0; x <= 90; x++) {
      myServo.write(x);
      delay(5);
      y = 1;
    }
    delay(2000);
  }

  if (distance > 15 && y == 1) {
    delay(3000);
    for (int x = 90; x >= 0; x--) {
      myServo.write(x);
      delay(5);
      y = 0;
    }
    delay(2000);
  }

  delay(100);
}
