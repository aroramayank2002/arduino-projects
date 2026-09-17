void setup() {
  Serial.begin(9600);
  pinMode(13, INPUT_PULLUP);
}

void loop() {
  float MQ = analogRead(A0);
  Serial.println(MQ);
  delay(2000);
  /*if(digitalRead(13)<1){
    Serial.println("MQ Activated!");
    delay(1000);
}*/

}
