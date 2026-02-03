// Pixel Buzz Box - VFX (Trails, Popups, Camera, Pulse)
#include "game.h"
#include <math.h>

// -------------------- TRAIL STATE --------------------
TrailParticle trail[Pool::TRAIL_MAX];
int trailNextIdx = 0;

// -------------------- SCORE POPUP STATE --------------------
ScorePopup scorePopups[Pool::SCORE_POPUP_N];

// -------------------- CAMERA STATE --------------------
CameraState camera = {
  .zoom = 1.0f,
  .shakeX = 0.0f, .shakeY = 0.0f,
  .shakeUntilMs = 0,
  .shakeDurationMs = 0,
  .shakeMagnitude = 0.0f
};

// -------------------- HIVE PULSE STATE --------------------
uint32_t hivePulseUntilMs = 0;

// -------------------- CAMERA FUNCTIONS --------------------
// Bee's screen position (center of screen, no shake - bee stays stable)
int beeScreenCX() { return tft.width() / 2; }
int beeScreenCY() { return (tft.height() + Display::HUD_H) / 2; }

void triggerCameraShake(uint32_t nowMs, float magnitude, uint32_t durationMs) {
  camera.shakeUntilMs = nowMs + durationMs;
  camera.shakeDurationMs = durationMs;
  camera.shakeMagnitude = magnitude;
}

void updateCamera(float dt, bool boosting, uint32_t nowMs) {
  // Zoom
  float targetZoom = boosting ? Camera::ZOOM_BOOST : Camera::ZOOM_NORMAL;
  float zoomLerp = clampf(Camera::ZOOM_LERP_SPEED * dt, 0.0f, 1.0f);
  camera.zoom += (targetZoom - camera.zoom) * zoomLerp;

  // Shake
  if ((int32_t)(nowMs - camera.shakeUntilMs) < 0 && camera.shakeDurationMs > 0) {
    float t = (float)(camera.shakeUntilMs - nowMs) / (float)camera.shakeDurationMs;
    t = clampf(t, 0.0f, 1.0f);
    float amp = camera.shakeMagnitude * t * t;
    float phase = (float)nowMs * Camera::SHAKE_PHASE_MULT;
    camera.shakeX = sinf(phase * Camera::SHAKE_FREQ_X) * amp;
    camera.shakeY = cosf(phase * Camera::SHAKE_FREQ_Y) * amp;
  } else {
    camera.shakeX = 0.0f;
    camera.shakeY = 0.0f;
  }
}

void resetCamera() {
  camera.zoom = 1.0f;
  camera.shakeX = 0.0f;
  camera.shakeY = 0.0f;
  camera.shakeUntilMs = 0;
  camera.shakeDurationMs = 0;
  camera.shakeMagnitude = 0.0f;
}

// -------------------- COORDINATE TRANSFORMS --------------------
// World to screen includes shake offset (world shakes around stable bee)
void worldToScreen(int32_t wx, int32_t wy, int &sx, int &sy) {
  float dx = (float)wx - bee.wx;
  float dy = (float)wy - bee.wy;
  sx = beeScreenCX() + (int)(dx * camera.zoom + camera.shakeX);
  sy = beeScreenCY() + (int)(dy * camera.zoom + camera.shakeY);
}

void worldToScreenF(float wx, float wy, int &sx, int &sy) {
  float dx = wx - bee.wx;
  float dy = wy - bee.wy;
  sx = beeScreenCX() + (int)(dx * camera.zoom + camera.shakeX);
  sy = beeScreenCY() + (int)(dy * camera.zoom + camera.shakeY);
}

uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt) {
  return hash32((uint32_t)cx * 73856093u ^ (uint32_t)cy * 19349663u ^ salt);
}

// -------------------- TRAIL FUNCTIONS --------------------
void spawnTrailParticle(float wx, float wy, float speedN, uint32_t nowMs, int8_t forceVariant) {
  trail[trailNextIdx].wx = wx;
  trail[trailNextIdx].wy = wy;
  trail[trailNextIdx].bornMs = nowMs;
  trail[trailNextIdx].alive = 1;
  // Variant 3 = gold particles for rare flowers
  trail[trailNextIdx].variant = (forceVariant >= 0) ? (uint8_t)forceVariant : (uint8_t)(xrnd() % 3);
  trail[trailNextIdx].speedN = clampf(speedN, 0.0f, 1.0f);
  trailNextIdx = (trailNextIdx + 1) % Pool::TRAIL_MAX;
}

void updateTrailParticles(uint32_t nowMs) {
  for (int i = 0; i < Pool::TRAIL_MAX; i++) {
    if (!trail[i].alive) continue;
    if ((uint32_t)(nowMs - trail[i].bornMs) > Vfx::TRAIL_LIFE_MS) {
      trail[i].alive = 0;
    }
  }
}

bool anyTrailAlive() {
  for (int i = 0; i < Pool::TRAIL_MAX; i++) {
    if (trail[i].alive) return true;
  }
  return false;
}

// -------------------- SCORE POPUP FUNCTIONS --------------------
void spawnScorePopup(uint32_t nowMs, uint8_t pollen, float mult, int screenX, int screenY) {
  int freeIdx = -1;
  uint32_t oldest = 0xFFFFFFFFu;
  int oldestIdx = 0;

  for (int i = 0; i < Pool::SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) { freeIdx = i; break; }
    if (scorePopups[i].bornMs < oldest) { oldest = scorePopups[i].bornMs; oldestIdx = i; }
  }

  int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
  scorePopups[idx].alive = 1;
  scorePopups[idx].bornMs = nowMs;
  scorePopups[idx].value = pollen;
  scorePopups[idx].multiplier = mult;
  scorePopups[idx].baseSX = (int16_t)screenX;
  scorePopups[idx].baseSY = (int16_t)screenY;
  scorePopups[idx].driftX = (int8_t)irand(PopupCfg::DRIFT_MIN, PopupCfg::DRIFT_MAX);
}

void updateScorePopups(uint32_t nowMs) {
  for (int i = 0; i < Pool::SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) continue;
    if ((uint32_t)(nowMs - scorePopups[i].bornMs) > Timing::SCORE_POPUP_LIFE_MS) {
      scorePopups[i].alive = 0;
    }
  }
}

bool anyScorePopupAlive() {
  for (int i = 0; i < Pool::SCORE_POPUP_N; i++) {
    if (scorePopups[i].alive) return true;
  }
  return false;
}

// -------------------- HIVE PULSE --------------------
void triggerHivePulse(uint32_t nowMs) {
  hivePulseUntilMs = nowMs + Timing::HIVE_PULSE_MS;
}

// -------------------- RESET --------------------
void resetVFX() {
  for (int i = 0; i < Pool::TRAIL_MAX; i++) trail[i].alive = 0;
  for (int i = 0; i < Pool::SCORE_POPUP_N; i++) scorePopups[i].alive = 0;
  trailNextIdx = 0;
  hivePulseUntilMs = 0;
  resetCamera();
}
