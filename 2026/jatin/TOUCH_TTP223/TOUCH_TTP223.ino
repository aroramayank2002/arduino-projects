int x = 0;
void setup() {
  Serial.begin(9600);
  pinMode(22, OUTPUT);

}

void loop() {
  if(analogRead(A1)>100 && x==0){
  digitalWrite(22, HIGH);
  delay(1000);
  x=1;
  }
  if(analogRead(A1)>100 && x==1){
    digitalWrite(22, LOW);
    delay(1000);
    x=0;
  }

}
