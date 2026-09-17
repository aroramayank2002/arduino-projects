// logs ntp time every minute
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

/* ===== WIFI ===== */
const char* ssid = "OWNIT-7B92";
const char* password = "NS3T7V55NKBKKR";

/* ===== NTP ===== */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
// 0 = UTC offset (change if needed)

/* ===== TIMER ===== */
unsigned long lastLogTime = 0;
const unsigned long logInterval = 60000; // 1 minute

void setup() {

  Serial.begin(115200);

  /* ---- Connect WiFi ---- */
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  /* ---- Start NTP ---- */
  timeClient.begin();
  timeClient.update();
}

void loop() {

  timeClient.update();

  unsigned long currentMillis = millis();

  if (currentMillis - lastLogTime >= logInterval) {

    lastLogTime = currentMillis;

    int hour = timeClient.getHours();
    int minute = timeClient.getMinutes();
    int second = timeClient.getSeconds();

    Serial.printf("Current Time: %02d:%02d:%02d\n",
                  hour,
                  minute,
                  second);

    Serial.println("Time logged successfully.");
  }
}