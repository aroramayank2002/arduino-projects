// board: LOLIN(WEMOS) D1 R2 & mini
#include <Wire.h>
#include <U8g2lib.h>

// SDA = D3, SCL = D2
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(
  U8G2_R0,
  U8X8_PIN_NONE,
  D7,  // SCL
  D6   // SDA
);

void setup() {
  Wire.begin(D6, D7);

  display.begin();

  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 15, "Hello World!");
  display.drawStr(0, 30, "D1 Mini");
  display.sendBuffer();
}

void loop() {
}