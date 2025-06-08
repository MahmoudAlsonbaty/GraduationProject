#include <AccelStepper.h>
#include <Arduino.h>

// Motor pins
#define DIR_PIN  19
#define STEP_PIN 18

// Limit switch pins
#define LIMIT_MIN_PIN  14   // Start of ball screw
#define LIMIT_MAX_PIN  27   // End of ball screw

#define CALIBRATION_SPEED 1000 // Speed during calibration
#define CALIBRATION_ACCELERATION 500 // Acceleration during calibration

void calibrateLimits();

// Stepper config
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Calibration
long maxPosition = 0; // Will be determined via calibration

// Slider target (can be set via serial or UI later)
long targetPosition = 0;

// Basic setup
void setup() {
  Serial.begin(9600);

  pinMode(LIMIT_MIN_PIN, INPUT_PULLDOWN);
  pinMode(LIMIT_MAX_PIN, INPUT_PULLDOWN);
  stepper.setEnablePin(23); // Enable pin for the stepper driver
  stepper.setPinsInverted(false, false, true);  // Invert enable pin logic
  stepper.enableOutputs();
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

  Serial.println("Starting calibration...");
  calibrateLimits();
  Serial.println("Calibration complete.");
  Serial.print("Max position: ");
  Serial.println(maxPosition);
}

void loop() {
  // Example: Set a target from 0 to maxPosition
  if (Serial.available()) {
    long pos = Serial.parseInt();
    if (pos >= 0 && pos <= maxPosition) {
      targetPosition = pos;
      stepper.moveTo(targetPosition);
    }
  }

  stepper.run();
}

// Move to the limit switch slowly until it triggers
void calibrateLimits() {
  // Move to MIN limit switch
  stepper.setMaxSpeed(CALIBRATION_SPEED);
  stepper.setAcceleration(CALIBRATION_ACCELERATION);
  stepper.moveTo(-100000); // Move backward a lot

  while (digitalRead(LIMIT_MIN_PIN) == LOW) {
   //TODO: Remove this debug print in production
    Serial.print("POS " );
    Serial.print(stepper.currentPosition());
    Serial.print(" SPEED " );
    Serial.print(stepper.speed());
    Serial.print(" ACC " );
    Serial.println(stepper.acceleration());
    stepper.run();
  }

  stepper.setCurrentPosition(0); // Set zero position

  // STOP motor before reversing direction
  stepper.stop();                  
  stepper.setSpeed(0);
  while (stepper.isRunning()) stepper.run();  // Ensure it fully stops
  stepper.setCurrentPosition(0);              // Reset again if needed

  // Set new motion toward MAX limit
  stepper.setMaxSpeed(CALIBRATION_SPEED);
  stepper.setAcceleration(CALIBRATION_ACCELERATION);  // Keep acceleration but reapply cleanly
  stepper.moveTo(100000);        // Move forward a lot

  while (digitalRead(LIMIT_MAX_PIN) == LOW) {
    //TODO: Remove this debug print in production
    Serial.print("POS " );
    Serial.print(stepper.currentPosition());
    Serial.print(" SPEED " );
    Serial.print(stepper.speed());
    Serial.print(" ACC " );
    Serial.println(stepper.acceleration());
    stepper.run();
  }

  maxPosition = stepper.currentPosition();

    // STOP motor before reversing direction
  stepper.stop();                  
  stepper.setSpeed(0);
    // Set new motion toward MAX limit
  stepper.setMaxSpeed(CALIBRATION_SPEED);
  stepper.setAcceleration(CALIBRATION_ACCELERATION);  // Keep acceleration but reapply cleanly

  // Return to home
  stepper.moveTo(0);
  while (stepper.distanceToGo() != 0) {
    //TODO: Remove this debug print in production
    Serial.print("POS " );
    Serial.print(stepper.currentPosition());
    Serial.print(" SPEED " );
    Serial.print(stepper.speed());
    Serial.print(" ACC " );
    Serial.println(stepper.acceleration());
    stepper.run();
  }
}

