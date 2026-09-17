#define LED_PIN LED_BUILTIN

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, LOW);   // LED ON
  delay(2000);

  digitalWrite(LED_PIN, HIGH);  // LED OFF
  delay(1000);
}