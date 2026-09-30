// Code of RCWL-0516 Radar Sensor tested successfully
// tested 10 Meter range successfully with an obstacle (wall)
// This module requires a proper positioning and a good power supply
// This module does not work properly with USB (PC) power supply

void setup() {
  Serial.begin(9600);
  pinMode(24, INPUT);
  pinMode(25, OUTPUT);

}

void loop() {
  if(digitalRead(24)>0){
    digitalWrite(25, HIGH);
    Serial.println("MOTION DETECTED!");
    delay(2000);
  }
  else{
    digitalWrite(25, LOW);
  }
  

}
