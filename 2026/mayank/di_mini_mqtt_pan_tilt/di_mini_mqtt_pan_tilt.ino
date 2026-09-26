/*
 * D1 Mini Pan/Tilt bridge - WiFi + MQTT, with serial control retained
 * ============================================================================
 * Combines the two prior sketches in this folder:
 *   - 1.ino's serial pan/tilt servo control - kept working completely
 *     unchanged: 'q'/'a' step pan, 'w'/'s' step tilt, +/-5 degrees each,
 *     clamped to 0-180, printed back over Serial as "PAN=x, TILT=y".
 *   - 2.ino's WiFi + MQTT pattern (ESP8266WiFi + PubSubClient), pointed at
 *     this project's own broker instead of a one-off IoT cloud.
 *
 * New behavior: subscribes to the MQTT state topic of the "Living-Room-
 * Camera" device (a SensorType.JOYSTICK in the home-assistant app, see
 * /sensors/joystick) - that device publishes the literal string
 * LEFT/RIGHT/UP/DOWN/STOP, once immediately and then at a configurable
 * interval (joystick.tick-interval-ms, 100ms by default) for as long as
 * its own joystick control page is held. Each message steps this sketch's
 * servos by STEP degrees (2 by default here - independent of 1.ino's own
 * step, and of the software Pan/Tilt device's, since this is real hardware
 * tuned to its own servos):
 *   LEFT  -> pan  +STEP
 *   RIGHT -> pan  -STEP
 *   UP    -> tilt -STEP
 *   DOWN  -> tilt +STEP
 *   STOP  -> ignored (nothing moved, so nothing to publish)
 * tiltAngle itself keeps this meaning (higher = "up") everywhere it's
 * reported (Serial, MQTT publish) - the tilt servo happens to be mounted so
 * a higher raw angle physically points it down, so only its actual
 * Servo::write() call (in stepTilt()/setup()) mirrors the value; nothing
 * else needed to change for this.
 *
 * After EVERY move - whether triggered by Serial or by MQTT - this
 * publishes its own new position, in the same JSON shape the app's own
 * SensorType.PAN_TILT device publishes (see PanTiltStateService.java /
 * GET /sensors/pan_tilt/control):
 *   {"name":"D1 Mini Pan/Tilt","pan":90,"tilt":90,
 *    "panMin":0,"panMax":180,"tiltMin":0,"tiltMax":180}
 * (min/max here are 0-180 - this hardware's own real servo range from
 * 1.ino, not the software device's simulated -90..90/-45..45 range).
 * Published to MQTT_TOPIC_STATE below as a plain state message, not
 * through Home Assistant's own MQTT discovery topics - this won't show up
 * as its own HA entity unless you also add discovery (see
 * docs/md/robot-arm-hardware.md for that pattern on another device).
 *
 * ===== Fill in before flashing =====
 *   - WIFI_SSID / WIFI_PASSWORD below are hardcoded to the same network
 *     2.ino uses - update if this D1 Mini is on a different one.
 *   - MQTT_TOPIC_JOYSTICK_STATE if "Living-Room-Camera"'s sensor id ever
 *     changes (it's sensor id 28 today - homeassistant/sensor/joystick_28/
 *     state). Confirm with:
 *       sqlite3 data/homeassistant.db \
 *         "SELECT id, entity_id FROM sensors WHERE name='Living-Room-Camera';"
 *     Same gotcha as everywhere else in this project: use the app's own
 *     entity_id column, not Home Assistant's slugified one.
 *
 * Board: D1 Mini (ESP8266). Libraries: ESP8266WiFi, PubSubClient, Servo.
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <EEPROM.h>

/* ===== WIFI SETTINGS (same network as 2.ino) ===== */
const char* WIFI_SSID     = "OWNIT-7B92";
const char* WIFI_PASSWORD = "NS3T7V55NKBKKR";

/* ===== MQTT SETTINGS ===== */
const char* MQTT_HOST      = "advait.se";
const int   MQTT_PORT      = 4023;
const char* MQTT_CLIENT_ID = "d1-mini-pan-tilt";

// "Living-Room-Camera" (the Joystick device)'s own state topic - see the
// header comment above for how to confirm/update this.
const char* MQTT_TOPIC_JOYSTICK_STATE = "homeassistant/sensor/joystick_28/state";

// Where this sketch publishes its own position after every move.
const char* MQTT_TOPIC_STATE = "d1mini/pantilt/state";

const char* DEVICE_NAME = "D1 Mini Pan/Tilt";

/* ===== SERVO WIRING (same pins/range/step as 1.ino) ===== */
const int PAN_PIN   = D7;
const int TILT_PIN  = D5;
const int STEP      = 2;
const int ANGLE_MIN = 0;
const int ANGLE_MAX = 180;

// Tilt-specific upper bound - DOWN drives tiltAngle toward ANGLE_MAX, but this hardware's
// tilt mount hits its bottom mechanical stop well before 180, so tilt is clamped to this
// instead of ANGLE_MAX to avoid straining the servo against the stop.
const int TILT_MAX  = 175;

Servo panServo;
Servo tiltServo;

int panAngle  = 90;
int tiltAngle = 90;

/* ===== Last-stable-position persistence =====
 * Booting always used to write the hardcoded 90/90 above straight to the servos - if the
 * horn had physically settled somewhere else (wherever it was left powered off), that's a
 * sudden jump/jerk on every power-up. Instead, whenever a position holds steady for
 * POSITION_SAVE_DELAY_MS, it's saved to EEPROM (flash-emulated on ESP8266), and setup()
 * loads that instead of hardcoding 90/90 - so a fresh boot writes the servos back to
 * wherever they already physically were, and only actual joystick/Serial input moves them
 * from there. */
const int POSITION_SAVE_DELAY_MS = 2500;
const int EEPROM_SIZE   = 3;
const byte EEPROM_MAGIC = 0x37; // arbitrary marker - distinguishes "saved a real position" from unwritten/erased flash

unsigned long lastMoveMillis = 0;
bool positionDirty = false;

void loadPosition() {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(0) == EEPROM_MAGIC) {
    panAngle  = constrain((int) EEPROM.read(1), ANGLE_MIN, ANGLE_MAX);
    tiltAngle = constrain((int) EEPROM.read(2), ANGLE_MIN, TILT_MAX);
    Serial.print("[EEPROM] Loaded last position PAN=");
    Serial.print(panAngle);
    Serial.print(", TILT=");
    Serial.println(tiltAngle);
  } else {
    Serial.println("[EEPROM] No saved position yet - using default PAN=90, TILT=90");
    positionDirty = true; // so the default gets saved once it's held stable
    lastMoveMillis = millis();
  }
}

void savePosition() {
  EEPROM.write(0, EEPROM_MAGIC);
  EEPROM.write(1, panAngle);
  EEPROM.write(2, tiltAngle);
  EEPROM.commit();
  Serial.print("[EEPROM] Saved stable position PAN=");
  Serial.print(panAngle);
  Serial.print(", TILT=");
  Serial.println(tiltAngle);
}

// Called once per loop() iteration - see the header comment above.
void maybeSavePosition() {
  if (positionDirty && millis() - lastMoveMillis >= POSITION_SAVE_DELAY_MS) {
    savePosition();
    positionDirty = false;
  }
}

WiFiClient espClient;
PubSubClient mqtt(espClient);

/* ===== Serial readback - unchanged from 1.ino ===== */
void sendPosition() {
  Serial.print("PAN=");
  Serial.print(panAngle);
  Serial.print(", TILT=");
  Serial.println(tiltAngle);
}

/* ===== Publishes the current position in the same JSON shape
 * PanTiltStateService.java uses. No-op if MQTT isn't connected yet, so
 * Serial control still works standalone before/without a network. ===== */
void publishState() {
  if (!mqtt.connected()) {
    Serial.println("[MQTT->] Skipped publish - not connected");
    return;
  }
  char payload[192];
  snprintf(payload, sizeof(payload),
           "{\"name\":\"%s\",\"pan\":%d,\"tilt\":%d,"
           "\"panMin\":%d,\"panMax\":%d,\"tiltMin\":%d,\"tiltMax\":%d}",
           DEVICE_NAME, panAngle, tiltAngle, ANGLE_MIN, ANGLE_MAX, ANGLE_MIN, TILT_MAX);

  Serial.print("[MQTT->] Publishing to ");
  Serial.print(MQTT_TOPIC_STATE);
  Serial.print(": ");
  Serial.println(payload);

  bool ok = mqtt.publish(MQTT_TOPIC_STATE, payload);
  Serial.println(ok ? "[MQTT->] Publish OK" : "[MQTT->] Publish FAILED");
}

/* ===== Smooth-motion tuning =====
 * A direct Servo::write(target) jumps the servo horn to the new angle in one instant PWM
 * change - even a small STEP-sized jump snaps rather than glides, which reads as "jerky". This
 * ramps every move through the 1-degree-per-step pattern 2.ino's own moveServosSmooth() already
 * uses in this project, just applied per-axis instead of to both servos in lockstep. It's
 * blocking (delay(), same as 2.ino), but deliberately brief: at STEP=2 degrees this is at most
 * STEP * SMOOTH_STEP_DELAY_MS = 16ms per move, well inside the 100ms joystick tick interval, so
 * it doesn't meaningfully delay mqtt.loop()/Serial polling or reintroduce the buffering problem
 * processPendingDirection() below was written to fix. */
const int SMOOTH_STEP_DELAY_MS = 8;

void moveServoSmooth(Servo &servo, int fromAngle, int toAngle) {
  int step = (toAngle > fromAngle) ? 1 : -1;
  for (int a = fromAngle; a != toAngle; a += step) {
    servo.write(a);
    delay(SMOOTH_STEP_DELAY_MS);
  }
  servo.write(toAngle);
}

/* ===== Shared step helpers - used by both Serial and MQTT input, so a
 * physical joystick press and a serial keypress move the servos by exactly
 * the same amount and both report back the same way. ===== */
void stepPan(int delta) {
  Serial.print("[Servo] Pan step ");
  Serial.println(delta);
  int fromAngle = panServo.read();
  panAngle = constrain(panAngle + delta, ANGLE_MIN, ANGLE_MAX);
  moveServoSmooth(panServo, fromAngle, panAngle);
  lastMoveMillis = millis();
  positionDirty = true;
  sendPosition();
  publishState();
}

void stepTilt(int delta) {
  Serial.print("[Servo] Tilt step ");
  Serial.println(delta);
  int fromAngle = tiltServo.read();
  // Clamped to TILT_MAX, not ANGLE_MAX - DOWN drives tiltAngle up, and this mount hits its
  // bottom mechanical stop well before 180 (see TILT_MAX's own comment above).
  tiltAngle = constrain(tiltAngle + delta, ANGLE_MIN, TILT_MAX);
  // This servo is mounted so a higher raw angle physically points it DOWN - mirrored here
  // so the reported/published tiltAngle keeps the same meaning for both Serial and MQTT
  // input. UP/DOWN's step direction (above) is swapped to match this hardware's physical
  // mount, so it's this mirror + that swap together that make UP mean "physically up".
  moveServoSmooth(tiltServo, fromAngle, ANGLE_MAX - tiltAngle);
  lastMoveMillis = millis();
  positionDirty = true;
  sendPosition();
  publishState();
}

/* ===== MQTT: LEFT/RIGHT/UP/DOWN/STOP arriving from the Joystick device =====
 * Deliberately does almost no work here - just records the latest direction and
 * returns immediately. PubSubClient's loop() dispatches one queued message per call, so if
 * this callback did the full Serial-logging + servo-move + publish-back work synchronously
 * (like it used to), any messages that arrived while it was busy sat buffered and got acted
 * on one-by-one afterwards - visibly moving the servo for a while after you'd already
 * released the joystick and a STOP had long since been sent. Separating "received" from
 * "acted on" (see the pending* variables + loop() below) means only the newest direction
 * ever gets acted on - anything older is silently superseded, so there's nothing left to
 * catch up on. */
volatile bool hasPendingDirection = false;
String pendingDirection;

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String direction;
  for (unsigned int i = 0; i < length; i++) {
    direction += (char)payload[i];
  }
  pendingDirection = direction;
  hasPendingDirection = true;
}

/* ===== Acts on the latest direction received, once per loop() iteration - see
 * onMqttMessage() above for why this is split out instead of acting inline. ===== */
void processPendingDirection() {
  if (!hasPendingDirection) {
    return;
  }
  hasPendingDirection = false;
  String direction = pendingDirection;

  Serial.print("[MQTT<-] ");
  Serial.println(direction);

  if (direction == "LEFT") {
    stepPan(STEP);
  } else if (direction == "RIGHT") {
    stepPan(-STEP);
  } else if (direction == "UP") {
    stepTilt(-STEP);
  } else if (direction == "DOWN") {
    stepTilt(STEP);
  } else if (direction == "STOP") {
    Serial.println("[MQTT<-] STOP - nothing to move");
  } else {
    Serial.println("[MQTT<-] Unrecognized direction - ignored");
  }
}

void connectWiFi() {
  Serial.print("[WiFi] Connecting to SSID \"");
  Serial.print(WIFI_SSID);
  Serial.println("\"");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    attempt++;
    Serial.print("[WiFi] Still connecting... (attempt ");
    Serial.print(attempt);
    Serial.print(", status=");
    Serial.print(WiFi.status());
    Serial.println(")");
  }

  Serial.print("[WiFi] Connected. IP=");
  Serial.print(WiFi.localIP());
  Serial.print(" RSSI=");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.print("[MQTT] Connecting to ");
    Serial.print(MQTT_HOST);
    Serial.print(":");
    Serial.print(MQTT_PORT);
    Serial.print(" as \"");
    Serial.print(MQTT_CLIENT_ID);
    Serial.print("\" ... ");

    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      mqtt.subscribe(MQTT_TOPIC_JOYSTICK_STATE);
      Serial.print("[MQTT] Subscribed to ");
      Serial.println(MQTT_TOPIC_JOYSTICK_STATE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" - retrying in 3s");
      delay(3000);
    }
  }
}

void setup() {
  // 115200, not 1.ino's 9600 - the extra logging this sketch adds takes noticeably longer to
  // print at 9600 baud, which was part of what let incoming MQTT messages queue up faster
  // than they were being drained (see onMqttMessage()'s comment). Update your Serial Monitor's
  // baud rate to match if you're only used to 1.ino's 9600.
  Serial.begin(115200);
  delay(200); // let the serial monitor catch up before the first prints

  Serial.println("[Boot] D1 Mini Pan/Tilt starting up");

  loadPosition();

  panServo.attach(PAN_PIN);
  tiltServo.attach(TILT_PIN);
  panServo.write(panAngle);
  tiltServo.write(ANGLE_MAX - tiltAngle); // mirrored - see the comment in stepTilt()
  Serial.println("[Boot] Servos attached and moved to last saved position");

  Serial.println("D1 Mini Ready");
  sendPosition();

  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  Serial.print("[Boot] MQTT server set to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);
}

void loop() {
  if (!mqtt.connected()) {
    connectMqtt();
  }
  mqtt.loop();
  processPendingDirection();
  maybeSavePosition();

  /* ===== Serial control - same keys as 1.ino, pan and tilt directions flipped to
   * match the joystick's LEFT/RIGHT and UP/DOWN mapping above ===== */
  if (Serial.available()) {
    char cmd = Serial.read();
    Serial.print("[Serial<-] Received '");
    Serial.print(cmd);
    Serial.println("'");

    switch (cmd) {
      case 'q': stepPan(STEP);   break;
      case 'a': stepPan(-STEP);  break;
      case 'w': stepTilt(-STEP); break;
      case 's': stepTilt(STEP);  break;
      default:
        Serial.println("[Serial<-] Unrecognized command - ignored");
        break;
    }
  }
}
