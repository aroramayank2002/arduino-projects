/*
  Add motors one at a time, forward then reverse
  Board: NodeMCU 1.0 (ESP-12E Module)

  M1: D0, D1
  M2: D2, D3
  M3: D5, D6  (wired opposite, D6 HIGH = forward)
  M4: D7, D8  (wired opposite, D8 HIGH = forward)
*/

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize all motor pins as outputs.
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  // forward: add one motor every 2 seconds
  digitalWrite(D0, HIGH);     // M1 forward
  digitalWrite(D1, LOW);
  delay(2000);                // wait 2 seconds

  digitalWrite(D2, HIGH);     // M2 forward
  digitalWrite(D3, LOW);
  delay(2000);                // wait 2 seconds

  digitalWrite(D5, LOW);      // M3 forward (wired opposite)
  digitalWrite(D6, HIGH);
  delay(2000);                // wait 2 seconds

  digitalWrite(D7, LOW);      // M4 forward (wired opposite)
  digitalWrite(D8, HIGH);
  delay(2000);                // wait 2 seconds

  // stop: all pins LOW
  digitalWrite(D0, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D6, LOW);
  digitalWrite(D8, LOW);
  delay(2000);                // wait 2 seconds

  // reverse: add one motor every 2 seconds
  digitalWrite(D0, LOW);      // M1 reverse
  digitalWrite(D1, HIGH);
  delay(2000);                // wait 2 seconds

  digitalWrite(D2, LOW);      // M2 reverse
  digitalWrite(D3, HIGH);
  delay(2000);                // wait 2 seconds

  digitalWrite(D5, HIGH);     // M3 reverse (wired opposite)
  digitalWrite(D6, LOW);
  delay(2000);                // wait 2 seconds

  digitalWrite(D7, HIGH);     // M4 reverse (wired opposite)
  digitalWrite(D8, LOW);
  delay(2000);                // wait 2 seconds

  // stop: all pins LOW
  digitalWrite(D1, LOW);
  digitalWrite(D3, LOW);
  digitalWrite(D5, LOW);
  digitalWrite(D7, LOW);
  delay(2000);                // wait 2 seconds
}
