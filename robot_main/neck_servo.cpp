#include "neck_servo.h"
#include <ESP32Servo.h>

static Servo yawServo;
static Servo pitchServo;

static int currentYaw   = YAW_CENTER;
static int currentPitch = PITCH_CENTER;
static int targetYaw    = YAW_CENTER;
static int targetPitch  = PITCH_CENTER;

static unsigned long lastUpdate   = 0;
static const int UPDATE_INTERVAL  = 15;
static const int SERVO_STEP       = 2;

void neckInit() {
  yawServo.attach(NECK_YAW_PIN);
  pitchServo.attach(NECK_PITCH_PIN);
  yawServo.write(currentYaw);
  pitchServo.write(currentPitch);
  Serial.println(F("Neck: OK"));
}

void neckUpdate() {
  unsigned long now = millis();
  if (now - lastUpdate < UPDATE_INTERVAL) return;
  lastUpdate = now;

  if (currentYaw != targetYaw) {
    int step  = constrain(targetYaw - currentYaw, -SERVO_STEP, SERVO_STEP);
    currentYaw += step;
    yawServo.write(currentYaw);
  }

  if (currentPitch != targetPitch) {
    int step  = constrain(targetPitch - currentPitch, -SERVO_STEP, SERVO_STEP);
    currentPitch += step;
    pitchServo.write(currentPitch);
  }
}

void neckLookRight()  { targetYaw   = YAW_RIGHT;    }
void neckLookLeft()   { targetYaw   = YAW_LEFT;     }
void neckLookCenter() { targetYaw   = YAW_CENTER;   }
void neckLookUp()     { targetPitch = PITCH_UP;     }
void neckLookDown()   { targetPitch = PITCH_DOWN;   }
void neckLookMiddle() { targetPitch = PITCH_CENTER; }

void neckSetYaw(int angle)   { targetYaw   = constrain(angle, 0, 180); }
void neckSetPitch(int angle) { targetPitch = constrain(angle, 0, 180); }

int  neckGetCurrentYaw()   { return currentYaw;   }
int  neckGetCurrentPitch() { return currentPitch; }
int  neckGetTargetYaw()    { return targetYaw;    }
int  neckGetTargetPitch()  { return targetPitch;  }
bool neckIsMoving()        { return (currentYaw != targetYaw) || (currentPitch != targetPitch); }
