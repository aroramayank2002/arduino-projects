#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// font style
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

// Mega pins
#define TFT_CS   10
#define TFT_DC    9
#define TFT_RST   8

Adafruit_ST7735 tft = Adafruit_ST7735 (TFT_CS, TFT_DC, TFT_RST);

void setup() {

  Serial.begin(9600);
 // 1.8 inch 128x160 ST7735
  tft.initR(INITR_BLACKTAB);

  tft.setRotation(0); // 0 for Potrait and 1 for Landscap

  tft.fillScreen(ST77XX_YELLOW);
  delay(2000);

  tft.fillScreen(ST77XX_RED);
  delay(2000);

  tft.fillScreen(ST77XX_GREEN);
  delay(2000);

  tft.fillScreen(ST77XX_MAGENTA);
  delay(2000);

  tft.fillScreen(ST77XX_WHITE);
  delay(2000);
  // Border
  tft.drawRect(0, 0, 128, 160, ST77XX_YELLOW);

  // Text
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setCursor(12, 20);
  tft.println("TFT TEST");

  tft.setCursor(10, 55);
  tft.println("128 x 160");

  tft.setCursor(15, 90);
  tft.println("1.8 SPI");
  delay(2000);

  tft.fillScreen(ST77XX_WHITE);

  tft.setCursor(35, 10);
  tft.println("MENU");

  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(5, 30);
  tft.println("1 TEMPRATURE SENSOR");

  tft.setCursor(5, 40);
  tft.println("2 SERVO MOTOR");

  tft.setCursor(5, 50);
  tft.println("3 TOUCH SENSOR");

  tft.setCursor(5, 60);
  tft.println("4 MOTOR DRIVER");

  tft.setCursor(5, 70);
  tft.println("5 GAS SENSOR");

  tft.setCursor(5, 80);
  tft.println("6 LIGHT SENSOR");

  tft.setCursor(5, 90);
  tft.println("7 ULTRASONIC SENSOR");

  tft.setCursor(5, 100);
  tft.println("8 OLED TESTING");

  tft.setCursor(5, 110);
  tft.println("9 MOTION SENSOR");

  tft.setCursor(3, 120);
  tft.println("10 READ SWITCH TEST");

  tft.setCursor(3, 130);
  tft.println("11 LIDAR SENSOR");

  tft.setCursor(3, 140);
  tft.println("12 RADAR SENSOR");

}

void loop() {
  
}
  /*tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(10, 120);
  tft.println("Arduino Mega");

  tft.setFont(&FreeMono9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLUE);
  tft.setCursor(10, 50);
  tft.println("Arduino Mega");*/