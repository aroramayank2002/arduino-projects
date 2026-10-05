#include <Wire.h>
#include <U8g2lib.h>

/*
Module    D1 mini

CON       D4
SDA       D5
SCL       D2
PSH       D1
TRA       D0
TRB       D7
BAK       A0
GND       GND
VCC       5V
*/

#define CON D4
#define PSH D1
#define TRA D0
#define TRB D7
#define BAK A0

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(
  U8G2_R0,
  U8X8_PIN_NONE,
  D2,
  D5
);

const char text[] = "jatin bajaj!     rakshit     ";
const int textLength = sizeof(text) - 1;

int scrollPosition = 0;
int lastTRA = HIGH;

// PSH
bool pshPressed = false;
bool blinking = false;
bool blinkVisible = true;

// BAK
bool bakPressed = false;
bool bakTextVisible = false;

// CON
bool conPressed = false;
bool conTextVisible = false;

unsigned long lastEncoderTime = 0;
unsigned long lastBlinkTime = 0;
unsigned long lastBakTime = 0;
unsigned long lastConTime = 0;

const unsigned long encoderDelay = 5;
const unsigned long blinkInterval = 300;
const unsigned long buttonDebounce = 100;


// --------------------------------------------------
// DISPLAY
// --------------------------------------------------

void showText(bool visible) {

  display.clearBuffer();

  // Main scrolling text
  if (visible) {

    display.setFont(u8g2_font_ncenB08_tr);

    char output[21];

    for (int i = 0; i < 20; i++) {
      int index = (scrollPosition + i) % textLength;
      output[i] = text[index];
    }

    output[20] = '\0';

    display.drawStr(0, 20, output);
  }


  // CON text - left
  if (conTextVisible) {

    display.setFont(u8g2_font_6x10_tr);

    display.drawStr(0, 10, "CON");
  }


  // BAK text - right
  if (bakTextVisible) {

    display.setFont(u8g2_font_6x10_tr);

    display.drawStr(100, 10, "BACK");
  }

  display.sendBuffer();
}


// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {

  Serial.begin(115200);

  pinMode(CON, INPUT_PULLUP);

  pinMode(PSH, INPUT_PULLUP);
  pinMode(TRA, INPUT_PULLUP);
  pinMode(TRB, INPUT_PULLUP);

  Wire.begin(D5, D2);

  display.begin();

  lastTRA = digitalRead(TRA);

  showText(true);

  Serial.println("System started");
}


// --------------------------------------------------
// LOOP
// --------------------------------------------------

void loop() {

  unsigned long now = millis();


  // =================================================
  // PSH BUTTON - BLINK TOGGLE
  // =================================================

  bool currentPSH = digitalRead(PSH);

  if (currentPSH == LOW) {

    if (!pshPressed &&
        now - lastBlinkTime > buttonDebounce) {

      pshPressed = true;

      // Toggle blinking
      blinking = !blinking;

      if (blinking) {

        blinkVisible = true;
        lastBlinkTime = now;

        Serial.println("PSH: blinking ON");

        showText(true);

      } else {

        // Stop blinking and make text visible
        blinkVisible = true;

        Serial.println("PSH: blinking OFF");

        showText(true);
      }
    }

  } else {

    pshPressed = false;
  }


  // =================================================
  // BLINK
  // =================================================

  if (blinking &&
      now - lastBlinkTime >= blinkInterval) {

    lastBlinkTime = now;

    blinkVisible = !blinkVisible;

    showText(blinkVisible);
  }


  // =================================================
  // ROTARY ENCODER
  // =================================================

  int currentTRA = digitalRead(TRA);

  if (currentTRA != lastTRA) {

    if (now - lastEncoderTime > encoderDelay) {

      if (currentTRA == LOW) {

        if (digitalRead(TRB) == HIGH) {
          scrollPosition++;
          Serial.println("Scroll forward");
        } else {
          scrollPosition--;
          Serial.println("Scroll backward");
        }

        if (scrollPosition >= textLength) {
          scrollPosition = 0;
        }

        if (scrollPosition < 0) {
          scrollPosition = textLength - 1;
        }

        if (!blinking || blinkVisible) {
          showText(true);
        }
      }

      lastEncoderTime = now;
    }

    lastTRA = currentTRA;
  }


  // =================================================
  // BAK BUTTON - A0
  // =================================================

  int bakValue = analogRead(A0);

  bool currentBAK = (bakValue < 200);

  if (currentBAK) {

    if (!bakPressed &&
        now - lastBakTime > buttonDebounce) {

      bakPressed = true;

      bakTextVisible = !bakTextVisible;

      showText(!blinking || blinkVisible);

      Serial.print("BAK: ");
      Serial.println(bakTextVisible ? "ON" : "OFF");

      lastBakTime = now;
    }

  } else {

    bakPressed = false;
  }


  // =================================================
  // CON BUTTON - D4
  // =================================================

  bool currentCON = digitalRead(CON);

  if (currentCON == LOW) {

    if (!conPressed &&
        now - lastConTime > buttonDebounce) {

      conPressed = true;

      conTextVisible = !conTextVisible;

      showText(!blinking || blinkVisible);

      Serial.print("CON: ");
      Serial.println(conTextVisible ? "ON" : "OFF");

      lastConTime = now;
    }

  } else {

    conPressed = false;
  }
}