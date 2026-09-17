// Works
#include <ESP32Servo.h>

Servo panServo;
Servo tiltServo;

const int PAN_PIN = 18;
const int TILT_PIN = 19;

int panAngle = 90;
int tiltAngle = 90;

const int STEP = 5;

void sendPosition() {
  Serial.print("PAN=");
  Serial.print(panAngle);
  Serial.print(", TILT=");
  Serial.println(tiltAngle);
}

void setup() {
  Serial.begin(9600);

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);

  panServo.write(panAngle);
  tiltServo.write(tiltAngle);

  Serial.println("ESP32 Ready");
  sendPosition();
}

void loop() {

  if (Serial.available() > 0) {

    char cmd = Serial.read();

    Serial.print("Received: ");
    Serial.println(cmd);

    switch (cmd) {

      case 'q':
        panAngle -= STEP;
        if (panAngle < 0) panAngle = 0;
        panServo.write(panAngle);
        break;

      case 'a':
        panAngle += STEP;
        if (panAngle > 180) panAngle = 180;
        panServo.write(panAngle);
        break;

      case 'w':
        tiltAngle += STEP;
        if (tiltAngle > 180) tiltAngle = 180;
        tiltServo.write(tiltAngle);
        break;

      case 's':
        tiltAngle -= STEP;
        if (tiltAngle < 0) tiltAngle = 0;
        tiltServo.write(tiltAngle);
        break;

      case 'p':
        // Only query position
        break;

      default:
        return;
    }

    sendPosition();
  }
}