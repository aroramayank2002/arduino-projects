/*
 * D1 Mini + PCA9685 robot arm bridge - WiFi + MQTT + Home Assistant discovery
 * ============================================================================
 * board: LOLIN(WEMOS) D1 R2 & mini
 * Libraries: ESP8266WiFi, PubSubClient, ArduinoJson (7.x), Adafruit_PWMServoDriver
 *
 * Combines:
 *   - d1_mini_pca8965_serial_control - same PCA9685 wiring/pulse range, and the per-joint
 *     channel + safe min/max + default servo angles found with it (see JOINTS below).
 *   - di_mini_mqtt_pan_tilt - same WiFi/MQTT pattern, latest-message-wins handling,
 *     smooth motion and EEPROM "last stable position" persistence.
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
 * Devices" dashboard), the source device, and the raw servo angles.
 *
 * Home event: it also registers a Home Assistant "Home" BUTTON entity on the same device
 * (homeassistant/button/d1_mini_robot_arm_home/config) and subscribes to its command topic,
 *   homeassistant/button/d1_mini_robot_arm/home   payload: PRESS
 * On PRESS every joint ramps back to its servoDefault. Pressing it in HA's UI and pressing
 * Home / Reset on the app's robotic-arm.html page (POST /robotic-arm/home, topic set by
 * robotic-arm.home-command-topic) send exactly the same message.
 *
 * ===== Fill in before flashing =====
 *   - WIFI_SSID / WIFI_PASSWORD if this D1 Mini is on a different network.
 *   - MQTT_TOPIC_ARM_STATE if the app's servo arm sensor id isn't 33. Confirm with:
 *       sqlite3 data/homeassistant.db \
 *         "SELECT id, name, entity_id FROM sensors WHERE type='SERVO_ARM';"
 *     sensor.servo_arm_33 -> homeassistant/sensor/servo_arm_33/state. Use the app's own
 *     entity_id column, not Home Assistant's slugified one.
 *   - JOINTS below - scale sign per joint, after watching each joint move once.
 *
 * Serial (115200) is kept for testing/calibration, same keys as the serial sketch:
 *   1-6 select joint (CH0-CH5), q = -2, a = +2 (servo degrees), h = home, p = print
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <EEPROM.h>

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
const char* UNIQUE_ID         = "d1-mini-robot-arm-01";
const char* FIRMWARE_VERSION  = "1.0";
const char* HARDWARE          = "d1-mini + pca9685";
const char* TOPIC_CONFIG       = "homeassistant/sensor/d1_mini_robot_arm/config";
const char* TOPIC_STATE        = "homeassistant/sensor/d1_mini_robot_arm/state";
const char* TOPIC_ATTRIBUTES   = "homeassistant/sensor/d1_mini_robot_arm/attributes";
const char* TOPIC_AVAILABILITY = "homeassistant/sensor/d1_mini_robot_arm/availability";

// Home button entity - TOPIC_HOME_COMMAND must match the app's robotic-arm.home-command-topic.
const char* HOME_UNIQUE_ID      = "d1-mini-robot-arm-01-home";
const char* TOPIC_HOME_CONFIG   = "homeassistant/button/d1_mini_robot_arm_home/config";
const char* TOPIC_HOME_COMMAND  = "homeassistant/button/d1_mini_robot_arm/home";
const char* HOME_PAYLOAD_PRESS  = "PRESS";

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
 * faster than the app's 2 degrees per 100ms tick, so the arm keeps up with a held button. */
const int SMOOTH_STEP_MS = 8;
unsigned long lastSmoothStepMillis = 0;
bool wasMoving = false;

/* ===== State publish throttle - while moving, at most every STATE_PUBLISH_MS; plus once
 * when every joint has settled, so HA always ends on the real resting position. ===== */
const int STATE_PUBLISH_MS = 500;
unsigned long lastStatePublishMillis = 0;

/* ===== Last-stable-position persistence (same idea as the pan/tilt sketch) =====
 * The PCA9685 can't read a servo's position back, so on boot we assume the arm is still
 * where it was last left and write that, instead of snapping every joint to its default.
 * The retained MQTT state arriving right after connect is then ramped to smoothly. */
const int POSITION_SAVE_DELAY_MS = 2500;
const int EEPROM_SIZE   = 1 + NUM_JOINTS;
const byte EEPROM_MAGIC = 0x52; // arbitrary - distinguishes a saved position from erased flash

unsigned long lastMoveMillis = 0;
bool positionDirty = false;

WiFiClient espClient;
PubSubClient mqtt(espClient);

int selectedJoint = -1; // serial control

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

bool anyJointMoving() {
  for (int i = 0; i < NUM_JOINTS; i++) {
    if (currentAngle[i] != targetAngle[i]) return true;
  }
  return false;
}

void stepTowardTargets() {
  if (millis() - lastSmoothStepMillis < SMOOTH_STEP_MS) return;
  lastSmoothStepMillis = millis();

  bool moved = false;
  for (int i = 0; i < NUM_JOINTS; i++) {
    if (currentAngle[i] == targetAngle[i]) continue;
    currentAngle[i] += (targetAngle[i] > currentAngle[i]) ? 1 : -1;
    writeServo(i, currentAngle[i]);
    moved = true;
  }

  if (moved) {
    lastMoveMillis = millis();
    positionDirty = true;
    wasMoving = true;
    if (millis() - lastStatePublishMillis >= STATE_PUBLISH_MS) publishState();
  } else if (wasMoving) {
    wasMoving = false;
    printPosition();
    publishState();
  }
}

/* ===== EEPROM ===== */
void loadPosition() {
  EEPROM.begin(EEPROM_SIZE);
  bool saved = EEPROM.read(0) == EEPROM_MAGIC;
  for (int i = 0; i < NUM_JOINTS; i++) {
    currentAngle[i] = clampServo(i, saved ? (int) EEPROM.read(1 + i) : JOINTS[i].servoDefault);
    targetAngle[i] = currentAngle[i];
  }
  if (saved) {
    Serial.println("[EEPROM] Loaded last position");
  } else {
    Serial.println("[EEPROM] No saved position yet - using defaults");
    positionDirty = true;
    lastMoveMillis = millis();
  }
}

void savePosition() {
  EEPROM.write(0, EEPROM_MAGIC);
  for (int i = 0; i < NUM_JOINTS; i++) {
    EEPROM.write(1 + i, currentAngle[i]);
  }
  EEPROM.commit();
  Serial.println("[EEPROM] Saved stable position");
}

void maybeSavePosition() {
  if (positionDirty && millis() - lastMoveMillis >= POSITION_SAVE_DELAY_MS) {
    savePosition();
    positionDirty = false;
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
  if (!mqtt.connected()) return;

  JsonDocument state;
  state["name"] = DEVICE_NAME;
  for (int i = 0; i < NUM_JOINTS; i++) {
    state[JOINTS[i].key] = lroundf(servoToLogical(i, currentAngle[i]));
  }
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
  char attrPayload[384];
  serializeJson(attrs, attrPayload, sizeof(attrPayload));
  mqtt.publish(TOPIC_ATTRIBUTES, attrPayload, true);

  Serial.print("[MQTT->] ");
  Serial.println(payload);
}

/* ===== MQTT: joint state arriving from the app =====
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

  // Missing keys keep their current target - a partial message moves only what it names.
  for (int i = 0; i < NUM_JOINTS; i++) {
    JsonVariant v = doc[JOINTS[i].key];
    if (v.is<float>() || v.is<int>()) {
      setTarget(i, logicalToServo(i, v.as<float>()));
    }
  }
}

/* ===== Connectivity ===== */
void connectWiFi() {
  Serial.print("[WiFi] Connecting to SSID \"");
  Serial.print(WIFI_SSID);
  Serial.println("\"");

  WiFi.mode(WIFI_STA);
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

  loadPosition();
  for (int i = 0; i < NUM_JOINTS; i++) {
    writeServo(i, currentAngle[i]);
  }
  Serial.println("[Boot] Servos holding last saved position");
  printPosition();
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
  maybeSavePosition();
  handleSerial();
}
