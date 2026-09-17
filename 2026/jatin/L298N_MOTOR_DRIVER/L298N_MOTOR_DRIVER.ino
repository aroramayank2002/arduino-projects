void setup() {
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(4, HIGH);
  digitalWrite(7, LOW);
}

void loop() {
  /* digitalWrite(4, LOW);
  digitalWrite(7, HIGH);
  analogWrite(3, 255);
  delay(5000);

  digitalWrite(4, HIGH);
  digitalWrite(7, LOW);
  analogWrite(3, 150);
  delay(5000);

  digitalWrite(4, HIGH);
  digitalWrite(7, LOW);
  analogWrite(3, 100);
  delay(5000);

  digitalWrite(4, HIGH);
  digitalWrite(7, LOW);
  analogWrite(3, 70);
  delay(5000);*/

  // Motor speed slow to high and high to slow (as a dimmer)
  /*for(int x = 70; x<=255; x++){  
  analogWrite(3, x);
  delay(50);
 }
 for(int y = 255; y>=70; y--){  
  analogWrite(3, y);
  delay(50);
 }*/

  // motor speed controll with potentiometer
  int x = map(analogRead(A0), 0, 1023, 70, 255);
  analogWrite(3, x);
  delay(10);
}
