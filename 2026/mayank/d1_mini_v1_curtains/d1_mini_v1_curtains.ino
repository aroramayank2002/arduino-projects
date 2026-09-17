#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>
#include <Servo.h>
#include <ESP8266WebServer.h>




/* ================= PIN DEFINITIONS ================= */

#define SERVO1_PIN D8
#define SERVO2_PIN D7
#define BUTTON_PIN D5
#define LOCK_PIN   D0
#define LDR_PIN    A0
#define STATUS_LED D4

/* ================= OBJECTS ================= */

WiFiClient espClient;
PubSubClient mqtt(espClient);
ESP8266WebServer server(80);
Servo servo1;
Servo servo2;

/* ================= CONFIG VARIABLES ================= */

String mqtt_host, mqtt_user, mqtt_pass;
String topic_curtain, topic_lock, topic_state;
int mqtt_port;
int ldr_threshold = 600;

bool curtainClosed = false;
bool lockEnabled = false;
bool mqttLockOverride = false;

/* ================= SERVO POSITIONS ================= */

const int OPEN_S1 = 20;
const int OPEN_S2 = 110;
const int CLOSE_S1 = 110;
const int CLOSE_S2 = 20;

int targetS1 = OPEN_S1;
int targetS2 = OPEN_S2;
int currentS1 = OPEN_S1;
int currentS2 = OPEN_S2;

unsigned long lastServoStep = 0;
const int servoInterval = 15;

/* ================= MQTT QUEUE ================= */

#define QUEUE_SIZE 10

struct MqttMessage {
  String topic;
  String payload;
  bool retained;
};

MqttMessage queue[QUEUE_SIZE];
int qHead = 0;
int qTail = 0;

bool enqueue(String topic, String payload, bool retained=false) {
  int next = (qTail + 1) % QUEUE_SIZE;
  if (next == qHead) {
    Serial.println("[QUEUE] Full");
    return false;
  }
  queue[qTail] = {topic, payload, retained};
  qTail = next;
  return true;
}

void processQueue() {
  if (!mqtt.connected()) return;

  while (qHead != qTail) {
    MqttMessage msg = queue[qHead];
    if (mqtt.publish(msg.topic.c_str(), msg.payload.c_str(), msg.retained)) {
      Serial.println("[MQTT OUT] " + msg.topic);
      qHead = (qHead + 1) % QUEUE_SIZE;
    } else {
      break;
    }
  }
}

/* ================= LED BLINK ================= */

unsigned long ledUntil = 0;

void blinkLED() {
  digitalWrite(STATUS_LED, LOW);
  ledUntil = millis() + 150;
}

void handleLED() {
  if (ledUntil && millis() > ledUntil) {
    digitalWrite(STATUS_LED, HIGH);
    ledUntil = 0;
  }
}

/* ================= CURTAIN CONTROL ================= */

void setCurtain(bool closeCurtain) {

  if (lockEnabled) closeCurtain = false;

  if (closeCurtain) {
    targetS1 = CLOSE_S1;
    targetS2 = CLOSE_S2;
    curtainClosed = true;
    Serial.println("[CURTAIN] Closing");
  } else {
    targetS1 = OPEN_S1;
    targetS2 = OPEN_S2;
    curtainClosed = false;
    Serial.println("[CURTAIN] Opening");
  }

  enqueue(topic_state, closeCurtain ? "1" : "0", true);
}

/* ================= SERVO NON-BLOCKING ================= */

void updateServos() {

  if (millis() - lastServoStep < servoInterval) return;
  lastServoStep = millis();

  if (currentS1 == targetS1 && currentS2 == targetS2) return;

  if (currentS1 < targetS1) currentS1++;
  if (currentS1 > targetS1) currentS1--;

  if (currentS2 < targetS2) currentS2++;
  if (currentS2 > targetS2) currentS2--;

  servo1.write(currentS1);
  servo2.write(currentS2);
}

/* ================= RESET WINDOW ================= */

void resetWindow() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  unsigned long t = millis();
  while (millis() - t < 5000) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("[RESET] Clearing config");
      LittleFS.remove("/config.json");
      ESP.restart();
    }
    yield();
  }
}

/* ================= STORAGE ================= */

void saveConfig() {
  StaticJsonDocument<512> doc;
  doc["mqtt_host"] = mqtt_host;
  doc["mqtt_port"] = mqtt_port;
  doc["mqtt_user"] = mqtt_user;
  doc["mqtt_pass"] = mqtt_pass;
  doc["topic_curtain"] = topic_curtain;
  doc["topic_lock"] = topic_lock;
  doc["topic_state"] = topic_state;
  doc["ldr_threshold"] = ldr_threshold;
  doc["lockEnabled"] = lockEnabled;

  File f = LittleFS.open("/config.json", "w");
  serializeJson(doc, f);
  f.close();
}

bool loadConfig() {
  if (!LittleFS.exists("/config.json")) return false;
  File f = LittleFS.open("/config.json", "r");
  StaticJsonDocument<512> doc;
  deserializeJson(doc, f);
  f.close();

  mqtt_host = doc["mqtt_host"].as<String>();
  mqtt_port = doc["mqtt_port"];
  mqtt_user = doc["mqtt_user"].as<String>();
  mqtt_pass = doc["mqtt_pass"].as<String>();
  topic_curtain = doc["topic_curtain"].as<String>();
  topic_lock = doc["topic_lock"].as<String>();
  topic_state = doc["topic_state"].as<String>();
  ldr_threshold = doc["ldr_threshold"];
  lockEnabled = doc["lockEnabled"];
  return true;
}

/* ================= WIFI PORTAL ================= */

void startPortal() {

  WiFiManager wm;
  
  WiFiManagerParameter p_host("host","MQTT Host","mqtt.iotbhai.io",40);
  //WiFiManagerParameter p_host("host","MQTT Host","broker.freemqtt.com",40);
  WiFiManagerParameter p_port("port","MQTT Port","1883",6);
  WiFiManagerParameter p_user("user","MQTT User","freemqtt",20);
  WiFiManagerParameter p_pass("pass","MQTT Pass","public",20);
  WiFiManagerParameter p_curtain("curtain","Curtain Topic","esp/curtain/cmd",40);
  WiFiManagerParameter p_lock("lock","Lock Topic","esp/curtain/lock",40);
  WiFiManagerParameter p_state("state","State Topic","esp/curtain/state",40);
  WiFiManagerParameter p_ldr("ldr","LDR Threshold","600",6);

  wm.addParameter(&p_host);
  wm.addParameter(&p_port);
  wm.addParameter(&p_user);
  wm.addParameter(&p_pass);
  wm.addParameter(&p_curtain);
  wm.addParameter(&p_lock);
  wm.addParameter(&p_state);
  wm.addParameter(&p_ldr);

  wm.startConfigPortal("Curtain_Setup");

  mqtt_host = p_host.getValue();
  mqtt_port = atoi(p_port.getValue());
  mqtt_user = p_user.getValue();
  mqtt_pass = p_pass.getValue();
  topic_curtain = p_curtain.getValue();
  topic_lock = p_lock.getValue();
  topic_state = p_state.getValue();
  ldr_threshold = atoi(p_ldr.getValue());

  saveConfig();
}

/* ================= MQTT ================= */

unsigned long lastMqttAttempt = 0;

void mqttReconnect() {
  if (mqtt.connected()) return;

  if (millis() - lastMqttAttempt < 5000) return;
  lastMqttAttempt = millis();

  Serial.println("[MQTT] Connecting...");

  if (mqtt.connect("ESP_CURTAIN",
                   mqtt_user.c_str(),
                   mqtt_pass.c_str())) {

    Serial.println("[MQTT] Connected");

    mqtt.subscribe(topic_curtain.c_str());
    mqtt.subscribe(topic_lock.c_str());

    enqueue("esp/curtain/status", "online", true);
  }
}

void mqttCallback(char* t, byte* p, unsigned int len) {

  p[len] = 0;
  String msg = (char*)p;
  String topic = String(t);

  Serial.println("[MQTT IN] " + topic + " -> " + msg);
  blinkLED();

  StaticJsonDocument<200> ack;
  ack["topic"] = topic;
  ack["payload"] = msg;
  ack["status"] = "received";

  char buf[200];
  serializeJson(ack, buf);
  enqueue("esp/curtain/ack", buf);

  if (topic == topic_curtain && !lockEnabled) {
    if (msg == "1") setCurtain(true);
    if (msg == "0") setCurtain(false);
  }

  if (topic == topic_lock) {
    if (msg == "1") {
      mqttLockOverride = true;
      lockEnabled = true;
      setCurtain(false);
    } else {
      mqttLockOverride = false;
      lockEnabled = false;
    }
    saveConfig();
  }
}

/* ================= SETUP ================= */

void setup() {

  Serial.begin(115200);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);

  LittleFS.begin();
  resetWindow();

  if (!loadConfig())
    startPortal();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LOCK_PIN, INPUT_PULLUP);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  servo1.write(currentS1);
  servo2.write(currentS2);

  mqtt.setServer(mqtt_host.c_str(), mqtt_port);
  mqtt.setCallback(mqttCallback);

  ArduinoOTA.begin();
}

/* ================= LOOP ================= */

void loop() {

  ArduinoOTA.handle();
  mqtt.loop();
  mqttReconnect();
  processQueue();
  handleLED();
  updateServos();

  /* --- Physical Button D5 --- */
  static bool lastBtn = HIGH;
  bool btn = digitalRead(BUTTON_PIN);

  if (btn == LOW && lastBtn == HIGH && !lockEnabled) {
    setCurtain(!curtainClosed);
    delay(200);
  }
  lastBtn = btn;

  /* --- Lock Switch D0 --- */
  if (!mqttLockOverride) {
    bool physicalLock = (digitalRead(LOCK_PIN) == LOW);
    if (physicalLock != lockEnabled) {
      lockEnabled = physicalLock;
      if (lockEnabled) setCurtain(false);
      saveConfig();
    }
  }

  /* --- LDR Auto Close --- */
  int ldr = analogRead(LDR_PIN);
  if (ldr > ldr_threshold && !curtainClosed && !lockEnabled) {
    Serial.println("[LDR] Threshold exceeded");
    setCurtain(true);
  }
}