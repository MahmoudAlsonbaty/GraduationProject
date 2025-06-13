#include <Servo.h>
#include <Arduino.h>

Servo servo1;
Servo servo2;

void setup() {
//! Don't forget to check PWM pins that are usable
    servo1.attach(9);   // First servo on pin 9
  servo2.attach(10);  // Second servo on pin 10
  Serial.begin(9600);
  Serial.println("Send: S1 <angle> or S2 <angle> (e.g., S1 90)");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("S1")) {
      int angle = cmd.substring(2).toInt();
      if (angle >= 0 && angle <= 180) {
        servo1.write(angle);
        Serial.print("Servo1 moved to: ");
        Serial.println(angle);
      } else {
        Serial.println("Invalid angle for Servo1.");
      }
    } else if (cmd.startsWith("S2")) {
      int angle = cmd.substring(2).toInt();
      if (angle >= 0 && angle <= 180) {
        servo2.write(angle);
        Serial.print("Servo2 moved to: ");
        Serial.println(angle);
      } else {
        Serial.println("Invalid angle for Servo2.");
      }
    } else {
      Serial.println("Invalid command. Use S1 <angle> or S2 <angle>.");
    }
  }
}