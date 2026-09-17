void setup() {
  pinMode(12, INPUT);
  pinMode(13, OUTPUT);
}

void loop() {
  if(digitalRead(12)<1){
    digitalWrite(13, HIGH);
  }
  else{
    digitalWrite(13, LOW);
  }

}
