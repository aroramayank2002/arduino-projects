/*
 * D1 Mini + PCA9685 robot arm bridge - WiFi + MQTT + Home Assistant discovery
 * ============================================================================
 * board: LOLIN(WEMOS) D1 R2 & mini
 * Libraries: ESP8266WiFi, PubSubClient, ArduinoJson (7.x), Adafruit_PWMServoDriver
 *
 * Combines:
 *   - d1_mini_pca8965_serial_control - same PCA9685 wiring/pulse range, and the per-joint
 *     channel + safe min/max + default servo angles found with it (see JOINTS below).
 *   - di_mini_mqtt_pan_tilt - same WiFi/MQTT pattern, latest-message-wins handling and
 *     smooth motion (but no EEPROM position persistence - the arm boots at its defaults).
 *
 * Architecture (docs/md/robot-arm-hardware.md): the app is the brain, the arm is a dumb
 * bridge. The home-assistant app's SERVO_ARM device ("first-servo-arm", sensor id 33)
 * owns the 6 joint angles, clamps and steps them, and publishes one retained JSON state:
 *   homeassistant/sensor/servo_arm_33/state
 *   {"name":"first-servo-arm","base":0.0,"boom":0.0,"arm":90.0,"wristPitch":0.0,"wrist":0.0,"grip":18.0}
 * (wrist = wrist ROLL, wristPitch = up/down bend - see docs/md/robot-arm.md.)
 * This sketch subscribes to it and moves 6 servos to match. It doesn't decide anything.
 *
 * Logical angle (app) -> servo angle (PCA9685), per joint - the CENTER_OFFSET idea from
 * robot-arm-hardware.md's calibration section:
 *   servo = servoDefault + (logical - logicalDefault) * scale,  clamped to [servoMin, servoMax]
 * servoDefault is the servo angle the joint physically rests at when the app says
 * logicalDefault. scale is +1 for 1:1 degrees, -1 if a servo is mounted mirrored (the
 * joint moves the wrong way), or another factor when the app's logical span doesn't match
 * the servo's travel (grip: 28 logical degrees -> ~56 servo degrees).
 * servoMin/servoMax are a hard safety clamp on the real hardware, whatever the app sends.
 *
 * Home Assistant: on every MQTT connect this publishes its own retained discovery config
 * (homeassistant/sensor/d1_mini_robot_arm/config), so it shows up in HA as its own device
 * "D1 Mini Robot Arm" with no app-side bridge - plus an availability topic (online, and
 * "offline" via MQTT last will when it drops off), and after each move its *actual* reached
 * position in the same JSON shape the app publishes, so the servo-arm dashboard template
 * works on it unchanged. Attributes carry category=hardware (picked up by the "Discovered
 * Devices" dashboard), the source device, the raw servo angles, and each joint's real
 * min/max in logical angles ("limits") for robotic-arm.html to show and respect.
 *
 * Home event: it also registers a Home Assistant "Home" BUTTON entity on the same device
 * (homeassistant/button/d1_mini_robot_arm_home/config) and subscribes to its command topic,
 *   homeassistant/button/d1_mini_robot_arm/home   payload: PRESS
 * On PRESS every joint ramps back to its servoDefault. Pressing it in HA's UI and pressing
 * Home / Reset on the app's page linked to this arm by name,
 *   http://localhost:4025/robotic-arm.html?device-name=D1 Mini Robot Arm
 * (POST /robotic-arm/home?device-name=..., topic template robotic-arm.home-command-topic)
 * send exactly the same message. The app slugifies the name into DEVICE_ID below
 * ("D1 Mini Robot Arm" -> d1_mini_robot_arm), so keep DEVICE_ID = DEVICE_NAME slugified.
 *
 * Position command: that page's joint buttons drive this arm directly too - every press/hold
 * publishes all 6 logical angles as one JSON message (not retained) to
 *   homeassistant/sensor/d1_mini_robot_arm/set   {"base":-23,"boom":0,"arm":90,...}
 * (POST /robotic-arm/position?device-name=..., topic template robotic-arm.command-topic),
 * the same shape as servo_arm_33's state, so it goes through the same handler. Whichever of
 * the two topics sent the latest message wins.
 *
 * Speed: the command may also carry "step" (1, 2 or 4) - degrees each joint moves per
 * SMOOTH_STEP_MS tick, i.e. the ramp speed (1 ~= 125 deg/s, 2 ~= 250, 4 ~= 500). It sticks
 * until the next message that names it (servo_arm_33's state and the Home button don't, so they
 * use the last one). Every state publish reports it back as "step" plus "speed" in deg/s.
 * A hobby servo has no speed input - it always rushes to its pulse width at full speed - so
 * speed here only means how fast we move the target; above roughly 500 deg/s (SG90 no-load
 * ~600 deg/s) the servo itself becomes the limit and "speed" overstates what it really does.
 *
 * ===== Fill in before flashing =====
 *   - WIFI_SSID / WIFI_PASSWORD if this D1 Mini is on a different network.
 *   - MQTT_TOPIC_ARM_STATE if the app's servo arm sensor id isn't 33. Confirm with:
 *       sqlite3 data/homeassistant.db \
 *         "SELECT id, name, entity_id FROM sensors WHERE type='SERVO_ARM';"
 *     sensor.servo_arm_33 -> homeassistant/sensor/servo_arm_33/state. Use the app's own
 *     entity_id column, not Home Assistant's slugified one.
 *   - JOINTS below - scale sign per joint, after watching each joint move once.
 *   - For a second arm: a new DEVICE_NAME, DEVICE_ID (its slug), UNIQUE_ID and
 *     MQTT_CLIENT_ID - every topic below is built from DEVICE_ID.
 *
 * Serial (115200) is kept for testing/calibration, same keys as the serial sketch:
 *   1-6 select joint (CH0-CH5), q = -2, a = +2 (servo degrees), h = home, p = print
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

/* ===== WIFI SETTINGS (same network as di_mini_mqtt_pan_tilt) ===== */
const char* WIFI_SSID     = "OWNIT-7B92";
const char* WIFI_PASSWORD = "NS3T7V55NKBKKR";

/* ===== MQTT SETTINGS ===== */
const char* MQTT_HOST      = "advait.se";
const int   MQTT_PORT      = 4023;
const char* MQTT_CLIENT_ID = "d1-mini-robot-arm";

// The app's SERVO_ARM device ("first-servo-arm") - see the header comment above.
const char* MQTT_TOPIC_ARM_STATE = "homeassistant/sensor/servo_arm_33/state";
const char* SOURCE_DEVICE_NAME   = "first-servo-arm";

/* ===== THIS DEVICE IN HOME ASSISTANT =====
 * UNIQUE_ID must stay the same forever once flashed - HA's device/entity registry keys
 * history and customizations off it (same rule as the app's discoveryId). */
const char* DEVICE_NAME       = "D1 Mini Robot Arm";
// DEVICE_NAME slugified (lowercase, non-alphanumerics -> "_") - what ?device-name= resolves to.
#define DEVICE_ID "d1_mini_robot_arm"
const char* UNIQUE_ID         = "d1-mini-robot-arm-01";
const char* FIRMWARE_VERSION  = "1.0";
const char* HARDWARE          = "d1-mini + pca9685";
const char* TOPIC_CONFIG       = "homeassistant/sensor/" DEVICE_ID "/config";
const char* TOPIC_STATE        = "homeassistant/sensor/" DEVICE_ID "/state";
const char* TOPIC_ATTRIBUTES   = "homeassistant/sensor/" DEVICE_ID "/attributes";
const char* TOPIC_AVAILABILITY = "homeassistant/sensor/" DEVICE_ID "/availability";

// Home button entity - TOPIC_HOME_COMMAND must match the app's robotic-arm.home-command-topic
// (homeassistant/button/{device}/home) with {device} = DEVICE_ID.
const char* HOME_UNIQUE_ID      = "d1-mini-robot-arm-01-home";
const char* TOPIC_HOME_CONFIG   = "homeassistant/button/" DEVICE_ID "_home/config";
const char* TOPIC_HOME_COMMAND  = "homeassistant/button/" DEVICE_ID "/home";
const char* HOME_PAYLOAD_PRESS  = "PRESS";

// Position command - must match the app's robotic-arm.command-topic
// (homeassistant/sensor/{device}/set) with {device} = DEVICE_ID.
const char* TOPIC_COMMAND       = "homeassistant/sensor/" DEVICE_ID "/set";

/* ===== PCA9685 (same as d1_mini_pca8965_serial_control) ===== */
#define SDA_PIN D2
#define SCL_PIN D1

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// SG90
#define SERVOMIN 102    // ~500 us
#define SERVOMAX 492    // ~2400 us

// MG946R alternative:
// #define SERVOMIN 102    // ~500 us
// #define SERVOMAX 512    // ~2500 us

/* ===== JOINT MAPPING =====
 * channel/servoMin/servoMax/servoDefault: from the serial sketch's menu (lines 75-80).
 * logicalDefault: the app's robotic-arm.joints.<key>.default-angle (application.properties).
 *
 * base: no default was noted, so 90 (the serial sketch's START_ANGLE). */
struct Joint {
  const char* key;      // key in the app's JSON state
  uint8_t channel;
  int servoMin;
  int servoMax;
  int servoDefault;
  float logicalDefault;
  float scale;
};

#define NUM_JOINTS 6

Joint JOINTS[NUM_JOINTS] = {
  // key          ch  min  max  def  logicalDef  scale
  { "base",       0,  10, 170,  90,   0.0,  1.0 },
  { "boom",       1,  30, 170,  90,   0.0,  1.0 },
  { "arm",        2,  10, 170,  94,  90.0,  1.0 },
  { "wristPitch", 3,  10, 170,  62,   0.0,  1.0 },
  { "wrist",      4,  20, 170,  74,   0.0,  1.0 },  // wrist ROLL
  { "grip",       5,  80, 138, 110,  18.0,  2.0 },  // app 6..34 -> servo 86..142 (clamped 138)
};

int currentAngle[NUM_JOINTS];  // servo angle last written to the PCA9685
int targetAngle[NUM_JOINTS];   // servo angle we're ramping toward

/* ===== Smooth motion =====
 * Non-blocking (unlike the pan/tilt sketch's delay() ramp - 6 joints ramping one after
 * another would block too long): every SMOOTH_STEP_MS, each joint not yet at its target
 * moves 1 degree toward it, all joints together. 8ms/degree ~= 125 deg/s, comfortably
 * faster than the app's 2 degrees per 100ms tick, so the arm keeps up with a held button.
 * stepDegrees (from the command's "step") scales that: N degrees per tick = N x 125 deg/s. */
const int SMOOTH_STEP_MS = 8;
const int MAX_STEP_DEGREES = 4;
int stepDegrees = 1;
unsigned long lastSmoothStepMillis = 0;
bool wasMoving = false;

/* ===== State publish throttle - while moving, at most every STATE_PUBLISH_MS; plus once
 * when every joint has settled, so HA always ends on the real resting position. ===== */
const int STATE_PUBLISH_MS = 200; // the app's robotic-arm.html animates from these
unsigned long lastStatePublishMillis = 0;

WiFiClient espClient;
PubSubClient mqtt(espClient);

int selectedJoint = -1; // serial control

/* No servo is driven until WiFi + MQTT are up and the first position/Home command arrives:
 * at boot every channel stays signal-less (servos limp), and the first command writes all six
 * straight to its targets - one jump, since the PCA9685 can't read back where they are.
 * After that, motion pauses whenever MQTT is down and resumes once it's back. */
bool servosActive = false;

bool motionAllowed() {
  return WiFi.status() == WL_CONNECTED && mqtt.connected();
}

void activateServos() {
  if (servosActive) return;
  servosActive = true;
  for (int i = 0; i < NUM_JOINTS; i++) {
    currentAngle[i] = targetAngle[i];
    writeServo(i, currentAngle[i]);
  }
  Serial.println("[Servo] First command - servos on");
  printPosition();
  publishState();
}

/* ===== Servo helpers ===== */
int clampServo(int i, int angle) {
  return constrain(angle, JOINTS[i].servoMin, JOINTS[i].servoMax);
}

void writeServo(int i, int angle) {
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(JOINTS[i].channel, 0, pulse);
}

int logicalToServo(int i, float logical) {
  const Joint& j = JOINTS[i];
  return clampServo(i, (int) lroundf(j.servoDefault + (logical - j.logicalDefault) * j.scale));
}

float servoToLogical(int i, int servo) {
  const Joint& j = JOINTS[i];
  return j.logicalDefault + (servo - j.servoDefault) / j.scale;
}

void setTarget(int i, int servoAngle) {
  targetAngle[i] = clampServo(i, servoAngle);
}

void stepTowardTargets() {
  if (!servosActive || !motionAllowed()) return;
  if (millis() - lastSmoothStepMillis < SMOOTH_STEP_MS) return;
  lastSmoothStepMillis = millis();

  bool moved = false;
  for (int i = 0; i < NUM_JOINTS; i++) {
    int diff = targetAngle[i] - currentAngle[i];
    if (diff == 0) continue;
    // Up to stepDegrees, never past the target.
    currentAngle[i] += constrain(diff, -stepDegrees, stepDegrees);
    writeServo(i, currentAngle[i]);
    moved = true;
  }

  if (moved) {
    wasMoving = true;
    if (millis() - lastStatePublishMillis >= STATE_PUBLISH_MS) publishState();
  } else if (wasMoving) {
    wasMoving = false;
    printPosition();
    publishState();
  }
}

/* ===== Serial readback ===== */
void printPosition() {
  for (int i = 0; i < NUM_JOINTS; i++) {
    Serial.print(JOINTS[i].key);
    Serial.print("=");
    Serial.print(currentAngle[i]);
    Serial.print(i < NUM_JOINTS - 1 ? ", " : "\n");
  }
}

/* ===== Home Assistant: discovery config, availability, state, attributes ===== */
void publishDiscovery() {
  JsonDocument doc;
  // name null + device block = HA's "device with one entity" convention - the entity
  // inherits the device's name (see MqttHomeAssistantPublisher's comment on this).
  doc["name"] = nullptr;
  doc["unique_id"] = UNIQUE_ID;
  doc["state_topic"] = TOPIC_STATE;
  doc["json_attributes_topic"] = TOPIC_ATTRIBUTES;
  doc["availability_topic"] = TOPIC_AVAILABILITY;
  doc["icon"] = "mdi:robot-industrial";
  addDeviceBlock(doc);

  char payload[640];
  serializeJson(doc, payload, sizeof(payload));
  bool ok = mqtt.publish(TOPIC_CONFIG, payload, true);
  Serial.print(ok ? "[HA] Discovery published: " : "[HA] Discovery FAILED: ");
  Serial.println(payload);

  // Home button - same device block, so HA shows it on this device's page. Named "Home"
  // (not null like the sensor above), since the device now has more than one entity.
  JsonDocument button;
  button["name"] = "Home";
  button["unique_id"] = HOME_UNIQUE_ID;
  button["command_topic"] = TOPIC_HOME_COMMAND;
  button["payload_press"] = HOME_PAYLOAD_PRESS;
  button["availability_topic"] = TOPIC_AVAILABILITY;
  button["icon"] = "mdi:home-import-outline";
  addDeviceBlock(button);

  serializeJson(button, payload, sizeof(payload));
  ok = mqtt.publish(TOPIC_HOME_CONFIG, payload, true);
  Serial.print(ok ? "[HA] Home button published: " : "[HA] Home button FAILED: ");
  Serial.println(payload);

  mqtt.publish(TOPIC_AVAILABILITY, "online", true);
}

void addDeviceBlock(JsonDocument& doc) {
  JsonObject device = doc["device"].to<JsonObject>();
  device["identifiers"].to<JsonArray>().add(UNIQUE_ID);
  device["name"] = DEVICE_NAME;
  device["model"] = "servo-arm";
  device["sw_version"] = FIRMWARE_VERSION;
  device["hw_version"] = HARDWARE;
}

/* State = the position the arm has actually reached, in the app's own JSON shape and
 * logical angles. Attributes = static device info + the raw servo angles. */
void publishState() {
  lastStatePublishMillis = millis();
  // Nothing written to the servos yet, so there's no real position to report.
  if (!mqtt.connected() || !servosActive) return;

  JsonDocument state;
  state["name"] = DEVICE_NAME;
  for (int i = 0; i < NUM_JOINTS; i++) {
    state[JOINTS[i].key] = lroundf(servoToLogical(i, currentAngle[i]));
  }
  // The ramp step this move ran at, and the speed it gives (servo degrees per second).
  state["step"] = stepDegrees;
  state["speed"] = stepDegrees * 1000 / SMOOTH_STEP_MS;
  char payload[256];
  serializeJson(state, payload, sizeof(payload));
  mqtt.publish(TOPIC_STATE, payload, true);

  JsonDocument attrs;
  attrs["category"] = "hardware";
  attrs["source_device"] = SOURCE_DEVICE_NAME;
  attrs["ip"] = WiFi.localIP().toString();
  JsonObject servo = attrs["servo_angles"].to<JsonObject>();
  for (int i = 0; i < NUM_JOINTS; i++) {
    servo[JOINTS[i].key] = currentAngle[i];
  }
  // This arm's real per-joint range in the app's logical angles (servoMin/servoMax mapped
  // back, rounded inward) - robotic-arm.html shows it and stops its buttons there, since the
  // hardware clamp may be tighter than the app's configured range (e.g. boom: -60, not -70).
  JsonObject limits = attrs["limits"].to<JsonObject>();
  for (int i = 0; i < NUM_JOINTS; i++) {
    float a = servoToLogical(i, JOINTS[i].servoMin);
    float b = servoToLogical(i, JOINTS[i].servoMax);
    JsonObject range = limits[JOINTS[i].key].to<JsonObject>();
    range["min"] = (int) ceilf(min(a, b));
    range["max"] = (int) floorf(max(a, b));
  }
  char attrPayload[768];
  serializeJson(attrs, attrPayload, sizeof(attrPayload));
  mqtt.publish(TOPIC_ATTRIBUTES, attrPayload, true);

  Serial.print("[MQTT->] ");
  Serial.println(payload);
}

/* ===== MQTT: joint state arriving from the app (servo_arm_33 state or our /set command) =====
 * Same split as the pan/tilt sketch: the callback only records the newest payload, and
 * loop() acts on it - so older messages queued while busy are superseded, not replayed. */
bool hasPendingState = false;
String pendingState;
bool hasPendingHome = false;

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_HOME_COMMAND) == 0) {
    hasPendingHome = length == strlen(HOME_PAYLOAD_PRESS)
        && memcmp(payload, HOME_PAYLOAD_PRESS, length) == 0;
    return;
  }
  pendingState = "";
  pendingState.reserve(length);
  for (unsigned int i = 0; i < length; i++) {
    pendingState += (char) payload[i];
  }
  hasPendingState = true;
}

/* Home event (HA button or robotic-arm.html) - every joint ramps back to its servoDefault.
 * A joint state message arriving after it still wins, same latest-message-wins rule. */
void processPendingHome() {
  if (!hasPendingHome) return;
  hasPendingHome = false;
  Serial.println("[MQTT<-] Home event");
  homeAllJoints();
  activateServos();
}

void homeAllJoints() {
  for (int i = 0; i < NUM_JOINTS; i++) setTarget(i, JOINTS[i].servoDefault);
}

void processPendingState() {
  if (!hasPendingState) return;
  hasPendingState = false;

  if (pendingState.length() == 0) {
    Serial.println("[MQTT<-] Empty state (device deleted in the app?) - ignored");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, pendingState);
  if (err) {
    Serial.print("[MQTT<-] Bad JSON (");
    Serial.print(err.c_str());
    Serial.print("): ");
    Serial.println(pendingState);
    return;
  }

  Serial.print("[MQTT<-] ");
  Serial.println(pendingState);

  // Ramp step first, so this message's move already runs at it.
  JsonVariant step = doc["step"];
  if (step.is<int>() || step.is<float>()) {
    int newStep = constrain((int) lroundf(step.as<float>()), 1, MAX_STEP_DEGREES);
    if (newStep != stepDegrees) {
      stepDegrees = newStep;
      Serial.print("[MQTT<-] Step ");
      Serial.print(stepDegrees);
      Serial.print(" deg/tick = ");
      Serial.print(stepDegrees * 1000 / SMOOTH_STEP_MS);
      Serial.println(" deg/s");
    }
  }

  // Missing keys keep their current target - a partial message moves only what it names.
  for (int i = 0; i < NUM_JOINTS; i++) {
    JsonVariant v = doc[JOINTS[i].key];
    if (v.is<float>() || v.is<int>()) {
      setTarget(i, logicalToServo(i, v.as<float>()));
    }
  }
  activateServos();
}

/* ===== Connectivity ===== */
void connectWiFi() {
  Serial.print("[WiFi] Connecting to SSID \"");
  Serial.print(WIFI_SSID);
  Serial.println("\"");

  WiFi.mode(WIFI_STA);
  // ESP8266 defaults to modem sleep: the radio dozes between AP beacons, so a command can
  // sit at the router for up to ~1s+ (measured: ping 7ms..3.6s, MQTT set->move 16ms..1.3s).
  // The arm is mains powered and must react to a held button now, so keep the radio awake.
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    attempt++;
    Serial.print("[WiFi] Still connecting... (attempt ");
    Serial.print(attempt);
    Serial.println(")");
  }

  Serial.print("[WiFi] Connected. IP=");
  Serial.print(WiFi.localIP());
  Serial.print(" RSSI=");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");
}

/* Non-blocking: one attempt per RETRY_MS, so Serial control and the servo ramp keep
 * working while the broker is unreachable. */
const unsigned long MQTT_RETRY_MS = 3000;
unsigned long lastMqttAttemptMillis = 0;

void maintainMqtt() {
  if (mqtt.connected()) return;
  if (lastMqttAttemptMillis != 0 && millis() - lastMqttAttemptMillis < MQTT_RETRY_MS) return;
  lastMqttAttemptMillis = millis();

  Serial.print("[MQTT] Connecting to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.print(MQTT_PORT);
  Serial.print(" ... ");

  // Last will: the broker publishes "offline" (retained) for us if we drop off,
  // so HA greys the device out instead of showing a stale position as live.
  if (mqtt.connect(MQTT_CLIENT_ID, nullptr, nullptr, TOPIC_AVAILABILITY, 1, true, "offline")) {
    Serial.println("connected");
    publishDiscovery();
    publishState();
    mqtt.subscribe(MQTT_TOPIC_ARM_STATE);
    Serial.print("[MQTT] Subscribed to ");
    Serial.println(MQTT_TOPIC_ARM_STATE);
    mqtt.subscribe(TOPIC_HOME_COMMAND);
    Serial.print("[MQTT] Subscribed to ");
    Serial.println(TOPIC_HOME_COMMAND);
    mqtt.subscribe(TOPIC_COMMAND);
    Serial.print("[MQTT] Subscribed to ");
    Serial.println(TOPIC_COMMAND);
  } else {
    Serial.print("failed, rc=");
    Serial.print(mqtt.state());
    Serial.println(" - retrying in 3s");
  }
}

/* ===== Serial control (testing/calibration) ===== */
void printMenu() {
  Serial.println();
  Serial.println("==============================");
  Serial.println(" PCA9685 ROBOT ARM (MQTT)");
  Serial.println("==============================");
  for (int i = 0; i < NUM_JOINTS; i++) {
    Serial.print(i + 1);
    Serial.print(" = CH");
    Serial.print(JOINTS[i].channel);
    Serial.print(" ");
    Serial.println(JOINTS[i].key);
  }
  Serial.println("q = -2 degrees, a = +2 degrees (selected joint)");
  Serial.println("h = home (servo defaults), p = print position");
  Serial.println();
}

void handleSerial() {
  if (!Serial.available()) return;
  char cmd = Serial.read();

  if (cmd >= '1' && cmd < '1' + NUM_JOINTS) {
    selectedJoint = cmd - '1';
    Serial.print("[Serial] Selected ");
    Serial.print(JOINTS[selectedJoint].key);
    Serial.print(" at ");
    Serial.println(targetAngle[selectedJoint]);
  } else if ((cmd == 'q' || cmd == 'a') && selectedJoint >= 0) {
    setTarget(selectedJoint, targetAngle[selectedJoint] + (cmd == 'a' ? 2 : -2));
    Serial.print("[Serial] ");
    Serial.print(JOINTS[selectedJoint].key);
    Serial.print(" -> ");
    Serial.println(targetAngle[selectedJoint]);
  } else if (cmd == 'h') {
    homeAllJoints();
    Serial.println("[Serial] Home");
  } else if (cmd == 'p') {
    printPosition();
  } else if (cmd == '\n' || cmd == '\r') {
    // ignore line endings
  } else {
    printMenu();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("[Boot] D1 Mini Robot Arm starting up");

  Wire.begin(SDA_PIN, SCL_PIN);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);

  for (int ch = 0; ch < 16; ch++) {
    pwm.setPWM(ch, 0, 0); // no signal on unused channels
  }

  // Servos stay signal-less (above) until WiFi + MQTT are up and a command arrives - see
  // servosActive. Until then, joints a first partial command doesn't name go to servoDefault.
  for (int i = 0; i < NUM_JOINTS; i++) {
    currentAngle[i] = targetAngle[i] = JOINTS[i].servoDefault;
  }
  Serial.println("[Boot] Servos off until WiFi + MQTT connect");

  printMenu();

  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(1024); // default 256 is too small for the discovery config
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    maintainMqtt();
    mqtt.loop();
  }
  processPendingHome();
  processPendingState();
  stepTowardTargets();
  handleSerial();
}
