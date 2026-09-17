#define ENC_PSH D1
#define ENC_A   D8
#define ENC_B   D7

void setup() {
  Serial.begin(9600);
delay(2000);  // Wait 2 seconds for Serial Monitor


  pinMode(ENC_PSH, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  Serial.println("Rotary encoder test started");
}

void loop() {
  // Push button
  static int lastButton = HIGH;
  int button = digitalRead(ENC_PSH);

  if (button != lastButton) {
    if (button == LOW) {
      Serial.println("BUTTON: PRESSED");
    } else {
      Serial.println("BUTTON: RELEASED");
    }
    lastButton = button;
  }

  // Rotary encoder
  static int lastA = HIGH;
  int a = digitalRead(ENC_A);
  int b = digitalRead(ENC_B);

  if (a != lastA) {
    if (a == LOW) {
      if (b == HIGH) {
        Serial.println("ROTATE: CLOCKWISE");
      } else {
        Serial.println("ROTATE: COUNTER-CLOCKWISE");
      }
    }
    lastA = a;
  }

  delay(2);
}