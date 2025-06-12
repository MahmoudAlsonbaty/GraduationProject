#include <AccelStepper.h>
#include <Arduino.h>

// Motor pins Horizontal (X-axis)
#define HOR_DIR_PIN  19
#define HOR_STEP_PIN 18
#define HOR_ENABLE_PIN 23
// Limit switch pins Horizontal (X-axis)
#define HOR_LIMIT_MIN_PIN  14
#define HOR_LIMIT_MAX_PIN  27
#define HOR_LIMIT_PULLUP 32 

// Motor pins Vertical (Y-axis) 
#define VERT_DIR_PIN  5
#define VERT_STEP_PIN 17
#define VERT_ENABLE_PIN 22
// Limit switch pins Vertical (Y-axis)
#define VERT_LIMIT_MIN_PIN 26
#define VERT_LIMIT_MAX_PIN 25
#define VERT_LIMIT_PULLUP 21

#define CALIBRATION_SPEED 1000
#define CALIBRATION_ACCELERATION 500

AccelStepper HOR_Stepper(AccelStepper::DRIVER, HOR_STEP_PIN, HOR_DIR_PIN);
AccelStepper VERT_Stepper(AccelStepper::DRIVER, VERT_STEP_PIN, VERT_DIR_PIN);

bool connected = false;
long HOR_maxPosition = 0;
long VERT_maxPosition = 0;
long targetPosition = 0;
bool isCalibrated = false;

void calibrateLimits();
void moveToPosition(long pos);
void grabDrugs(String command);

void setup() {
  Serial.begin(9600);
  pinMode(HOR_LIMIT_MIN_PIN, INPUT_PULLDOWN);
  pinMode(HOR_LIMIT_MAX_PIN, INPUT_PULLDOWN);
  pinMode(VERT_LIMIT_MIN_PIN, INPUT_PULLDOWN);
  pinMode(VERT_LIMIT_MAX_PIN, INPUT_PULLDOWN);

  pinMode(HOR_LIMIT_PULLUP, OUTPUT);
  pinMode(VERT_LIMIT_PULLUP, OUTPUT);

  digitalWrite(HOR_LIMIT_PULLUP, HIGH); // Enable pull-up for horizontal limit switches
  digitalWrite(VERT_LIMIT_PULLUP, HIGH); // Enable pull-up for vertical limit switches

  HOR_Stepper.setEnablePin(HOR_ENABLE_PIN);
  HOR_Stepper.setPinsInverted(false, false, true);
  HOR_Stepper.enableOutputs();
  HOR_Stepper.setMaxSpeed(1000);
  HOR_Stepper.setAcceleration(500);

  VERT_Stepper.setEnablePin(VERT_ENABLE_PIN);
  VERT_Stepper.setPinsInverted(false, false, true);
  VERT_Stepper.enableOutputs();
  VERT_Stepper.setMaxSpeed(1000);
  VERT_Stepper.setAcceleration(500);

  Serial.println("ESP32 Ready. Waiting for handshake...");

  // while(true){
  //   if(digitalRead(VERT_LIMIT_MAX_PIN)){
  //     Serial.println("VERt LImit MAX ON");
  //   }
  //    if(digitalRead(VERT_LIMIT_MIN_PIN)){
  //     Serial.println("VERt LImit MIN ON");
  //   } if(digitalRead(HOR_LIMIT_MAX_PIN)){
  //     Serial.println("HOR LImit MAX ON");
  //   } if(digitalRead(HOR_LIMIT_MIN_PIN)){
  //     Serial.println("HOR LImit MIN ON");
  //   }

  // }

  while (!connected) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      if (cmd == "HANDSHAKE") {
        Serial.println("HANDSHAKE_DONE");
        connected = true;
        break;
      }
    }
  }

  Serial.println("ESP32 Setup complete. Ready for commands.");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "CALIBRATE") {
      Serial.println("CALIBRATION_START");
      calibrateLimits();
      isCalibrated = true;
      Serial.println("CALIBRATION_DONE");
    } else if (input.startsWith("GRAB")) {
      if (!isCalibrated) {
        Serial.println("ERROR: NOT_CALIBRATED");
        return;
      }
      String command = input.substring(5);
      grabDrugs(command);
    }
  }

  HOR_Stepper.run();
}

void moveToPosition(long pos) {
  HOR_Stepper.moveTo(pos);
  while (HOR_Stepper.distanceToGo() != 0) {
    HOR_Stepper.run();
  }
}

void calibrateLimits() {
  //
  // HORIZONTAL CALIBRATION
  //
  HOR_Stepper.setMaxSpeed(CALIBRATION_SPEED);
  HOR_Stepper.setAcceleration(CALIBRATION_ACCELERATION);

  // Move to MIN limit
  HOR_Stepper.moveTo(100000);
  while (digitalRead(HOR_LIMIT_MIN_PIN) == LOW) {
    HOR_Stepper.run();
  }
  HOR_Stepper.setCurrentPosition(0);
  HOR_Stepper.stop();
  while (HOR_Stepper.isRunning()) HOR_Stepper.run();

  // Move to MAX limit
  HOR_Stepper.moveTo(-100000);
  while (digitalRead(HOR_LIMIT_MAX_PIN) == LOW) {
    HOR_Stepper.run();
  }
  HOR_maxPosition = HOR_Stepper.currentPosition();

  HOR_Stepper.stop();
  while (HOR_Stepper.isRunning()) HOR_Stepper.run();

  // Return home
  HOR_Stepper.moveTo(0);
  while (HOR_Stepper.distanceToGo() != 0) {
    HOR_Stepper.run();
  }

  Serial.println("Vertical Calibration Starting");

  //
  //! VERTICAL CALIBRATION
  //

  VERT_Stepper.setMaxSpeed(CALIBRATION_SPEED);
  VERT_Stepper.setAcceleration(CALIBRATION_ACCELERATION);

  // Move to MIN limit
  VERT_Stepper.moveTo(100000);
  while (digitalRead(VERT_LIMIT_MIN_PIN) == LOW) {
    VERT_Stepper.run();
  }
  VERT_Stepper.setCurrentPosition(0);
  VERT_Stepper.stop();
  while (VERT_Stepper.isRunning()) VERT_Stepper.run();

  // Move to MAX limit
  VERT_Stepper.moveTo(-100000);
  while (digitalRead(VERT_LIMIT_MAX_PIN) == LOW) {
    VERT_Stepper.run();
  }
  VERT_maxPosition = VERT_Stepper.currentPosition();

  VERT_Stepper.stop();
  while (VERT_Stepper.isRunning()) VERT_Stepper.run();

  // Return home
  VERT_Stepper.moveTo(0);
  while (VERT_Stepper.distanceToGo() != 0) {
    VERT_Stepper.run();
  }

}

void grabDrugs(String command) {
  command += ','; // Append a comma to ensure the last pair is processed
  int start = 0;

  while (start < command.length()) {
    int commaIndex = command.indexOf(',', start);
    if (commaIndex == -1) break;

    String pair = command.substring(start, commaIndex);
    int xIndex = pair.indexOf('x');
    if (xIndex == -1 || xIndex == 0 || xIndex == pair.length() - 1) {
      Serial.println("ERROR: INVALID_FORMAT");
      return;
    }

    int slot = pair.substring(0, xIndex).toInt();
    int quantity = pair.substring(xIndex + 1).toInt();

    if (slot < 1 || slot > 24 || quantity < 1) {
      Serial.println("ERROR: INVALID_SLOT_OR_QUANTITY");
      return;
    }

    long stepPerSlot = HOR_maxPosition / 24;
    targetPosition = stepPerSlot * (slot - 1);
    Serial.print("MOVING_TO_SLOT ");
    Serial.println(slot);
    moveToPosition(targetPosition);

    // Simulate grab
    Serial.print("(");
    Serial.print((slot < 10) ? "0" : "");
    Serial.print(slot);
    Serial.print("x");
    Serial.print((quantity < 10) ? "0" : "");
    Serial.print(quantity);
    Serial.println(")");

    start = commaIndex + 1;
  }

  Serial.println("ALL_GRABS_DONE");
}
