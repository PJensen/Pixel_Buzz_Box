// Pixel Buzz Box - Bee (Position, Physics, Wings, Boost)
#include "game.h"
#include <math.h>

// -------------------- BEE STATE --------------------
float beeWX = 0.0f, beeWY = 0.0f;
float beeVX = 0.0f, beeVY = 0.0f;
float wingPhase = 0.0f;
float wingSpeed = 0.0f;

// -------------------- BOOST STATE --------------------
uint32_t boostActiveUntilMs = 0;
uint32_t boostCooldownUntilMs = 0;

// -------------------- QUERIES --------------------
bool isBoosting(uint32_t nowMs) {
  return (int32_t)(nowMs - boostActiveUntilMs) < 0;
}

bool isBoostOnCooldown(uint32_t nowMs) {
  return (int32_t)(boostCooldownUntilMs - nowMs) > 0;
}

// -------------------- BOOST CONTROL --------------------
void triggerAutoBoost(uint32_t nowMs) {
  boostActiveUntilMs = nowMs + BOOST_DURATION_AUTO;
  boostCooldownUntilMs = nowMs + BOOST_COOLDOWN_AUTO;
}

void triggerManualBoost(uint32_t nowMs) {
  boostActiveUntilMs = nowMs + BOOST_DURATION_MANUAL;
  boostCooldownUntilMs = nowMs + BOOST_COOLDOWN_MANUAL;
}

// -------------------- PHYSICS UPDATE --------------------
void updateBeePhysics(float nx, float ny, int rawDx, int rawDy, float dt, bool boosting) {
  // Joystick maps to target position in world space (bounded exploration area)
  const float roamRadius = BOUNDARY_COMFORTABLE;
  float targetWX = nx * roamRadius;
  float targetWY = ny * roamRadius;

  // When joystick neutral, target snaps to hive center
  if (rawDx == 0) targetWX = 0.0f;
  if (rawDy == 0) targetWY = 0.0f;

  // Spring constants (higher = more responsive)
  const float springK = boosting ? SPRING_K_BOOST : SPRING_K_NORMAL;
  const float damping = boosting ? DAMPING_BOOST : DAMPING_NORMAL;

  // Spring force: F = k * (target - current) - damping * velocity
  float forceX = springK * (targetWX - beeWX) - damping * beeVX;
  float forceY = springK * (targetWY - beeWY) - damping * beeVY;

  beeVX += forceX * dt;
  beeVY += forceY * dt;

  beeWX += beeVX * dt;
  beeWY += beeVY * dt;
}

// -------------------- WING ANIMATION --------------------
void updateWingAnimation(float dt) {
  float sp = fabsf(beeVX) + fabsf(beeVY);
  float spN = clampf(sp / WING_SPEED_DIVISOR, 0.0f, 1.0f);
  wingSpeed = spN;
  float hz = 3.0f + 14.0f * wingSpeed;
  wingPhase += 2.0f * 3.1415926f * hz * dt;
  if (wingPhase > 1000.0f) wingPhase -= 1000.0f;
}

// -------------------- RESET --------------------
void resetBee() {
  beeWX = 0.0f;
  beeWY = 0.0f;
  beeVX = 0.0f;
  beeVY = 0.0f;
  wingPhase = 0.0f;
  wingSpeed = 0.0f;
  boostActiveUntilMs = 0;
  boostCooldownUntilMs = 0;
}

void stopBeeMovement() {
  beeWX = 0.0f;
  beeWY = 0.0f;
  beeVX = 0.0f;
  beeVY = 0.0f;
  wingSpeed = 0.0f;
}

float getBeeSpeed() {
  return sqrtf(beeVX * beeVX + beeVY * beeVY);
}
