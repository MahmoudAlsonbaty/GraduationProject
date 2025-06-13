
#include <Arduino.h>
// Limit switch pins Horizontal (X-axis)
#define HOR_LIMIT_MIN_PIN  14
#define HOR_LIMIT_MAX_PIN  27
#define HOR_LIMIT_PULLUP 32

// Limit switch pins Vertical (Y-axis)
#define VERT_LIMIT_MIN_PIN 26
#define VERT_LIMIT_MAX_PIN 25
#define VERT_LIMIT_PULLUP 21
void setup() {
pinMode(HOR_LIMIT_MIN_PIN, INPUT_PULLDOWN);
  pinMode(HOR_LIMIT_MAX_PIN, INPUT_PULLDOWN);
  pinMode(VERT_LIMIT_MIN_PIN, INPUT_PULLDOWN);
  pinMode(VERT_LIMIT_MAX_PIN, INPUT_PULLDOWN);

  pinMode(HOR_LIMIT_PULLUP, OUTPUT);
  pinMode(VERT_LIMIT_PULLUP, OUTPUT);

  digitalWrite(HOR_LIMIT_PULLUP, HIGH); // Enable pull-up for horizontal limit switches
  digitalWrite(VERT_LIMIT_PULLUP, HIGH); // Enable pull-up for vertical limit switches
  Serial.begin(9600);
}

void loop() {
  String status = "";

  if (digitalRead(HOR_LIMIT_MIN_PIN)) {
    status += "H Min ";
  }
  if (digitalRead(HOR_LIMIT_MAX_PIN)) {
    status += "H Max ";
  }
  if (digitalRead(VERT_LIMIT_MIN_PIN)) {
    status += "V Min ";
  }
  if (digitalRead(VERT_LIMIT_MAX_PIN)) {
    status += "V Max ";
  }

  if (status.length() > 0) {
    Serial.println(status);
  }
}