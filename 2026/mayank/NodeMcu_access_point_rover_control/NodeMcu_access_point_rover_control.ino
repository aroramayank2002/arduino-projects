/*
SSID: NodeMCU-Robot
Password: 12345678
Robot URL: http://192.168.4.1

KNOWN ISSUE: motors have trouble running together in this sketch.
One motor runs fine, but as soon as a second one starts (e.g. with
the M1-M4 toggles) both slow down, and multi-motor moves can buzz
without turning. The same motors run together fine in
NodeMCU_rover_all_motors_together. The pins are driven the same
way in both (plain on/off, no PWM), so this looks like the supply
sagging under load rather than the code - not yet confirmed.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* AP_SSID = "NodeMCU-Robot";
const char* AP_PASSWORD = "12345678";

ESP8266WebServer server(80);

// =========================
// START DELAY
// =========================

// Gap between starting one motor and the next, in ms.
// Can also be changed from the web page.
unsigned long startDelay = 300;

const unsigned long MIN_START_DELAY = 0;
const unsigned long MAX_START_DELAY = 3000;
const unsigned long START_DELAY_STEP = 50;

// =========================
// MOTORS
// =========================

enum Direction {
  STOPPED,
  FORWARD,
  REVERSE
};

struct Motor {
  const char* name;
  const char* pinLabel1;
  const char* pinLabel2;
  uint8_t pin1;
  uint8_t pin2;
  Direction state;  // what the motor is doing right now
};

// Front left
Motor M1 = {"M1", "D0", "D1", D0, D1};

// Rear left
Motor M2 = {"M2", "D2", "D3", D2, D3};

// Front right (wired opposite, so pins are swapped)
Motor M3 = {"M3", "D6", "D5", D6, D5};

// Rear right (wired opposite, so pins are swapped)
Motor M4 = {"M4", "D8", "D7", D8, D7};

Motor* motors[] = {&M1, &M2, &M3, &M4};

const int MOTOR_COUNT = 4;

// =========================
// MOTOR CONTROL
// =========================

// Plain on/off drive, no PWM. Motors always run at full power.
void setMotor(Motor &m, Direction direction) {

  m.state = direction;

  if (direction == FORWARD) {
    digitalWrite(m.pin1, HIGH);
    digitalWrite(m.pin2, LOW);
  }
  else if (direction == REVERSE) {
    digitalWrite(m.pin1, LOW);
    digitalWrite(m.pin2, HIGH);
  }
  else {
    digitalWrite(m.pin1, LOW);
    digitalWrite(m.pin2, LOW);
  }
}

// =========================
// LOGGING
// =========================

void logMotor(Motor &m, Direction direction) {

  Serial.print(m.name);
  Serial.print(" | pins ");
  Serial.print(m.pinLabel1);
  Serial.print(",");
  Serial.print(m.pinLabel2);
  Serial.print(" | ");

  if (direction == FORWARD)
    Serial.println("FORWARD");
  else if (direction == REVERSE)
    Serial.println("REVERSE");
  else
    Serial.println("STOP");
}

// Prints e.g. "[12345 ms] M1 | pins D0,D1 | FORWARD"
void logTrigger(Motor &m, Direction direction) {

  Serial.print("[");
  Serial.print(millis());
  Serial.print(" ms] ");

  logMotor(m, direction);
}

void startMotor(Motor &m, Direction direction) {

  if (direction == STOPPED)
    return;

  setMotor(m, direction);
  logTrigger(m, direction);

  delay(startDelay);
}

// =========================
// DRIVE
// =========================

void drive(
  Direction d1,
  Direction d2,
  Direction d3,
  Direction d4
) {

  Serial.println();
  Serial.print("===== MOVEMENT | start delay ");
  Serial.print(startDelay);
  Serial.println(" ms =====");

  // Stop first before changing direction
  setMotor(M1, STOPPED);
  setMotor(M2, STOPPED);
  setMotor(M3, STOPPED);
  setMotor(M4, STOPPED);

  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms] all stopped");

  delay(startDelay);

  // Start motors one at a time so their start-up
  // current spikes never hit the supply together.
  startMotor(M1, d1);
  startMotor(M2, d2);
  startMotor(M3, d3);
  startMotor(M4, d4);

  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms] done");
}

// =========================
// MOVEMENTS
// =========================

// Without PWM there is no "half speed", so gentle turns
// stop the inner side and drive the outer side.

void forward() {
  drive(FORWARD, FORWARD, FORWARD, FORWARD);
}

void forwardLeft() {
  drive(STOPPED, STOPPED, FORWARD, FORWARD);
}

void forwardRight() {
  drive(FORWARD, FORWARD, STOPPED, STOPPED);
}

void reverseRobot() {
  drive(REVERSE, REVERSE, REVERSE, REVERSE);
}

void reverseLeft() {
  drive(STOPPED, STOPPED, REVERSE, REVERSE);
}

void reverseRight() {
  drive(REVERSE, REVERSE, STOPPED, STOPPED);
}

void left() {
  drive(REVERSE, REVERSE, FORWARD, FORWARD);
}

void right() {
  drive(FORWARD, FORWARD, REVERSE, REVERSE);
}

// =========================
// STOP
// =========================

void stopRobot() {

  setMotor(M1, STOPPED);
  setMotor(M2, STOPPED);
  setMotor(M3, STOPPED);
  setMotor(M4, STOPPED);

  Serial.println();
  Serial.println("===== STOP =====");
  Serial.println("All motors stopped.");
}

// =========================
// MOTOR TOGGLES
// =========================

// One character per motor: 0 = stopped, F = forward, R = reverse
void sendMotorStates() {

  String states = "";

  for (int i = 0; i < MOTOR_COUNT; i++) {
    if (motors[i]->state == FORWARD)
      states += "F";
    else if (motors[i]->state == REVERSE)
      states += "R";
    else
      states += "0";
  }

  server.send(200, "text/plain", states);
}

// /toggle?m=1 switches M1 between stopped and forward
void toggleMotor() {

  int index = server.arg("m").toInt() - 1;

  if (index < 0 || index >= MOTOR_COUNT) {
    server.send(400, "text/plain", "bad motor");
    return;
  }

  Motor &m = *motors[index];

  Direction next = (m.state == STOPPED) ? FORWARD : STOPPED;

  setMotor(m, next);

  Serial.print("TOGGLE ");
  logTrigger(m, next);

  sendMotorStates();
}

// =========================
// START DELAY
// =========================

void sendStartDelay() {
  server.send(
    200,
    "text/plain",
    String(startDelay)
  );
}

void increaseStartDelay() {

  startDelay += START_DELAY_STEP;

  if (startDelay > MAX_START_DELAY)
    startDelay = MAX_START_DELAY;

  Serial.print("START DELAY + : ");
  Serial.print(startDelay);
  Serial.println(" ms");

  sendStartDelay();
}

void decreaseStartDelay() {

  // unsigned, so check before subtracting
  if (startDelay < MIN_START_DELAY + START_DELAY_STEP)
    startDelay = MIN_START_DELAY;
  else
    startDelay -= START_DELAY_STEP;

  Serial.print("START DELAY - : ");
  Serial.print(startDelay);
  Serial.println(" ms");

  sendStartDelay();
}

// =========================
// WEB PAGE
// =========================

const char MAIN_PAGE[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
      content="width=device-width, initial-scale=1">

<title>NodeMCU Robot</title>

<style>

body {
  font-family: Arial;
  text-align: center;
  background: #222;
  color: white;
  margin: 0;
  padding: 15px;
}

h1 {
  margin: 10px;
}

.control {
  max-width: 380px;
  margin: auto;
}

.row {
  display: flex;
  justify-content: center;
}

button {
  width: 105px;
  height: 75px;
  margin: 5px;
  border: none;
  border-radius: 15px;
  font-size: 16px;
  font-weight: bold;
  background: #444;
  color: white;
}

button:active {
  background: #777;
  transform: scale(.95);
}

.stop {
  background: #b00000;
}

.toggle {
  width: 80px;
  height: 60px;
}

.toggle.on {
  background: #1a8a1a;
}

.delay {
  width: 160px;
  height: 55px;
  background: #0066aa;
}

#delay {
  font-size: 22px;
  font-weight: bold;
  margin: 15px;
}

</style>

<script>

function command(cmd) {
  fetch("/" + cmd)
  .then(refreshMotors);
}

function toggleMotor(n) {
  fetch("/toggle?m=" + n)
  .then(response => response.text())
  .then(showMotors);
}

function refreshMotors() {
  fetch("/motors")
  .then(response => response.text())
  .then(showMotors);
}

// states is e.g. "FF00": one character per motor
function showMotors(states) {
  for (let i = 0; i < states.length; i++) {
    const button = document.getElementById("m" + (i + 1));
    const on = states[i] !== "0";

    button.classList.toggle("on", on);
    button.innerHTML = "M" + (i + 1) + "<br>" +
      (states[i] === "F" ? "FWD" :
       states[i] === "R" ? "REV" : "OFF");
  }
}

function changeDelay(cmd) {
  fetch("/" + cmd)
  .then(response => response.text())
  .then(showDelay);
}

function showDelay(ms) {
  document.getElementById("delay")
    .innerText = "Start delay: " + ms + " ms";
}

// Show the current value when the page opens
window.onload = () => {
  changeDelay("delay");
  refreshMotors();
};

</script>

</head>

<body>

<h1>🤖 Robot</h1>

<div id="delay">
Start delay: ...
</div>

<div class="control">

<div class="row">

<button class="delay"
        onclick="changeDelay('delay-down')">
− DELAY
</button>

<button class="delay"
        onclick="changeDelay('delay-up')">
+ DELAY
</button>

</div>

<div class="row">

<button id="m1" class="toggle" onclick="toggleMotor(1)">M1<br>OFF</button>
<button id="m2" class="toggle" onclick="toggleMotor(2)">M2<br>OFF</button>
<button id="m3" class="toggle" onclick="toggleMotor(3)">M3<br>OFF</button>
<button id="m4" class="toggle" onclick="toggleMotor(4)">M4<br>OFF</button>

</div>

<div class="row">

<button onclick="command('forward-left')">
↖<br>
Forward<br>
Left
</button>

<button onclick="command('forward')">
↑<br>
Forward
</button>

<button onclick="command('forward-right')">
↗<br>
Forward<br>
Right
</button>

</div>

<div class="row">

<button onclick="command('left')">
←<br>
Left
</button>

<button class="stop"
        onclick="command('stop')">
STOP
</button>

<button onclick="command('right')">
→<br>
Right
</button>

</div>

<div class="row">

<button onclick="command('reverse-left')">
↙<br>
Reverse<br>
Left
</button>

<button onclick="command('reverse')">
↓<br>
Reverse
</button>

<button onclick="command('reverse-right')">
↘<br>
Reverse<br>
Right
</button>

</div>

</div>

</body>

</html>

)rawliteral";

// =========================
// HTTP HANDLERS
// =========================

void handleRoot() {
  server.send_P(
    200,
    "text/html; charset=utf-8",
    MAIN_PAGE
  );
}

void handleForward() {
  forward();
  server.send(200, "text/plain", "FORWARD");
}

void handleForwardLeft() {
  forwardLeft();
  server.send(200, "text/plain", "FORWARD LEFT");
}

void handleForwardRight() {
  forwardRight();
  server.send(200, "text/plain", "FORWARD RIGHT");
}

void handleLeft() {
  left();
  server.send(200, "text/plain", "LEFT");
}

void handleRight() {
  right();
  server.send(200, "text/plain", "RIGHT");
}

void handleReverse() {
  reverseRobot();
  server.send(200, "text/plain", "REVERSE");
}

void handleReverseLeft() {
  reverseLeft();
  server.send(200, "text/plain", "REVERSE LEFT");
}

void handleReverseRight() {
  reverseRight();
  server.send(200, "text/plain", "REVERSE RIGHT");
}

void handleStop() {
  stopRobot();
  server.send(200, "text/plain", "STOP");
}

// =========================
// SETUP
// =========================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("       NODEMCU ROBOT");
  Serial.println("================================");

  Serial.println("M1 = D0, D1");
  Serial.println("M2 = D2, D3");
  Serial.println("M3 = D5, D6");
  Serial.println("M4 = D7, D8");

  // Motor pins

  pinMode(M1.pin1, OUTPUT);
  pinMode(M1.pin2, OUTPUT);

  pinMode(M2.pin1, OUTPUT);
  pinMode(M2.pin2, OUTPUT);

  pinMode(M3.pin1, OUTPUT);
  pinMode(M3.pin2, OUTPUT);

  pinMode(M4.pin1, OUTPUT);
  pinMode(M4.pin2, OUTPUT);

  stopRobot();

  // =========================
  // WIFI ACCESS POINT
  // =========================

  WiFi.mode(WIFI_AP);

  WiFi.softAP(
    AP_SSID,
    AP_PASSWORD
  );

  IPAddress ip = WiFi.softAPIP();

  Serial.println();
  Serial.println("ACCESS POINT STARTED");

  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);

  Serial.print("URL: http://");
  Serial.println(ip);

  // =========================
  // WEB SERVER
  // =========================

  server.on("/", handleRoot);

  server.on("/forward", handleForward);
  server.on("/forward-left", handleForwardLeft);
  server.on("/forward-right", handleForwardRight);

  server.on("/left", handleLeft);
  server.on("/right", handleRight);

  server.on("/reverse", handleReverse);
  server.on("/reverse-left", handleReverseLeft);
  server.on("/reverse-right", handleReverseRight);

  server.on("/stop", handleStop);

  server.on("/motors", sendMotorStates);
  server.on("/toggle", toggleMotor);

  server.on("/delay", sendStartDelay);
  server.on("/delay-up", increaseStartDelay);
  server.on("/delay-down", decreaseStartDelay);

  server.begin();

  Serial.println("Web server started.");

  Serial.println("================================");
}

// =========================
// LOOP
// =========================

void loop() {

  server.handleClient();
}

