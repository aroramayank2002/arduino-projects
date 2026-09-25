// Code of RCWL-0516 Radar Sensor tested successfully
// tested 10 Meter range successfully with an obstacle (wall)
// This module requires a proper positioning and a good power supply
// This module does not work properly with USB (PC) power supply

void setup() {
  Serial.begin(9600);
  pinMode(12, INPUT);
  pinMode(13, OUTPUT);

}

void loop() {
  if(digitalRead(12)>0){
    digitalWrite(13, HIGH);
    Serial.println("MOTION DETECTED!");
    delay(2000);
  }
  else{
    digitalWrite(13, LOW);
  }
  

}
