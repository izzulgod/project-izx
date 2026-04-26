#pragma once
#include <Arduino.h>

#define NECK_YAW_PIN   13
#define NECK_PITCH_PIN 12

#define YAW_CENTER    90
#define YAW_RIGHT     45
#define YAW_LEFT     135

#define PITCH_CENTER  90
#define PITCH_UP      55
#define PITCH_DOWN   120

void neckInit();
void neckUpdate();

void neckLookRight();
void neckLookLeft();
void neckLookCenter();
void neckLookUp();
void neckLookDown();
void neckLookMiddle();

void neckSetYaw(int angle);
void neckSetPitch(int angle);

int  neckGetCurrentYaw();
int  neckGetCurrentPitch();
int  neckGetTargetYaw();
int  neckGetTargetPitch();
bool neckIsMoving();
