int x = 0;
void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

}

void loop() {
  if(analogRead(A0)>100 && x==0){
  digitalWrite(13, HIGH);
  delay(1000);
  x=1;
  }
  if(analogRead(A0)>100 && x==1){
    digitalWrite(13, LOW);
    delay(1000);
    x=0;
  }

}
