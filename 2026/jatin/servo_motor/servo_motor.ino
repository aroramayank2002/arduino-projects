#include <Servo.h>
Servo myServo;

void setup() {
 myServo.attach(10);  // servo motor connected with pin no - 10
  myServo.write(0);    // Lock position
  delay(2000);
  myServo.write(90);
  delay(2000);

}

void loop() {
  
// smooth running
  for(int x = 0; x<=90; x++){
    myServo.write(x);
    delay(5);
  }
  delay(1000);
  for(int x = 90; x>=0; x--){
    myServo.write(x);
    delay(5);
  }
  delay(1000);

}
