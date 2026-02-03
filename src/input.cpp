// Pixel Buzz Box - Input (Joystick and Buttons)
#include "game.h"

// -------------------- INPUT STATE --------------------
InputState input = {
  .centerX = Input::JOY_CENTER_DEFAULT,
  .centerY = Input::JOY_CENTER_DEFAULT,
  .minY = 1023,
  .maxY = 0,
  .btnPrev = false
};

// -------------------- RAW READING --------------------
int readJoyX() { return analogRead(PIN_JOY_VRX); }
int readJoyY() { return analogRead(PIN_JOY_VRY); }
bool joyPressedRaw() { return digitalRead(PIN_JOY_SW) == LOW; }

// -------------------- PROCESSING --------------------
int applyDeadzone(int v, int center, int dz) {
  int d = v - center;
  if (d > -dz && d < dz) return 0;
  return d;
}

void calibrateJoystick() {
  long sx = 0, sy = 0;
  delay(Input::CALIBRATION_DELAY_MS);
  for (int i = 0; i < Input::CALIBRATION_SAMPLES; i++) {
    sx += readJoyX();
    sy += readJoyY();
    delay(Timing::LOOP_DELAY_MS);
  }
  input.centerX = (int)(sx / Input::CALIBRATION_SAMPLES);
  input.centerY = (int)(sy / Input::CALIBRATION_SAMPLES);
}

// -------------------- NORMALIZED INPUT --------------------
void readNormalizedJoystick(float &nx, float &ny, int &rawDx, int &rawDy) {
  int rawX = readJoyX();
  int rawY = readJoyY();

  // Update observed extremes for Y (helps asymmetry)
  if (rawY < input.minY) input.minY = rawY;
  if (rawY > input.maxY) input.maxY = rawY;

  // Deadzone
  rawDx = applyDeadzone(rawX, input.centerX, Input::JOY_DEADZONE);
  rawDy = applyDeadzone(rawY, input.centerY, Input::JOY_DEADZONE);

  // Normalize X
  nx = -(float)clampi(rawDx, -Input::JOY_RANGE, Input::JOY_RANGE) / (float)Input::JOY_RANGE;

  // Normalize Y with auto-cal asymmetry
  int upSpan   = input.centerY - input.minY;
  int downSpan = input.maxY - input.centerY;
  if (upSpan < 1) upSpan = 1;
  if (downSpan < 1) downSpan = 1;

  float nyRaw;
  if (rawDy >= 0) {
    nyRaw = (float)rawDy / (float)downSpan;
    nyRaw *= Input::JOY_DOWN_BOOST;
  } else {
    nyRaw = (float)rawDy / (float)upSpan;
  }
  nyRaw = clampf(nyRaw, -1.0f, 1.0f);
  ny = -nyRaw;

  // Circle->square boost (diagonals)
  float ax = fabsf(nx);
  float ay = fabsf(ny);
  float m  = (ax > ay) ? ax : ay;
  if (m > 0.0001f) {
    nx /= m;
    ny /= m;
    nx = clampf(nx, -1.0f, 1.0f);
    ny = clampf(ny, -1.0f, 1.0f);
  }
}

bool readButtonEdge() {
  bool b = joyPressedRaw();
  bool edge = (b && !input.btnPrev);
  input.btnPrev = b;
  return edge;
}

void resetButtonState() {
  input.btnPrev = false;
}
