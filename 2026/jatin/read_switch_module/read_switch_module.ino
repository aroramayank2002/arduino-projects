void setup() {
  pinMode(12, INPUT);
  pinMode(2, OUTPUT);
}

void loop() {
  if(digitalRead(12)<1){
    digitalWrite(2, HIGH);
  }
  else{
    digitalWrite(2, LOW);
  }

}
