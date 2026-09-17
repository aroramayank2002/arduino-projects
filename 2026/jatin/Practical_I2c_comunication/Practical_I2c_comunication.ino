#include <SPI.h>
#include <Wire.h>                     // I2C library
#include <Adafruit_PWMServoDriver.h>  //PCA9685 Servo module library
#include <Adafruit_GFX.h>             // OLED library
#include <Adafruit_SSD1306.h>         // OLED library
#include <BH1750.h>                   // Light (lux) module library

// Define screen dimensions for the OLED panel
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // The parameter -1 indicates that the display does not share an external reset pin
#define SCREEN_ADDRESS 0x3C // Typical I2C address for 0.96" OLED modules
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // create object

BH1750 lightMeter;  // create an object (lux module)

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // PCA9685 module address

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pwm.begin();
  pwm.setPWMFreq(50);  // Servo = 50 Hz
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextSize(2);               // Normal 2:1 pixel scale (readable size)
  display.setTextColor(SSD1306_WHITE);  // Draw white text
}

void loop() {
  int lux = lightMeter.readLightLevel();  // Read LUX value from sensor

  int servo = map(lux, 0, 15000, 150, 600);  // Mapping lux into pwm
  int angle = map(servo, 150, 600, 0, 180);  // mapping pwm into degree
  pwm.setPWM(0, 0, servo);

  display.clearDisplay();   // Clear the buffer initialization screen (Adafruit splash screen)
  display.setCursor(0, 0);  // Set position coordinate (X, Y)
  display.print("LUX ");
  display.print(lux);  // Write message to local display buffer

  display.setCursor(0, 20);  // Set position coordinate (X, Y)
  display.print("Angle ");
  display.print(angle);  // Write message to local display buffer
  display.display();     // Display massage on OLED display
  delay(200);
}
