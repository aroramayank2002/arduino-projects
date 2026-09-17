#include <Servo.h>

Servo panServo;
Servo tiltServo;

int panAngle = 90;
int tiltAngle = 90;

const int PAN_PIN = D7;
const int TILT_PIN = D8;

void setup() {
  Serial.begin(9600);

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  panServo.write(panAngle);
  tiltServo.write(tiltAngle);

  Serial.println("D1 Mini Ready");
  sendPosition();
}

void sendPosition() {
  Serial.print("PAN=");
  Serial.print(panAngle);
  Serial.print(", TILT=");
  Serial.println(tiltAngle);
}

void loop() {
  if (Serial.available()) {

    char cmd = Serial.read();

    switch(cmd) {

      case 'q':
        panAngle = max(0, panAngle - 5);
        panServo.write(panAngle);
        break;

      case 'a':
        panAngle = min(180, panAngle + 5);
        panServo.write(panAngle);
        break;

      case 'w':
        tiltAngle = min(180, tiltAngle + 5);
        tiltServo.write(tiltAngle);
        break;

      case 's':
        tiltAngle = max(0, tiltAngle - 5);
        tiltServo.write(tiltAngle);
        break;
    }

    sendPosition();
  }
}