#include <ESP32Servo.h>
#include <Arduino.h>
Servo myservo1;  // create first servo object
Servo myservo2;  // create second servo object

//! Don't forget to check PWM pins that are usable
int servoPin1 = 13;
int servoPin2 = 12; // choose another available pin for the second servo

void setup() {
    Serial.begin(9600); // Start serial communication
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myservo1.setPeriodHertz(50);    // standard 50 hz servo
    myservo2.setPeriodHertz(50);
    myservo1.attach(servoPin1, 500, 2400); // attaches the first servo
    myservo2.attach(servoPin2, 500, 2400); // attaches the second servo
    Serial.println("Send: servo_number angle (e.g., 1 90 or 2 45)");
}

void loop() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        int spaceIndex = input.indexOf(' ');
        if (spaceIndex > 0) {
            int servoNum = input.substring(0, spaceIndex).toInt();
            int angle = input.substring(spaceIndex + 1).toInt();
            if (angle >= 0 && angle <= 180) {
                if (servoNum == 1) {
                    myservo1.write(angle);
                    Serial.print("Servo 1 moved to angle: ");
                    Serial.println(angle);
                } else if (servoNum == 2) {
                    myservo2.write(angle);
                    Serial.print("Servo 2 moved to angle: ");
                    Serial.println(angle);
                } else {
                    Serial.println("Invalid servo number. Use 1 or 2.");
                }
            } else {
                Serial.println("Invalid angle. Enter 0-180.");
            }
        } else {
            Serial.println("Invalid input. Use: servo_number angle (e.g., 1 90)");
        }
    }
}