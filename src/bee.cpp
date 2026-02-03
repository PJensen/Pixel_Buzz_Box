// Pixel Buzz Box - Bee (Position, Physics, Wings, Boost)
#include "game.h"
#include <math.h>

// -------------------- BEE STATE --------------------
BeeState bee = {
  .wx = 0.0f, .wy = 0.0f,
  .vx = 0.0f, .vy = 0.0f,
  .wingPhase = 0.0f,
  .wingSpeed = 0.0f,
  .boostActiveUntilMs = 0,
  .boostCooldownUntilMs = 0,
  .stunnedUntilMs = 0
};

// -------------------- QUERIES --------------------
bool isBoosting(uint32_t nowMs) {
  return (int32_t)(nowMs - bee.boostActiveUntilMs) < 0;
}

bool isBoostOnCooldown(uint32_t nowMs) {
  return (int32_t)(bee.boostCooldownUntilMs - nowMs) > 0;
}

bool isBeeStunned(uint32_t nowMs) {
  return (int32_t)(nowMs - bee.stunnedUntilMs) < 0;
}

void stunBee(uint32_t nowMs) {
  bee.stunnedUntilMs = nowMs + WaspCfg::BEE_STUN_MS;
  bee.vx = 0.0f;
  bee.vy = 0.0f;
}

// -------------------- BOOST CONTROL --------------------
void triggerAutoBoost(uint32_t nowMs) {
  bee.boostActiveUntilMs = nowMs + Physics::BOOST_DURATION_AUTO;
  bee.boostCooldownUntilMs = nowMs + Physics::BOOST_COOLDOWN_AUTO;
}

void triggerManualBoost(uint32_t nowMs) {
  bee.boostActiveUntilMs = nowMs + Physics::BOOST_DURATION_MANUAL;
  bee.boostCooldownUntilMs = nowMs + Physics::BOOST_COOLDOWN_MANUAL;
}

// -------------------- PHYSICS UPDATE --------------------
void updateBeePhysics(float nx, float ny, int rawDx, int rawDy, float dt, bool boosting, uint32_t nowMs) {
  // Skip physics when stunned
  if (isBeeStunned(nowMs)) return;

  // Joystick maps to target position in world space (bounded exploration area)
  const float roamRadius = World::BOUNDARY_COMFORTABLE;
  float targetWX = nx * roamRadius;
  float targetWY = ny * roamRadius;

  // When joystick neutral, target snaps to hive center
  if (rawDx == 0) targetWX = 0.0f;
  if (rawDy == 0) targetWY = 0.0f;

  // Spring constants (higher = more responsive)
  float springK = boosting ? Physics::SPRING_K_BOOST : Physics::SPRING_K_NORMAL;
  float damping = boosting ? Physics::DAMPING_BOOST : Physics::DAMPING_NORMAL;

  // Carry weight penalty: heavier pollen loads feel less agile.
  float load = clampf((float)survival.pollenCount / (float)Pool::MAX_POLLEN_CARRY, 0.0f, 1.0f);
  springK *= (1.0f - Physics::CARRY_WEIGHT_SPRING_PENALTY * load);
  damping *= (1.0f + Physics::CARRY_WEIGHT_DAMPING_PENALTY * load);

  // Spring force: F = k * (target - current) - damping * velocity
  float forceX = springK * (targetWX - bee.wx) - damping * bee.vx;
  float forceY = springK * (targetWY - bee.wy) - damping * bee.vy;

  // Slow movement during magnet cinematic
  float moveScale = isMagnetActive(nowMs) ? Magnet::CINEMATIC_BEE_SPEED : 1.0f;

  bee.vx += forceX * dt * moveScale;
  bee.vy += forceY * dt * moveScale;

  bee.wx += bee.vx * dt * moveScale;
  bee.wy += bee.vy * dt * moveScale;
}

// -------------------- WING ANIMATION --------------------
void updateWingAnimation(float dt) {
  float sp = fabsf(bee.vx) + fabsf(bee.vy);
  float spN = clampf(sp / Physics::WING_SPEED_DIVISOR, 0.0f, 1.0f);
  bee.wingSpeed = spN;
  float hz = Physics::WING_HZ_MIN + Physics::WING_HZ_RANGE * bee.wingSpeed;
  bee.wingPhase += 2.0f * MathConst::PI_F * hz * dt;
  if (bee.wingPhase > Physics::WING_PHASE_WRAP) bee.wingPhase -= Physics::WING_PHASE_WRAP;
}

// -------------------- RESET --------------------
void resetBee() {
  bee.wx = 0.0f;
  bee.wy = 0.0f;
  bee.vx = 0.0f;
  bee.vy = 0.0f;
  bee.wingPhase = 0.0f;
  bee.wingSpeed = 0.0f;
  bee.boostActiveUntilMs = 0;
  bee.boostCooldownUntilMs = 0;
  bee.stunnedUntilMs = 0;
}

void stopBeeMovement() {
  bee.wx = 0.0f;
  bee.wy = 0.0f;
  bee.vx = 0.0f;
  bee.vy = 0.0f;
  bee.wingSpeed = 0.0f;
}

float getBeeSpeed() {
  return sqrtf(bee.vx * bee.vx + bee.vy * bee.vy);
}
