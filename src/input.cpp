// Pixel Buzz Box - Input (Joystick and Buttons)
#include "game.h"

// -------------------- INPUT STATE --------------------
int joyCenterX = 512;
int joyCenterY = 512;
int joyMinY = 1023;
int joyMaxY = 0;
bool btnPrev = false;

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
  const int N = 40;
  delay(30);
  for (int i = 0; i < N; i++) {
    sx += readJoyX();
    sy += readJoyY();
    delay(2);
  }
  joyCenterX = (int)(sx / N);
  joyCenterY = (int)(sy / N);
}

// -------------------- NORMALIZED INPUT --------------------
void readNormalizedJoystick(float &nx, float &ny, int &rawDx, int &rawDy) {
  int rawX = readJoyX();
  int rawY = readJoyY();

  // Update observed extremes for Y (helps asymmetry)
  if (rawY < joyMinY) joyMinY = rawY;
  if (rawY > joyMaxY) joyMaxY = rawY;

  // Deadzone
  const int dead = 35;
  rawDx = applyDeadzone(rawX, joyCenterX, dead);
  rawDy = applyDeadzone(rawY, joyCenterY, dead);

  // Normalize X
  nx = -(float)clampi(rawDx, -512, 512) / 512.0f;

  // Normalize Y with auto-cal asymmetry
  int upSpan   = joyCenterY - joyMinY;
  int downSpan = joyMaxY - joyCenterY;
  if (upSpan < 1) upSpan = 1;
  if (downSpan < 1) downSpan = 1;

  float nyRaw;
  const float DOWN_BOOST = 1.20f;
  if (rawDy >= 0) {
    nyRaw = (float)rawDy / (float)downSpan;
    nyRaw *= DOWN_BOOST;
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
  bool edge = (b && !btnPrev);
  btnPrev = b;
  return edge;
}

void resetButtonState() {
  btnPrev = false;
}
