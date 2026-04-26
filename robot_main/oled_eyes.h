#pragma once
#include <Arduino.h>

enum AnimationMode {
  MODE_IDLE = 0,
  MODE_HAPPY,
  MODE_SURPRISED,
  MODE_SLEEPY,
  MODE_ANGRY,
  MODE_CONFUSED,
  MODE_FOCUSED,
  MODE_WINK
};

void          eyesInit();
void          eyesUpdate();
void          eyesSetMode(AnimationMode mode);
AnimationMode eyesGetMode();
void          eyesSetGazeOffset(int dx, int dy);
int           eyesGetGazeOffsetX();
int           eyesGetGazeOffsetY();
