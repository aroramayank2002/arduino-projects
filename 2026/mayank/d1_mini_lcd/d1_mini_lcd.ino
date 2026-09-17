#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN D3
#define SCL_PIN D2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {

  Serial.begin(9600);
  delay(2000);

  Serial.println();
  Serial.println("=== D1 Mini OLED Test ===");
  Serial.println("Starting...");

  // Initialize I2C
  Serial.println("Initializing I2C...");
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("SDA: D3");
  Serial.println("SCL: D2");

  // Initialize OLED
  Serial.println("Initializing OLED...");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("ERROR: OLED NOT FOUND!");
    Serial.println("Check:");
    Serial.println("- VCC");
    Serial.println("- GND");
    Serial.println("- SDA");
    Serial.println("- SCL");
    Serial.println("- OLED address (0x3C)");

    while (true) {
      delay(100);
    }
  }

  Serial.println("OLED FOUND!");
  Serial.println("Displaying Hello...");

  // Clear screen
  display.clearDisplay();

  // Text settings
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 25);

  display.println("Hello!");

  // Send buffer to display
  display.display();

  Serial.println("OLED display updated.");
  Serial.println("Setup complete.");
}

void loop() {
}