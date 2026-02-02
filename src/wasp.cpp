// Pixel Buzz Box - Wasp (Predator AI, Physics, Collision)
#include "game.h"
#include <math.h>

// -------------------- WASP STATE --------------------
Wasp wasps[WASP_N];
uint32_t beeInvulnUntilMs = 0;

// -------------------- HELPERS --------------------
static float waspDistToBee(const Wasp &w) {
  float dx = w.wx - beeWX;
  float dy = w.wy - beeWY;
  return sqrtf(dx * dx + dy * dy);
}

static void pickPatrolTarget(Wasp &w, uint32_t nowMs) {
  // Pick random point within WASP_PATROL_RADIUS of current position
  float angle = (float)(xrnd() % 360) * 0.0174532925f;
  float dist = (float)(WASP_PATROL_RADIUS / 2 + (int)(xrnd() % (WASP_PATROL_RADIUS / 2)));
  w.targetWX = w.wx + cosf(angle) * dist;
  w.targetWY = w.wy + sinf(angle) * dist;
  w.lastStateChangeMs = nowMs;
}

// -------------------- SPAWNING --------------------
static void spawnWasp(int idx, uint32_t nowMs) {
  Wasp &w = wasps[idx];

  // Spawn at random angle, safe distance from bee
  float angle = (float)(xrnd() % 360) * 0.0174532925f;
  int dist = WASP_SPAWN_DIST_MIN + (int)(xrnd() % (WASP_SPAWN_DIST_MAX - WASP_SPAWN_DIST_MIN));

  w.wx = beeWX + cosf(angle) * (float)dist;
  w.wy = beeWY + sinf(angle) * (float)dist;
  w.vx = 0.0f;
  w.vy = 0.0f;
  w.alive = 1;
  w.state = WASP_PATROL;
  w.wingPhase = (float)(xrnd() % 100) * 0.01f * 6.28f;  // Random starting phase
  w.stunnedUntilMs = 0;
  w.lastStateChangeMs = nowMs;

  pickPatrolTarget(w, nowMs);
}

void checkWaspSpawning(uint32_t nowMs) {
  // Score thresholds for each wasp slot
  const int thresholds[WASP_N] = { WASP_SPAWN_SCORE_1, WASP_SPAWN_SCORE_2, WASP_SPAWN_SCORE_3 };

  for (int i = 0; i < WASP_N; i++) {
    // Check if this slot should have a wasp based on score
    if (score < thresholds[i]) continue;

    // If wasp is alive, nothing to do
    if (wasps[i].alive) continue;

    // If wasp was never spawned (deathMs == 0), spawn immediately
    // Otherwise, wait for respawn delay
    if (wasps[i].deathMs == 0) {
      spawnWasp(i, nowMs);
    } else if ((nowMs - wasps[i].deathMs) >= WASP_RESPAWN_DELAY_MS) {
      spawnWasp(i, nowMs);
      wasps[i].deathMs = 0;  // Reset death time
    }
  }
}

// -------------------- AI STATE MACHINE --------------------
static void updateWaspAI(Wasp &w, uint32_t nowMs) {
  if (!w.alive) return;

  float distToBee = waspDistToBee(w);

  switch (w.state) {
    case WASP_PATROL:
      // Check if bee is close enough to start hunting
      if (distToBee < WASP_DETECTION_RADIUS) {
        w.state = WASP_HUNTING;
        w.lastStateChangeMs = nowMs;
      }
      // Periodically pick new patrol target
      else if ((nowMs - w.lastStateChangeMs) > WASP_PATROL_CHANGE_MS) {
        pickPatrolTarget(w, nowMs);
      }
      break;

    case WASP_HUNTING:
      // Target is always the bee
      w.targetWX = beeWX;
      w.targetWY = beeWY;

      // Lose interest if bee escapes
      if (distToBee > WASP_LOSE_INTEREST_RADIUS) {
        w.state = WASP_PATROL;
        pickPatrolTarget(w, nowMs);
      }
      break;

    case WASP_STUNNED:
      // Wait for stun to wear off
      if ((int32_t)(nowMs - w.stunnedUntilMs) >= 0) {
        w.state = WASP_PATROL;
        pickPatrolTarget(w, nowMs);
      }
      break;
  }
}

// -------------------- PHYSICS UPDATE --------------------
static void updateWaspPhysics(Wasp &w, float dt) {
  if (!w.alive) return;
  if (w.state == WASP_STUNNED) {
    // Drift slowly when stunned
    w.vx *= 0.95f;
    w.vy *= 0.95f;
    w.wx += w.vx * dt;
    w.wy += w.vy * dt;
    return;
  }

  // Determine max speed based on state
  float maxSpeed = (w.state == WASP_HUNTING) ? WASP_HUNT_SPEED : WASP_PATROL_SPEED;

  // Spring-based movement toward target
  float forceX = WASP_SPRING_K * (w.targetWX - w.wx) - WASP_DAMPING * w.vx;
  float forceY = WASP_SPRING_K * (w.targetWY - w.wy) - WASP_DAMPING * w.vy;

  w.vx += forceX * dt;
  w.vy += forceY * dt;

  // Clamp to max speed
  float speed = sqrtf(w.vx * w.vx + w.vy * w.vy);
  if (speed > maxSpeed) {
    float scale = maxSpeed / speed;
    w.vx *= scale;
    w.vy *= scale;
  }

  w.wx += w.vx * dt;
  w.wy += w.vy * dt;

  // Update wing animation (faster than bee)
  float wingHz = 8.0f + (speed / maxSpeed) * 12.0f;  // 8-20 Hz
  w.wingPhase += 2.0f * 3.1415926f * wingHz * dt;
  if (w.wingPhase > 100.0f) w.wingPhase -= 100.0f;
}

// -------------------- COLLISION --------------------
bool checkWaspCollision(uint32_t nowMs) {
  // Bee is invulnerable after being hit
  if ((int32_t)(nowMs - beeInvulnUntilMs) < 0) {
    return false;
  }

  for (int i = 0; i < WASP_N; i++) {
    Wasp &w = wasps[i];
    if (!w.alive) continue;
    if (w.state == WASP_STUNNED) continue;

    float dx = w.wx - beeWX;
    float dy = w.wy - beeWY;
    float distSq = dx * dx + dy * dy;
    float hitDist = (float)(WASP_HIT_RADIUS + BEE_HIT_RADIUS);

    if (distSq <= hitDist * hitDist) {
      // Collision! Stun the wasp
      w.state = WASP_STUNNED;
      w.stunnedUntilMs = nowMs + WASP_STUN_DURATION_MS;

      // Grant bee invulnerability
      beeInvulnUntilMs = nowMs + WASP_INVULN_MS;

      return true;  // Signal hit to main loop
    }
  }
  return false;
}

// -------------------- MAIN UPDATE --------------------
void updateWasps(uint32_t nowMs, float dt) {
  for (int i = 0; i < WASP_N; i++) {
    if (!wasps[i].alive) continue;
    updateWaspAI(wasps[i], nowMs);
    updateWaspPhysics(wasps[i], dt);
  }
}

// -------------------- KILL ON-SCREEN WASPS --------------------
int killOnScreenWasps(uint32_t nowMs) {
  int killed = 0;

  // Screen bounds (with margin for wasp size)
  int screenW = 320;  // tft.width()
  int screenH = 240;  // tft.height()
  int margin = 20;

  for (int i = 0; i < WASP_N; i++) {
    Wasp &w = wasps[i];
    if (!w.alive) continue;

    // Convert world position to screen position
    int sx, sy;
    worldToScreenF(w.wx, w.wy, sx, sy);

    // Check if on screen (with margin)
    bool onScreen = (sx >= -margin && sx <= screenW + margin &&
                     sy >= HUD_H - margin && sy <= screenH + margin);

    if (onScreen) {
      w.alive = 0;
      w.deathMs = nowMs;  // Record death time for respawn timing
      killed++;

      // Spawn death particles at wasp location
      for (int p = 0; p < 4; p++) {
        float angle = (float)p * 1.5707963f;  // 90 degrees apart
        float px = w.wx + cosf(angle) * 8.0f;
        float py = w.wy + sinf(angle) * 8.0f;
        spawnTrailParticle(px, py, 0.8f, nowMs, 5);  // Variant 5 = red death particles
      }
    }
  }

  return killed;
}

// -------------------- QUERIES --------------------
bool isAnyWaspHunting() {
  for (int i = 0; i < WASP_N; i++) {
    if (wasps[i].alive && wasps[i].state == WASP_HUNTING) {
      return true;
    }
  }
  return false;
}

bool isBeeInvulnerable(uint32_t nowMs) {
  return (int32_t)(nowMs - beeInvulnUntilMs) < 0;
}

// -------------------- RESET --------------------
void resetWasps() {
  for (int i = 0; i < WASP_N; i++) {
    wasps[i].alive = 0;
    wasps[i].wx = 0.0f;
    wasps[i].wy = 0.0f;
    wasps[i].vx = 0.0f;
    wasps[i].vy = 0.0f;
    wasps[i].state = WASP_PATROL;
    wasps[i].wingPhase = 0.0f;
    wasps[i].stunnedUntilMs = 0;
    wasps[i].lastStateChangeMs = 0;
    wasps[i].deathMs = 0;
  }
  beeInvulnUntilMs = 0;
}
