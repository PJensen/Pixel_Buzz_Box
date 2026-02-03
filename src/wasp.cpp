// Pixel Buzz Box - Wasp (Predator AI, Physics, Collision)
#include "game.h"
#include <math.h>

// -------------------- WASP STATE --------------------
Wasp wasps[Pool::WASP_MAX];
uint32_t beeInvulnUntilMs = 0;

// -------------------- HELPERS --------------------
static float waspDistToBee(const Wasp &w) {
  float dx = w.wx - bee.wx;
  float dy = w.wy - bee.wy;
  return sqrtf(dx * dx + dy * dy);
}

static void pickPatrolTarget(Wasp &w, uint32_t nowMs) {
  float angle = (float)(xrnd() % 360) * MathConst::DEG2RAD;
  float dist = (float)(WaspCfg::PATROL_RADIUS / 2 + (int)(xrnd() % (WaspCfg::PATROL_RADIUS / 2)));
  w.targetWX = w.wx + cosf(angle) * dist;
  w.targetWY = w.wy + sinf(angle) * dist;
  w.lastStateChangeMs = nowMs;
}

// -------------------- SPAWNING --------------------
static void spawnWasp(int idx, uint32_t nowMs) {
  Wasp &w = wasps[idx];

  float angle = (float)(xrnd() % 360) * MathConst::DEG2RAD;
  int dist = WaspCfg::SPAWN_DIST_MIN + (int)(xrnd() % (WaspCfg::SPAWN_DIST_MAX - WaspCfg::SPAWN_DIST_MIN));

  w.wx = bee.wx + cosf(angle) * (float)dist;
  w.wy = bee.wy + sinf(angle) * (float)dist;
  w.vx = 0.0f;
  w.vy = 0.0f;
  w.alive = 1;
  w.state = WASP_PATROL;
  w.wingPhase = (float)(xrnd() % 100) * 0.01f * MathConst::TAU;
  w.stunnedUntilMs = 0;
  w.lastStateChangeMs = nowMs;

  pickPatrolTarget(w, nowMs);
}

void checkWaspSpawning(uint32_t nowMs) {
  // Max wasps = current day (game gets harder each day!)
  int maxWasps = (survival.currentDay < Pool::WASP_MAX) ? survival.currentDay : Pool::WASP_MAX;

  for (int i = 0; i < maxWasps; i++) {
    // Dynamic threshold: first wasp at 5, then +10 for each additional
    int threshold = WaspCfg::SPAWN_SCORE_BASE + (i * WaspCfg::SPAWN_SCORE_INCREMENT);
    if (survival.score < threshold) continue;
    if (wasps[i].alive) continue;

    if (wasps[i].deathMs == 0) {
      spawnWasp(i, nowMs);
    } else if ((nowMs - wasps[i].deathMs) >= WaspCfg::RESPAWN_DELAY_MS) {
      spawnWasp(i, nowMs);
      wasps[i].deathMs = 0;
    }
  }
}

// -------------------- AI STATE MACHINE --------------------
static void updateWaspAI(Wasp &w, uint32_t nowMs) {
  if (!w.alive) return;

  float distToBee = waspDistToBee(w);

  switch (w.state) {
    case WASP_PATROL:
      if (distToBee < WaspCfg::DETECTION_RADIUS) {
        w.state = WASP_HUNTING;
        w.lastStateChangeMs = nowMs;
      }
      else if ((nowMs - w.lastStateChangeMs) > WaspCfg::PATROL_CHANGE_MS) {
        pickPatrolTarget(w, nowMs);
      }
      break;

    case WASP_HUNTING:
      w.targetWX = bee.wx;
      w.targetWY = bee.wy;

      if (distToBee > WaspCfg::LOSE_INTEREST_RADIUS) {
        w.state = WASP_PATROL;
        pickPatrolTarget(w, nowMs);
      }
      break;

    case WASP_STUNNED:
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
    w.vx *= WaspCfg::STUN_DRIFT;
    w.vy *= WaspCfg::STUN_DRIFT;
    w.wx += w.vx * dt;
    w.wy += w.vy * dt;
    return;
  }

  float maxSpeed = (w.state == WASP_HUNTING) ? WaspCfg::HUNT_SPEED : WaspCfg::PATROL_SPEED;

  float forceX = WaspCfg::SPRING_K * (w.targetWX - w.wx) - WaspCfg::DAMPING * w.vx;
  float forceY = WaspCfg::SPRING_K * (w.targetWY - w.wy) - WaspCfg::DAMPING * w.vy;

  w.vx += forceX * dt;
  w.vy += forceY * dt;

  float speed = sqrtf(w.vx * w.vx + w.vy * w.vy);
  if (speed > maxSpeed) {
    float scale = maxSpeed / speed;
    w.vx *= scale;
    w.vy *= scale;
  }

  w.wx += w.vx * dt;
  w.wy += w.vy * dt;

  float wingHz = WaspCfg::WING_HZ_BASE + (speed / maxSpeed) * WaspCfg::WING_HZ_RANGE;
  w.wingPhase += 2.0f * MathConst::PI_F * wingHz * dt;
  if (w.wingPhase > WaspCfg::WING_PHASE_WRAP) w.wingPhase -= WaspCfg::WING_PHASE_WRAP;
}

// -------------------- COLLISION --------------------
bool checkWaspCollision(uint32_t nowMs) {
  if ((int32_t)(nowMs - beeInvulnUntilMs) < 0) {
    return false;
  }

  for (int i = 0; i < Pool::WASP_MAX; i++) {
    Wasp &w = wasps[i];
    if (!w.alive) continue;
    if (w.state == WASP_STUNNED) continue;

    float dx = w.wx - bee.wx;
    float dy = w.wy - bee.wy;
    float distSq = dx * dx + dy * dy;
    float hitDist = (float)(WaspCfg::HIT_RADIUS + FlowerCfg::BEE_HIT_RADIUS);

    if (distSq <= hitDist * hitDist) {
      w.state = WASP_STUNNED;
      w.stunnedUntilMs = nowMs + WaspCfg::STUN_DURATION_MS;
      beeInvulnUntilMs = nowMs + WaspCfg::INVULN_MS;
      return true;
    }
  }
  return false;
}

// -------------------- MAIN UPDATE --------------------
void updateWasps(uint32_t nowMs, float dt) {
  for (int i = 0; i < Pool::WASP_MAX; i++) {
    if (!wasps[i].alive) continue;
    updateWaspAI(wasps[i], nowMs);
    updateWaspPhysics(wasps[i], dt);
  }
}

// -------------------- KILL ON-SCREEN WASPS --------------------
int killOnScreenWasps(uint32_t nowMs) {
  int killed = 0;

  for (int i = 0; i < Pool::WASP_MAX; i++) {
    Wasp &w = wasps[i];
    if (!w.alive) continue;

    int sx, sy;
    worldToScreenF(w.wx, w.wy, sx, sy);

    bool onScreen = (sx >= -WaspCfg::SCREEN_MARGIN && sx <= Display::SCREEN_W + WaspCfg::SCREEN_MARGIN &&
                     sy >= Display::HUD_H - WaspCfg::SCREEN_MARGIN && sy <= Display::SCREEN_H + WaspCfg::SCREEN_MARGIN);

    if (onScreen) {
      w.alive = 0;
      w.deathMs = nowMs;
      killed++;

      for (int p = 0; p < WaspCfg::DEATH_PARTICLE_COUNT; p++) {
        float angle = (float)p * (MathConst::TAU / WaspCfg::DEATH_PARTICLE_COUNT);
        float px = w.wx + cosf(angle) * WaspCfg::DEATH_PARTICLE_SPREAD;
        float py = w.wy + sinf(angle) * WaspCfg::DEATH_PARTICLE_SPREAD;
        spawnTrailParticle(px, py, 0.8f, nowMs, Vfx::VARIANT_RED);
      }
    }
  }

  return killed;
}

// -------------------- QUERIES --------------------
bool isAnyWaspHunting() {
  for (int i = 0; i < Pool::WASP_MAX; i++) {
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
  for (int i = 0; i < Pool::WASP_MAX; i++) {
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
