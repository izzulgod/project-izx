#include "command_handler.h"
#include "oled_eyes.h"
#include "neck_servo.h"

// ─── SYNC ─────────────────────────────────────────────────────────────────────

static void syncEyesWithYaw(int angle) {
  int dx = map(angle, YAW_RIGHT, YAW_LEFT, 12, -12);
  eyesSetGazeOffset(dx, eyesGetGazeOffsetY());
}

static void syncEyesWithPitch(int angle) {
  int dy = map(angle, PITCH_UP, PITCH_DOWN, -10, 8);
  eyesSetGazeOffset(eyesGetGazeOffsetX(), dy);
}

static void syncOnNeckCommand(const String& dir) {
  if (dir == "RIGHT") {
    syncEyesWithYaw(YAW_RIGHT);
    eyesSetMode(MODE_FOCUSED);
  } else if (dir == "LEFT") {
    syncEyesWithYaw(YAW_LEFT);
    eyesSetMode(MODE_FOCUSED);
  } else if (dir == "CENTER") {
    syncEyesWithYaw(YAW_CENTER);
  } else if (dir == "UP") {
    syncEyesWithPitch(PITCH_UP);
    eyesSetMode(MODE_SURPRISED);
  } else if (dir == "DOWN") {
    syncEyesWithPitch(PITCH_DOWN);
    eyesSetMode(MODE_SLEEPY);
  } else if (dir == "MIDDLE") {
    syncEyesWithPitch(PITCH_CENTER);
  }
}

// ─── TEST HELPERS ─────────────────────────────────────────────────────────────

static void waitAndUpdate(unsigned long ms) {
  unsigned long end = millis() + ms;
  while (millis() < end) {
    neckUpdate();
    eyesUpdate();
    delay(20);
  }
}

static void testNeck() {
  Serial.println(F("[TEST] Neck sequence start"));
  const struct { void (*fn)(); const char* label; } steps[] = {
    { neckLookRight,  "RIGHT"  },
    { neckLookCenter, "CENTER" },
    { neckLookLeft,   "LEFT"   },
    { neckLookCenter, "CENTER" },
    { neckLookUp,     "UP"     },
    { neckLookMiddle, "MIDDLE" },
    { neckLookDown,   "DOWN"   },
    { neckLookMiddle, "MIDDLE" },
    { neckLookCenter, "CENTER" },
  };
  for (auto& s : steps) {
    Serial.print(F("  -> ")); Serial.println(s.label);
    s.fn();
    waitAndUpdate(1200);
  }
  Serial.println(F("[TEST] Neck done"));
}

static void testEyes() {
  Serial.println(F("[TEST] Eyes sequence start"));
  const struct { AnimationMode mode; const char* label; unsigned long dur; } steps[] = {
    { MODE_HAPPY,     "HAPPY",     3200 },
    { MODE_SURPRISED, "SURPRISED", 2200 },
    { MODE_SLEEPY,    "SLEEPY",    5200 },
    { MODE_ANGRY,     "ANGRY",     3200 },
    { MODE_CONFUSED,  "CONFUSED",  4200 },
    { MODE_FOCUSED,   "FOCUSED",   3200 },
    { MODE_WINK,      "WINK",      1200 },
  };
  for (auto& s : steps) {
    Serial.print(F("  -> ")); Serial.println(s.label);
    eyesSetMode(s.mode);
    waitAndUpdate(s.dur);
  }
  eyesSetMode(MODE_IDLE);
  Serial.println(F("[TEST] Eyes done"));
}

static void testSync() {
  Serial.println(F("[TEST] Sync sequence start"));
  
  const struct { void (*neckFn)(); const char* dir; } steps[] = {
    { neckLookRight,  "RIGHT"  },
    { neckLookCenter, "CENTER" },
    { neckLookLeft,   "LEFT"   },
    { neckLookCenter, "CENTER" },
    { neckLookUp,     "UP"     },
    { neckLookMiddle, "MIDDLE" },
    { neckLookDown,   "DOWN"   },
    { neckLookMiddle, "MIDDLE" },
    { neckLookCenter, "CENTER" },
  };
  for (auto& s : steps) {
    Serial.print(F("  -> ")); Serial.println(s.dir);
    s.neckFn();
    syncOnNeckCommand(s.dir);
    waitAndUpdate(1500);
  }
  eyesSetGazeOffset(0, 0);
  eyesSetMode(MODE_IDLE);
  Serial.println(F("[TEST] Sync done"));
}

// ─── STATUS / HELP ────────────────────────────────────────────────────────────

static void printStatus() {
  Serial.println(F("=== STATUS ==="));
  Serial.print(F("  Eye mode : ")); Serial.println((int)eyesGetMode());
  Serial.print(F("  Gaze X/Y : ")); Serial.print(eyesGetGazeOffsetX());
  Serial.print(F(" / "));          Serial.println(eyesGetGazeOffsetY());
  Serial.print(F("  Yaw      : ")); Serial.print(neckGetCurrentYaw());
  Serial.print(F(" -> "));         Serial.println(neckGetTargetYaw());
  Serial.print(F("  Pitch    : ")); Serial.print(neckGetCurrentPitch());
  Serial.print(F(" -> "));         Serial.println(neckGetTargetPitch());
  Serial.print(F("  Moving   : ")); Serial.println(neckIsMoving() ? "YES" : "NO");
}

static void printHelp() {
  Serial.println(F("=== ROBOT COMMANDS ==="));
  Serial.println(F("  NECK_RIGHT | NECK_LEFT | NECK_CENTER"));
  Serial.println(F("  NECK_UP    | NECK_DOWN | NECK_MIDDLE"));
  Serial.println(F("  NECK_YAW:<0-180> | NECK_PITCH:<0-180>"));
  Serial.println(F("  EXPR_HAPPY | EXPR_SURPRISED | EXPR_SLEEPY"));
  Serial.println(F("  EXPR_ANGRY | EXPR_CONFUSED  | EXPR_FOCUSED"));
  Serial.println(F("  EXPR_WINK  | EXPR_IDLE"));
  Serial.println(F("  TEST_NECK  | TEST_EYES | TEST_SYNC"));
  Serial.println(F("  STATUS     | HELP"));
}

// ─── ROUTING ──────────────────────────────────────────────────────────────────

static void handleNeck(const String& sub) {
  if      (sub == "RIGHT")           { neckLookRight();         syncOnNeckCommand("RIGHT");  }
  else if (sub == "LEFT")            { neckLookLeft();          syncOnNeckCommand("LEFT");   }
  else if (sub == "CENTER")          { neckLookCenter();        syncOnNeckCommand("CENTER"); }
  else if (sub == "UP")              { neckLookUp();            syncOnNeckCommand("UP");     }
  else if (sub == "DOWN")            { neckLookDown();          syncOnNeckCommand("DOWN");   }
  else if (sub == "MIDDLE")          { neckLookMiddle();        syncOnNeckCommand("MIDDLE"); }
  else if (sub.startsWith("YAW:"))   {
    int a = sub.substring(4).toInt();
    neckSetYaw(a);
    syncEyesWithYaw(a);
  }
  else if (sub.startsWith("PITCH:")) {
    int a = sub.substring(6).toInt();
    neckSetPitch(a);
    syncEyesWithPitch(a);
  }
  else { Serial.print(F("Unknown neck cmd: ")); Serial.println(sub); return; }
  Serial.print(F("OK NECK_")); Serial.println(sub);
}

static void handleExpr(const String& expr) {
  if      (expr == "HAPPY")     eyesSetMode(MODE_HAPPY);
  else if (expr == "SURPRISED") eyesSetMode(MODE_SURPRISED);
  else if (expr == "SLEEPY")    eyesSetMode(MODE_SLEEPY);
  else if (expr == "ANGRY")     eyesSetMode(MODE_ANGRY);
  else if (expr == "CONFUSED")  eyesSetMode(MODE_CONFUSED);
  else if (expr == "FOCUSED")   eyesSetMode(MODE_FOCUSED);
  else if (expr == "WINK")      eyesSetMode(MODE_WINK);
  else if (expr == "IDLE")      eyesSetMode(MODE_IDLE);
  else { Serial.print(F("Unknown expr: ")); Serial.println(expr); return; }
  Serial.print(F("OK EXPR_")); Serial.println(expr);
}

// ─── PUBLIC API ───────────────────────────────────────────────────────────────

void commandHandlerInit() {
  printHelp();
  Serial.println(F("Ready."));
}

void commandHandlerUpdate() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.length() == 0) return;

  if      (cmd.startsWith("NECK_")) handleNeck(cmd.substring(5));
  else if (cmd.startsWith("EXPR_")) handleExpr(cmd.substring(5));
  else if (cmd == "TEST_NECK")      testNeck();
  else if (cmd == "TEST_EYES")      testEyes();
  else if (cmd == "TEST_SYNC")      testSync();
  else if (cmd == "STATUS")         printStatus();
  else if (cmd == "HELP")           printHelp();
  else { Serial.print(F("Unknown: ")); Serial.println(cmd); }
}
