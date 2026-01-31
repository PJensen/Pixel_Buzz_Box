// Pixel Buzz Box - VFX (Trails, Popups, Camera, Pulse)
#include "game.h"
#include <math.h>

// -------------------- TRAIL STATE --------------------
TrailParticle trail[TRAIL_MAX];
int trailNextIdx = 0;

// -------------------- SCORE POPUP STATE --------------------
ScorePopup scorePopups[SCORE_POPUP_N];

// -------------------- CAMERA STATE --------------------
float cameraZoom = 1.0f;
float cameraShakeX = 0.0f;
float cameraShakeY = 0.0f;
uint32_t cameraShakeUntilMs = 0;
uint32_t cameraShakeDurationMs = 0;
float cameraShakeMagnitude = 0.0f;

// -------------------- HIVE PULSE STATE --------------------
uint32_t hivePulseUntilMs = 0;

// -------------------- CAMERA FUNCTIONS --------------------
int beeScreenCX() { return tft.width() / 2 + (int)cameraShakeX; }
int beeScreenCY() { return (tft.height() + HUD_H) / 2 + (int)cameraShakeY; }

void triggerCameraShake(uint32_t nowMs, float magnitude, uint32_t durationMs) {
  cameraShakeUntilMs = nowMs + durationMs;
  cameraShakeDurationMs = durationMs;
  cameraShakeMagnitude = magnitude;
}

void updateCamera(float dt, bool boosting, uint32_t nowMs) {
  // Zoom
  float targetZoom = boosting ? CAMERA_ZOOM_BOOST : CAMERA_ZOOM_NORMAL;
  float zoomLerp = clampf(CAMERA_ZOOM_LERP_SPEED * dt, 0.0f, 1.0f);
  cameraZoom += (targetZoom - cameraZoom) * zoomLerp;

  // Shake
  if ((int32_t)(nowMs - cameraShakeUntilMs) < 0 && cameraShakeDurationMs > 0) {
    float t = (float)(cameraShakeUntilMs - nowMs) / (float)cameraShakeDurationMs;
    t = clampf(t, 0.0f, 1.0f);
    float amp = cameraShakeMagnitude * t * t;
    float phase = (float)nowMs * CAMERA_SHAKE_PHASE_MULT;
    cameraShakeX = sinf(phase * CAMERA_SHAKE_FREQ_X) * amp;
    cameraShakeY = cosf(phase * CAMERA_SHAKE_FREQ_Y) * amp;
  } else {
    cameraShakeX = 0.0f;
    cameraShakeY = 0.0f;
  }
}

void resetCamera() {
  cameraZoom = 1.0f;
  cameraShakeX = 0.0f;
  cameraShakeY = 0.0f;
  cameraShakeUntilMs = 0;
  cameraShakeDurationMs = 0;
  cameraShakeMagnitude = 0.0f;
}

// -------------------- COORDINATE TRANSFORMS --------------------
void worldToScreen(int32_t wx, int32_t wy, int &sx, int &sy) {
  float dx = (float)wx - beeWX;
  float dy = (float)wy - beeWY;
  sx = beeScreenCX() + (int)(dx * cameraZoom);
  sy = beeScreenCY() + (int)(dy * cameraZoom);
}

void worldToScreenF(float wx, float wy, int &sx, int &sy) {
  float dx = wx - beeWX;
  float dy = wy - beeWY;
  sx = beeScreenCX() + (int)(dx * cameraZoom);
  sy = beeScreenCY() + (int)(dy * cameraZoom);
}

uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt) {
  return hash32((uint32_t)cx * 73856093u ^ (uint32_t)cy * 19349663u ^ salt);
}

// -------------------- TRAIL FUNCTIONS --------------------
void spawnTrailParticle(float wx, float wy, float speedN, uint32_t nowMs) {
  trail[trailNextIdx].wx = wx;
  trail[trailNextIdx].wy = wy;
  trail[trailNextIdx].bornMs = nowMs;
  trail[trailNextIdx].alive = 1;
  trail[trailNextIdx].variant = (uint8_t)(xrnd() % 3);
  trail[trailNextIdx].speedN = clampf(speedN, 0.0f, 1.0f);
  trailNextIdx = (trailNextIdx + 1) % TRAIL_MAX;
}

void updateTrailParticles(uint32_t nowMs) {
  for (int i = 0; i < TRAIL_MAX; i++) {
    if (!trail[i].alive) continue;
    if ((uint32_t)(nowMs - trail[i].bornMs) > TRAIL_LIFE_MS) {
      trail[i].alive = 0;
    }
  }
}

bool anyTrailAlive() {
  for (int i = 0; i < TRAIL_MAX; i++) {
    if (trail[i].alive) return true;
  }
  return false;
}

// -------------------- SCORE POPUP FUNCTIONS --------------------
void spawnScorePopup(uint32_t nowMs, uint8_t value, int sx, int sy) {
  int freeIdx = -1;
  uint32_t oldest = 0xFFFFFFFFu;
  int oldestIdx = 0;

  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) { freeIdx = i; break; }
    if (scorePopups[i].bornMs < oldest) { oldest = scorePopups[i].bornMs; oldestIdx = i; }
  }

  int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
  scorePopups[idx].alive = 1;
  scorePopups[idx].bornMs = nowMs;
  scorePopups[idx].value = value;
  scorePopups[idx].baseSX = (int16_t)sx;
  scorePopups[idx].baseSY = (int16_t)sy;
  scorePopups[idx].driftX = (int8_t)irand(SCORE_POPUP_DRIFT_MIN, SCORE_POPUP_DRIFT_MAX);
}

void updateScorePopups(uint32_t nowMs) {
  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) continue;
    if ((uint32_t)(nowMs - scorePopups[i].bornMs) > SCORE_POPUP_LIFE_MS) {
      scorePopups[i].alive = 0;
    }
  }
}

bool anyScorePopupAlive() {
  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (scorePopups[i].alive) return true;
  }
  return false;
}

// -------------------- HIVE PULSE --------------------
void triggerHivePulse(uint32_t nowMs) {
  hivePulseUntilMs = nowMs + HIVE_PULSE_MS;
}

// -------------------- RESET --------------------
void resetVFX() {
  for (int i = 0; i < TRAIL_MAX; i++) trail[i].alive = 0;
  for (int i = 0; i < SCORE_POPUP_N; i++) scorePopups[i].alive = 0;
  trailNextIdx = 0;
  hivePulseUntilMs = 0;
  resetCamera();
}
