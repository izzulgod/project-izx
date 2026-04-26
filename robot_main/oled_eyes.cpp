#include "oled_eyes.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT     64
#define OLED_RESET        -1
#define SCREEN_ADDRESS  0x3C

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct EyeState {
  int x, y;
  int width, height;
  int target_width, target_height;
};

static const int REF_EYE_HEIGHT     = 40;
static const int REF_EYE_WIDTH      = 40;
static const int REF_SPACE_BETWEEN  = 10;
static const int REF_CORNER_RADIUS  = 10;

static EyeState left_eye, right_eye;
static AnimationMode current_mode   = MODE_IDLE;

static unsigned long last_blink        = 0;
static unsigned long last_breath       = 0;
static unsigned long last_idle_change  = 0;
static unsigned long mode_start_time   = 0;

static float   breath_phase   = 0;
static int     idle_pattern   = 0;
static boolean is_blinking    = false;
static int     blink_progress = 0;

static int gazeOffsetX = 0;
static int gazeOffsetY = 0;

static const unsigned long BLINK_INTERVAL_MIN   = 2000;
static const unsigned long BLINK_INTERVAL_MAX   = 5000;
static const unsigned long IDLE_CHANGE_INTERVAL = 8000;

// ─── HELPERS ──────────────────────────────────────────────────────────────────

static int eyeCX(EyeState& eye) {
  return constrain(eye.x + gazeOffsetX, eye.width  / 2, SCREEN_WIDTH  - eye.width  / 2);
}
static int eyeCY(EyeState& eye) {
  return constrain(eye.y + gazeOffsetY, eye.height / 2, SCREEN_HEIGHT - eye.height / 2);
}

static void drawEye(EyeState& eye, bool invert = false) {
  int cx = eyeCX(eye);
  int cy = eyeCY(eye);
  int x  = cx - eye.width  / 2;
  int y  = cy - eye.height / 2;

  if (invert) {
    display.fillRoundRect(x,   y,   eye.width,   eye.height,   REF_CORNER_RADIUS, SSD1306_BLACK);
    display.drawRoundRect(x-1, y-1, eye.width+2, eye.height+2, REF_CORNER_RADIUS, SSD1306_WHITE);
  } else {
    display.fillRoundRect(x, y, eye.width, eye.height, REF_CORNER_RADIUS, SSD1306_WHITE);
  }
}

static void updateDisplay() {
  display.clearDisplay();
  drawEye(left_eye);
  drawEye(right_eye);
  display.display();
}

static void smoothTransition(EyeState& eye, float speed = 0.3) {
  eye.width  += (int)round((eye.target_width  - eye.width)  * speed);
  eye.height += (int)round((eye.target_height - eye.height) * speed);
}

static void initializeEyes() {
  left_eye.x  = SCREEN_WIDTH  / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN / 2;
  left_eye.y  = SCREEN_HEIGHT / 2;
  left_eye.width  = left_eye.target_width  = REF_EYE_WIDTH;
  left_eye.height = left_eye.target_height = REF_EYE_HEIGHT;

  right_eye.x = SCREEN_WIDTH  / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN / 2;
  right_eye.y = SCREEN_HEIGHT / 2;
  right_eye.width  = right_eye.target_width  = REF_EYE_WIDTH;
  right_eye.height = right_eye.target_height = REF_EYE_HEIGHT;
}

// ─── IDLE ─────────────────────────────────────────────────────────────────────

static void updateIdleBreathing() {
  unsigned long now = millis();
  if (now - last_breath < 50) return;
  last_breath = now;

  breath_phase += 0.1f;
  if (breath_phase > 2 * PI) breath_phase = 0;

  float scale = 1.0f + 0.05f * sin(breath_phase);
  left_eye.target_width   = REF_EYE_WIDTH  * scale;
  left_eye.target_height  = REF_EYE_HEIGHT * scale;
  right_eye.target_width  = REF_EYE_WIDTH  * scale;
  right_eye.target_height = REF_EYE_HEIGHT * scale;
}

static void updateAlternatingPulse() {
  unsigned long now = millis();
  if (now - last_breath < 80) return;
  last_breath = now;

  breath_phase += 0.15f;
  if (breath_phase > 4 * PI) breath_phase = 0;

  float ls = sin(breath_phase);
  float rs = sin(breath_phase + PI);
  left_eye.target_width   = REF_EYE_WIDTH  * (1.0f + 0.1f * (ls > 0 ? ls : 0));
  left_eye.target_height  = REF_EYE_HEIGHT * (1.0f + 0.1f * (ls > 0 ? ls : 0));
  right_eye.target_width  = REF_EYE_WIDTH  * (1.0f + 0.1f * (rs > 0 ? rs : 0));
  right_eye.target_height = REF_EYE_HEIGHT * (1.0f + 0.1f * (rs > 0 ? rs : 0));
}

static void updateRandomBlink() {
  unsigned long now = millis();
  if (is_blinking) {
    blink_progress++;
    if (blink_progress < 3) {
      left_eye.target_height  = 3;
      right_eye.target_height = 3;
    } else if (blink_progress < 6) {
      left_eye.target_height  = REF_EYE_HEIGHT;
      right_eye.target_height = REF_EYE_HEIGHT;
    } else {
      is_blinking    = false;
      blink_progress = 0;
      last_blink     = now + random(BLINK_INTERVAL_MIN, BLINK_INTERVAL_MAX);
    }
  } else if (now > last_blink) {
    is_blinking    = true;
    blink_progress = 0;
  }
}

static void updateIdlePattern() {
  unsigned long now = millis();
  if (now - last_idle_change > IDLE_CHANGE_INTERVAL) {
    idle_pattern     = (idle_pattern + 1) % 2;
    last_idle_change = now;
  }
  (idle_pattern == 0) ? updateIdleBreathing() : updateAlternatingPulse();
  updateRandomBlink();
}

// ─── EXPRESSIONS ──────────────────────────────────────────────────────────────

static void animateHappy() {
  left_eye.target_width   = REF_EYE_WIDTH + 8;
  left_eye.target_height  = REF_EYE_HEIGHT - 5;
  right_eye.target_width  = REF_EYE_WIDTH + 8;
  right_eye.target_height = REF_EYE_HEIGHT - 5;

  display.clearDisplay();
  drawEye(left_eye);
  drawEye(right_eye);

  auto drawCurve = [](EyeState& eye) {
    int cx = eyeCX(eye);
    int cy = eyeCY(eye) + eye.height / 2 + 6;
    for (int i = 0; i < eye.width; i++) {
      int dy = (int)(4 * sin(PI * i / (float)eye.width));
      display.drawPixel(cx - eye.width / 2 + i, cy + dy, SSD1306_WHITE);
    }
  };
  drawCurve(left_eye);
  drawCurve(right_eye);
  display.display();
}

static void animateSurprised() {
  left_eye.target_width   = REF_EYE_WIDTH  + 15;
  left_eye.target_height  = REF_EYE_HEIGHT + 15;
  right_eye.target_width  = REF_EYE_WIDTH  + 15;
  right_eye.target_height = REF_EYE_HEIGHT + 15;
}

static void animateSleepy() {
  left_eye.target_width   = REF_EYE_WIDTH;
  left_eye.target_height  = REF_EYE_HEIGHT / 3;
  right_eye.target_width  = REF_EYE_WIDTH;
  right_eye.target_height = REF_EYE_HEIGHT / 3;
}

static void animateAngry() {
  left_eye.target_width   = REF_EYE_WIDTH  - 5;
  left_eye.target_height  = REF_EYE_HEIGHT - 10;
  right_eye.target_width  = REF_EYE_WIDTH  - 5;
  right_eye.target_height = REF_EYE_HEIGHT - 10;

  display.clearDisplay();
  drawEye(left_eye);
  drawEye(right_eye);

  int brow_y = eyeCY(left_eye) - left_eye.height / 2 - 8;
  display.drawLine(eyeCX(left_eye)  - left_eye.width  / 2, brow_y + 3,
                   eyeCX(left_eye)  + left_eye.width  / 2, brow_y,     SSD1306_WHITE);
  display.drawLine(eyeCX(right_eye) - right_eye.width / 2, brow_y,
                   eyeCX(right_eye) + right_eye.width / 2, brow_y + 3, SSD1306_WHITE);
  display.display();
}

static void animateConfused() {
  left_eye.target_width   = REF_EYE_WIDTH  + 5;
  left_eye.target_height  = REF_EYE_HEIGHT + 5;
  right_eye.target_width  = REF_EYE_WIDTH  - 3;
  right_eye.target_height = REF_EYE_HEIGHT - 3;

  display.clearDisplay();
  drawEye(left_eye);
  drawEye(right_eye);
  display.drawCircle(SCREEN_WIDTH - 15, 15, 3, SSD1306_WHITE);
  display.drawPixel(SCREEN_WIDTH - 15, 22, SSD1306_WHITE);
  display.display();
}

static void animateFocused() {
  left_eye.target_width   = REF_EYE_WIDTH  - 8;
  left_eye.target_height  = REF_EYE_HEIGHT - 5;
  right_eye.target_width  = REF_EYE_WIDTH  - 8;
  right_eye.target_height = REF_EYE_HEIGHT - 5;
}

static void animateWink() {
  left_eye.target_width   = REF_EYE_WIDTH;
  left_eye.target_height  = 2;
  right_eye.target_width  = REF_EYE_WIDTH;
  right_eye.target_height = REF_EYE_HEIGHT;
}

// ─── PUBLIC API ───────────────────────────────────────────────────────────────

void eyesInit() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 init failed"));
    while (1);
  }
  display.clearDisplay();
  display.display();
  initializeEyes();
  current_mode = MODE_IDLE;
  Serial.println(F("Eyes: OK"));
}

void eyesSetMode(AnimationMode mode) {
  current_mode    = mode;
  mode_start_time = millis();
  if (mode == MODE_IDLE) initializeEyes();
}

AnimationMode eyesGetMode() { return current_mode; }

void eyesSetGazeOffset(int dx, int dy) {
  gazeOffsetX = dx;
  gazeOffsetY = dy;
}

int eyesGetGazeOffsetX() { return gazeOffsetX; }
int eyesGetGazeOffsetY() { return gazeOffsetY; }

void eyesUpdate() {
  unsigned long now = millis();

  switch (current_mode) {
    case MODE_IDLE:
      updateIdlePattern();
      break;

    case MODE_HAPPY:
      animateHappy();
      if (now - mode_start_time > 3000) eyesSetMode(MODE_IDLE);
      return;

    case MODE_SURPRISED:
      animateSurprised();
      if (now - mode_start_time > 2000) eyesSetMode(MODE_IDLE);
      break;

    case MODE_SLEEPY:
      animateSleepy();
      if (now - mode_start_time > 5000) eyesSetMode(MODE_IDLE);
      break;

    case MODE_ANGRY:
      animateAngry();
      if (now - mode_start_time > 3000) eyesSetMode(MODE_IDLE);
      return;

    case MODE_CONFUSED:
      animateConfused();
      if (now - mode_start_time > 4000) eyesSetMode(MODE_IDLE);
      return;

    case MODE_FOCUSED:
      animateFocused();
      if (now - mode_start_time > 3000) eyesSetMode(MODE_IDLE);
      break;

    case MODE_WINK:
      animateWink();
      if (now - mode_start_time > 1000) eyesSetMode(MODE_IDLE);
      break;
  }

  smoothTransition(left_eye);
  smoothTransition(right_eye);
  updateDisplay();
}
