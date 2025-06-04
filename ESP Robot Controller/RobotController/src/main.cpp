#include <Arduino.h>

#define STEP_PIN_1 18
#define DIR_PIN_1 19
#define EN_PIN_1   23

#define STEP_PIN_2 21
#define DIR_PIN_2 22
#define EN_PIN_2   25

int stepCount = 200;          // Default value
int stepDelay = 500;          // Default delay in microseconds

void moveMotors(int steps, int delayMicros) {
  for (int i = 0; i < steps; i++) {
    digitalWrite(STEP_PIN_1, HIGH);
    digitalWrite(STEP_PIN_2, HIGH);
    delayMicroseconds(delayMicros);
    digitalWrite(STEP_PIN_1, LOW);
    digitalWrite(STEP_PIN_2, LOW);
    delayMicroseconds(delayMicros);
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(STEP_PIN_1, OUTPUT);
  pinMode(DIR_PIN_1, OUTPUT);
  pinMode(EN_PIN_1, OUTPUT);

  pinMode(STEP_PIN_2, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);
  pinMode(EN_PIN_2, OUTPUT);

  digitalWrite(EN_PIN_1, LOW);  // Enable drivers
  digitalWrite(EN_PIN_2, LOW);

  digitalWrite(DIR_PIN_1, HIGH); // Default direction
  digitalWrite(DIR_PIN_2, HIGH);

  Serial.println("Enter step count and delay in microseconds (e.g., 1000 500):");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    int spaceIndex = input.indexOf(' ');
    if (spaceIndex > 0) {
      stepCount = input.substring(0, spaceIndex).toInt();
      stepDelay = input.substring(spaceIndex + 1).toInt();

      Serial.print("Running ");
      Serial.print(stepCount);
      Serial.print(" steps with ");
      Serial.print(stepDelay);
      Serial.println("us delay.");

      moveMotors(stepCount, stepDelay);

      // Reverse direction
      digitalWrite(DIR_PIN_1, LOW);
      digitalWrite(DIR_PIN_2, LOW);
      delay(1000);

      moveMotors(stepCount, stepDelay);

      // Reset
      digitalWrite(DIR_PIN_1, HIGH);
      digitalWrite(DIR_PIN_2, HIGH);
      delay(1000);
    } else {
      Serial.println("Invalid input. Format: steps delayMicroseconds");
    }
  }
}


