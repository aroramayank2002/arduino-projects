#include <DHT.h> // Requires the "DHT sensor library" by Adafruit

#define DHTPIN 13     // use degital pin no 13 of Arduino Mega 
#define DHTTYPE DHT11 // Hum DHT11 sensor ka use kar rahe hain

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  // delay 2 seconds for read next reading
  delay(2000); 

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Celsius mein reading

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C | ");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

