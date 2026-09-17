#define BUTTON_PIN D6
#define LED_PIN LED_BUILTIN

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Button pressed
    digitalWrite(LED_PIN, LOW);   // Built-in LED is active LOW
  } else {
    // Button released
    digitalWrite(LED_PIN, HIGH);
  }
}