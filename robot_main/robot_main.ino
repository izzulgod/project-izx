#include "oled_eyes.h"
#include "neck_servo.h"
#include "command_handler.h"

void setup() {
  Serial.begin(115200);
  eyesInit();
  neckInit();
  commandHandlerInit();
}

void loop() {
  commandHandlerUpdate();
  neckUpdate();
  eyesUpdate();
}
