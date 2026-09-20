#define alarm 13

void setup() {
  Serial.begin(9600);
  pinMode(alarm, OUTPUT);
}

void loop() {
  int moisture = analogRead(A0);
  if (moisture > 100) {
    digitalWrite(alarm, HIGH);
    Serial.println("Raining !");
  } else {
    digitalWrite(alarm, LOW);
  }
  delay(2000);  // test in every 5 seconds
}
