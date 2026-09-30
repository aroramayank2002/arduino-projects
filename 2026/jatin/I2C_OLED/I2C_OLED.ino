//#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define screen dimensions for the OLED panel
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The parameter -1 indicates that the display does not share an external reset pin
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C // Typical I2C address for 0.96" OLED modules

// create object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
Serial.begin(9600);

// Initialize the I2C communication and display
display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

// Clear the buffer initialization screen (Adafruit splash screen)
display.clearDisplay();

display.setTextSize(2);  // Normal 2:1 pixel scale (readable size)
display.setTextColor(SSD1306_WHITE); // Draw white text
display.setCursor(42, 0);  // Set position coordinate (X, Y)
display.println("OLED"); // Write message to local display buffer
display.setCursor(20, 16);  // Set position coordinate (X, Y)
display.println("SSD1306"); // Write message to local display buffer

display.setCursor(20, 33);  // Set position coordinate (X, Y)
display.println("TESTING");

display.setCursor(25, 50);  // Set position coordinate (X, Y)
display.println("128*64");

/*display.setCursor(0, 48);  // Set position coordinate (X, Y)
display.println("my name jatin chawla");

display.setCursor(0, 32);  // Set position coordinate (X, Y)
display.println("my name jatin chawla");

display.setCursor(0, 40);  // Set position coordinate (X, Y)
display.println("my name jatin chawla");

display.setCursor(0, 48);  // Set position coordinate (X, Y)
display.println("my name jatin chawla");

display.setCursor(0, 56);  // Set position coordinate (X, Y)
display.println("my name jatin chawla");*/

display.display(); // Display massage on OLED display

}

void loop() {
  // put your main code here, to run repeatedly:

}
