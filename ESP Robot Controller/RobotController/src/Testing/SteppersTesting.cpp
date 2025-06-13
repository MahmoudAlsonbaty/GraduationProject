#include <AccelStepper.h>
#include <Arduino.h>

//! if motors don't stop immediatly when hitting the limit switch, use disableOutputs() to stop them
//! This might add future issues but test them just in case

//! The Calibrate both function uses the disable_outputs() function to stop the motors
//! But the Calibrate uses the traditional stop() function

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

// Speed
#define CALIBRATION_SPEED 1000
#define CALIBRATION_ACCELERATION 500

//
// Distances for grabbing
// These values are in steps, you can find the specific values using the other Script
//
long ROW_1 = 10;
long ROW_2 = 20;
long ROW_3 = 30;

long COLUMN_1 = 10;
long COLUMN_2 = 20;
long COLUMN_3 = 30;
long COLUMN_4 = 40;
long COLUMN_5 = 50;
long COLUMN_6 = 60;
long COLUMN_7 = 70;
long COLUMN_8 = 80;

long HOR_DROP_OFF_POSITION = 1000; // Horizontal position for drop-off
long VERT_DROP_OFF_POSITION = 5000; // Vertical position for drop-off

AccelStepper HOR_Stepper(AccelStepper::DRIVER, HOR_STEP_PIN, HOR_DIR_PIN);
AccelStepper VERT_Stepper(AccelStepper::DRIVER, VERT_STEP_PIN, VERT_DIR_PIN);

bool connected = false;
long HOR_maxPosition = 0;
long VERT_maxPosition = 0;
long targetPosition = 0;
bool isCalibrated = false;

void calibrateLimits();
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
    }else if (input.startsWith("GRAB")) {
      if (!isCalibrated) {
        Serial.println("ERROR: NOT_CALIBRATED");
        return;
      }
      String command = input.substring(5);
      grabDrugs(command);
    }else if (input.startsWith("GO")) {
      // Example: GO1000,7000
      if (!isCalibrated) {
        Serial.println("ERROR: NOT_CALIBRATED");
        return;
      }
      String command = input.substring(3);
      command.trim();
      int commaIdx = command.indexOf(',');
      if (commaIdx == -1) {
        Serial.println("ERROR: INVALID_GO_FORMAT");
        return;
      }
      long xPos = command.substring(0, commaIdx).toInt();
      long yPos = command.substring(commaIdx + 1).toInt();
      
      while(HOR_Stepper.distanceToGo() != 0 || VERT_Stepper.distanceToGo() != 0) {
        HOR_Stepper.run();
        VERT_Stepper.run();
      }

    }else if (input.startsWith("SET")) {
      // Example: SETC1,150 or SETR1,5000
      String setCmd = input.substring(3); // Remove "SET"
      setCmd.trim();
      int commaIdx = setCmd.indexOf(',');
      if (commaIdx == -1) {
        Serial.println("ERROR: INVALID_SET_FORMAT");
        return;
      }
      String var = setCmd.substring(0, commaIdx);
      long value = setCmd.substring(commaIdx + 1).toInt();

      if (var.startsWith("C")) {
        int col = var.substring(1).toInt();
        switch (col) {
          case 1: COLUMN_1 = value; break;
          case 2: COLUMN_2 = value; break;
          case 3: COLUMN_3 = value; break;
          case 4: COLUMN_4 = value; break;
          case 5: COLUMN_5 = value; break;
          case 6: COLUMN_6 = value; break;
          case 7: COLUMN_7 = value; break;
          case 8: COLUMN_8 = value; break;
          default: Serial.println("ERROR: INVALID_COLUMN"); return;
        }
        Serial.print("COLUMN_"); Serial.print(col); Serial.print(" SET TO "); Serial.println(value);
      } else if (var.startsWith("R")) {
        int row = var.substring(1).toInt();
        switch (row) {
          case 1: ROW_1 = value; break;
          case 2: ROW_2 = value; break;
          case 3: ROW_3 = value; break;
          default: Serial.println("ERROR: INVALID_ROW"); return;
        }
        Serial.print("ROW_"); Serial.print(row); Serial.print(" SET TO "); Serial.println(value);
      } else if (var == "HDROP") { //SETHDROP,1234 -> Sets the drop off to 1234
        HOR_DROP_OFF_POSITION = value;
        Serial.print("HOR_DROP_OFF_POSITION SET TO "); Serial.println(value);
      } else if (var == "VDROP") {
        VERT_DROP_OFF_POSITION = value;
        Serial.print("VERT_DROP_OFF_POSITION SET TO "); Serial.println(value);
      } else {
        Serial.println("ERROR: INVALID_SET_TARGET");
      }
    }
  }
}

void calibrateBoth(){
    HOR_Stepper.setMaxSpeed(CALIBRATION_SPEED);
    HOR_Stepper.setAcceleration(CALIBRATION_ACCELERATION);

    VERT_Stepper.setMaxSpeed(CALIBRATION_SPEED);
    VERT_Stepper.setAcceleration(CALIBRATION_ACCELERATION);

    bool horLimitMin = false;
    bool horLimitMax = false;
    bool vertLimitMin = false;
    bool vertLimitMax = false;

    //Starting Both Calibration
    Serial.println("Starting Calibration For MIN directions for both steppers.");
    HOR_Stepper.moveTo(100000);
    VERT_Stepper.moveTo(100000);
    while(!horLimitMin || !vertLimitMin){
        horLimitMin = digitalRead(HOR_LIMIT_MIN_PIN);
        vertLimitMin = digitalRead(VERT_LIMIT_MIN_PIN);
        if(horLimitMin){
            HOR_Stepper.disableOutputs(); // Immediately disables the stepper outputs (motor stops instantly)
        }
        if(vertLimitMin){
            VERT_Stepper.disableOutputs(); // Same for vertical stepper
        }
        HOR_Stepper.run();
        VERT_Stepper.run();
    }
    Serial.println("Reached MIN limits for both steppers.");
    HOR_Stepper.setCurrentPosition(0);
    HOR_Stepper.stop();
    while (HOR_Stepper.isRunning()) HOR_Stepper.run();

    VERT_Stepper.setCurrentPosition(0);
    VERT_Stepper.stop();
    while (VERT_Stepper.isRunning()) VERT_Stepper.run();
    
    delay(1000); // Wait for a second to ensure everything is stable

    HOR_Stepper.enableOutputs(); // Immediately enables the stepper outputs (motor starts)
    VERT_Stepper.enableOutputs(); // Same for vertical stepper


    Serial.println("Starting Calibration For MAX directions for both steppers.");
    // Move to MAX limit
    HOR_Stepper.moveTo(-100000);
    VERT_Stepper.moveTo(-100000);

    while(!horLimitMax || !vertLimitMax){
        horLimitMax = digitalRead(HOR_LIMIT_MAX_PIN);
        vertLimitMax = digitalRead(VERT_LIMIT_MAX_PIN);
        if(horLimitMax){
            HOR_Stepper.disableOutputs(); // Immediately disables the stepper outputs (motor stops instantly)
        }
        if(vertLimitMax){
            VERT_Stepper.disableOutputs(); // Same for vertical stepper
        }
        HOR_Stepper.run();
        VERT_Stepper.run();
    }
    HOR_maxPosition = HOR_Stepper.currentPosition();
    VERT_maxPosition = VERT_Stepper.currentPosition();
    
    HOR_Stepper.stop();
    while (HOR_Stepper.isRunning()) HOR_Stepper.run();
    
    VERT_Stepper.stop();
    while (VERT_Stepper.isRunning()) VERT_Stepper.run();
    
    delay(1000); // Wait for a second to ensure everything is stable
    Serial.println("Reached MAX limits for both steppers.");

    HOR_Stepper.enableOutputs(); // Immediately enables the stepper outputs (motor starts)
    VERT_Stepper.enableOutputs(); // Same for vertical stepper


    Serial.println("Calibration complete For Both.");
    Serial.print("Horizontal Max Position: ");
    Serial.print(HOR_maxPosition);
    Serial.print(" ,Vertical Max Position: ");
    Serial.println(VERT_maxPosition);
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
  HOR_Stepper.moveTo(HOR_maxPosition / 2); // Move to the middle position
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
  VERT_Stepper.moveTo(VERT_maxPosition / 2); // Move to the middle position
  while (VERT_Stepper.distanceToGo() != 0) {
    VERT_Stepper.run();
  }

}

//SLOTxQUANTITY
//00x01,02x03,04x05,06x07,08x09,10x11,12x13,14x15,16x17,18x19,20x21,22x23
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

    //
    goToDrug(slot);
    dropOFF(); // Drop off the medication after grabbing
    Serial.print("Grabbed medication from slot ");

    start = commaIndex + 1;
  }

  Serial.println("ALL_GRABS_DONE");
}


void dropOFF(){
  HOR_Stepper.moveTo(HOR_DROP_OFF_POSITION);
  VERT_Stepper.moveTo(VERT_DROP_OFF_POSITION);
  while(HOR_Stepper.distanceToGo() != 0 || VERT_Stepper.distanceToGo() != 0) {
    HOR_Stepper.run();
    VERT_Stepper.run();
  }
}


void goToDrug(int slot) {
  if (slot < 1 || slot > 24) {
    Serial.println("ERROR: INVALID_SLOT");
    return;
  }

  int row = (slot - 1) / 8 + 1;
  int col = (slot - 1) % 8 + 1;
  long rowPos = rowToPos(row);
  long colPos = columnToPos(col);

  if (rowPos == -1) {
    Serial.println("ERROR: INVALID_POSITION");
    return;
  }
  if (colPos == -1) {
    Serial.println("ERROR: INVALID_POSITION");
    return;
  }

  // Move to the specified Position (Both Horizontal and Vertical)
  while (VERT_Stepper.distanceToGo() != 0 || HOR_Stepper.distanceToGo() != 0) {
    VERT_Stepper.run();
    HOR_Stepper.run();
  }

}

long rowToPos(int row){
  if (row < 1 || row > 3) {
    Serial.println("ERROR: INVALID_ROW");
    return -1;
  }
  long pos = 0;
  if (row == 1){
    pos = ROW_1;
  } else if (row == 2){
    pos = ROW_2;
  } else if (row == 3){
    pos = ROW_3;
  }

  return pos;
  // VERT_Stepper.moveTo(pos);
  // while(VERT_Stepper.distanceToGo() != 0) {
  //   VERT_Stepper.run();
  // }
  

}

long columnToPos(int column){
  if (column < 1 || column > 8) {
    Serial.println("ERROR: INVALID_COLUMN");
    return -1;
  }

  // long stepPerSlot = HOR_maxPosition / 8;
  // targetPosition = stepPerSlot * (column);
  

  long pos = 0;

  switch (column)
  {
  case 1:
    pos = COLUMN_1;
    break;
  case 2:
    pos = COLUMN_2;
    break;
  case 3:
    pos = COLUMN_3;
    break;
  case 4:
    pos = COLUMN_4;
    break;
  case 5:
    pos = COLUMN_5;
    break;
  case 6:
    pos = COLUMN_6;
    break;
  case 7:
    pos = COLUMN_7;
    break;
  case 8:
    pos = COLUMN_8;
    break;
  default:
    break;
  }

  return pos;

}