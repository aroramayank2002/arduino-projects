// working: hardcoded wifi connection, received button D5 and from mqtt button at topic esp/curtain/toggle (1 or 0) to turn two servos
// in mirror fashion on D8 and D7
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

/* ===== WIFI SETTINGS ===== */
const char* ssid     = "OWNIT-7B92";
const char* password = "NS3T7V55NKBKKR";

/* ===== MQTT SETTINGS ===== */
const char* mqtt_server = "mqtt.iotbhai.io";
const int   mqtt_port   = 1883;

const char* topic_state  = "esp/cutrain/state";
const char* topic_toggle = "esp/curtain/toggle";

/* ===== PIN DEFINITIONS ===== */
#define BUTTON_PIN D5
#define SERVO1_PIN D8
#define SERVO2_PIN D7
#define LED_PIN    LED_BUILTIN

/* ===== SERVO POSITIONS ===== */
const int OPEN_S1  = 20;
const int OPEN_S2  = 110;
const int CLOSE_S1 = 110;
const int CLOSE_S2 = 20;

Servo servo1;
Servo servo2;

WiFiClient espClient;
PubSubClient mqtt(espClient);

bool curtainOpen = true;

/* ===== BUTTON DEBOUNCE ===== */
bool lastStableState = HIGH;
bool lastReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

/* ===== SMOOTH SERVO MOVE ===== */
void moveServosSmooth(int target1, int target2) {
  int current1 = servo1.read();
  int current2 = servo2.read();

  while (current1 != target1 || current2 != target2) {

    if (current1 < target1) current1++;
    if (current1 > target1) current1--;

    if (current2 < target2) current2++;
    if (current2 > target2) current2--;

    servo1.write(current1);
    servo2.write(current2);

    delay(15);
  }
}

/* ===== TOGGLE CURTAIN FUNCTION ===== */
void toggleCurtain() {

  curtainOpen = !curtainOpen;

  if (curtainOpen) {
    Serial.println("Opening curtains...");
    moveServosSmooth(OPEN_S1, OPEN_S2);
  } else {
    Serial.println("Closing curtains...");
    moveServosSmooth(CLOSE_S1, CLOSE_S2);
  }

  String payload = curtainOpen ? "1" : "0";
  //mqtt.publish(topic_state, payload.c_str(), true);

  bool ok = mqtt.publish(topic_state, payload.c_str(), true);

  Serial.print("Publish result: ");
  Serial.println(ok);
  Serial.println("State published");

  // Blink LED
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
}

/* ===== MQTT CALLBACK ===== */
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  payload[length] = '\0';
  String message = String((char*)payload);

  Serial.print("MQTT Message received on: ");
  Serial.println(topic);

  Serial.print("Payload: ");
  Serial.println(message);

  if (String(topic) == topic_toggle && message == "1") {
    toggleCurtain();
  }

  // Blink LED on any MQTT message
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
}

/* ===== WIFI CONNECT ===== */
void setupWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

/* ===== MQTT RECONNECT ===== */
void reconnectMQTT() {
  while (!mqtt.connected()) {

    Serial.print("Connecting to MQTT...");

    if (mqtt.connect("ESP_CURTAIN")) {
      Serial.println("connected!");

      mqtt.subscribe(topic_toggle);
      Serial.println("Subscribed to toggle topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying...");
      delay(3000);
    }
  }
}

/* ===== SETUP ===== */
void setup() {

  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);

  // Start OPEN
  servo1.write(OPEN_S1);
  servo2.write(OPEN_S2);

  setupWiFi();

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
}

/* ===== LOOP ===== */
void loop() {

  if (!mqtt.connected()) {
    reconnectMQTT();
  }

  mqtt.loop();

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = millis();
    lastReading = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != lastStableState) {
      lastStableState = reading;

      if (reading == LOW) {
        toggleCurtain();
      }
    }
  }
}