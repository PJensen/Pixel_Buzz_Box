// Pixel Buzz Box - Flowers (Spawning, Styles, Collection)
#include "game.h"
#include "BuzzSynth.h"
#include <math.h>

extern BuzzSynth buzzer;

// -------------------- FLOWER STATE --------------------
Flower flowers[Pool::FLOWER_N];
uint32_t flowerBornMs[Pool::FLOWER_N];

// -------------------- STYLING --------------------
void initFlowerStyle(Flower &f) {
  struct RGB { uint8_t r, g, b; };

  if (f.type == FLOWER_RARE) {
    static const RGB rarePetals[] = {
      {255, 215, 100},  // Gold
      {255, 240, 150},  // Bright gold
      {255, 200,  80},  // Deep gold
    };
    int pi = irand(0, (int)(sizeof(rarePetals)/sizeof(rarePetals[0])) - 1);
    RGB p = rarePetals[pi];

    f.petal = rgb565(p.r, p.g, p.b);
    uint8_t r2 = (p.r > 52) ? (uint8_t)(p.r - 52) : 0;
    uint8_t g2 = (p.g > 52) ? (uint8_t)(p.g - 52) : 0;
    uint8_t b2 = (p.b > 52) ? (uint8_t)(p.b - 52) : 0;
    f.petalLo = rgb565(r2, g2, b2);

    f.center = rgb565(255, 100, 50);  // Bright orange-red center
  } else {
    static const RGB petals[] = {
      {255, 120, 180},
      {170, 120, 255},
      {120, 200, 255},
      {255, 170,  80},
      {120, 255, 170},
    };
    int pi = irand(0, (int)(sizeof(petals)/sizeof(petals[0])) - 1);
    RGB p = petals[pi];

    f.petal = rgb565(p.r, p.g, p.b);
    uint8_t r2 = (p.r > 52) ? (uint8_t)(p.r - 52) : 0;
    uint8_t g2 = (p.g > 52) ? (uint8_t)(p.g - 52) : 0;
    uint8_t b2 = (p.b > 52) ? (uint8_t)(p.b - 52) : 0;
    f.petalLo = rgb565(r2, g2, b2);

    f.center = rgb565(255, 235, 130);
  }
}

// -------------------- SPAWNING --------------------
void spawnFlowerAt(int i, int32_t wx, int32_t wy, FlowerType type) {
  Flower &f = flowers[i];
  f.alive = 1;
  if (type == FLOWER_RARE) {
    f.r = (uint8_t)irand(FlowerCfg::RARE_RADIUS_MIN, FlowerCfg::RARE_RADIUS_MAX);
  } else {
    f.r = (uint8_t)irand(FlowerCfg::RADIUS_MIN, FlowerCfg::RADIUS_MAX);
  }
  f.wx = wx;
  f.wy = wy;
  f.type = type;
  initFlowerStyle(f);
  flowerBornMs[i] = millis();
}

void spawnFlowerNearOrigin(int i) {
  for (int tries = 0; tries < FlowerCfg::SPAWN_NEAR_TRIES; tries++) {
    int32_t r = (int32_t)irand(FlowerCfg::SPAWN_NEAR_DIST_MIN, FlowerCfg::SPAWN_NEAR_DIST_MAX);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * MathConst::DEG2RAD;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    bool ok = true;
    for (int k = 0; k < i; k++) {
      int32_t dx = wx - flowers[k].wx;
      int32_t dy = wy - flowers[k].wy;
      if ((dx*dx + dy*dy) < (FlowerCfg::COLLISION_DIST*FlowerCfg::COLLISION_DIST)) { ok = false; break; }
    }
    if (!ok) continue;

    spawnFlowerAt(i, wx, wy);
    return;
  }
  // fallback
  int32_t r = (int32_t)irand(FlowerCfg::FALLBACK_NEAR_MIN, FlowerCfg::FALLBACK_NEAR_MAX);
  int32_t a = (int32_t)irand(0, 359);
  float ang = (float)a * MathConst::DEG2RAD;
  spawnFlowerAt(i, (int32_t)(cosf(ang) * (float)r), (int32_t)(sinf(ang) * (float)r));
}

void spawnFlowerElsewhere(int i) {
  bool isRare = (irand(0, 99) < FlowerCfg::RARE_SPAWN_CHANCE);
  FlowerType type = isRare ? FLOWER_RARE : FLOWER_NORMAL;

  int32_t distMin = isRare ? FlowerCfg::RARE_DIST_MIN : FlowerCfg::SPAWN_ELSEWHERE_DIST_MIN;
  float worldBoundary = getWorldBoundary();
  int32_t distMax = isRare ? FlowerCfg::RARE_DIST_MAX : ((int)worldBoundary - FlowerCfg::SPAWN_ELSEWHERE_MARGIN);

  for (int tries = 0; tries < FlowerCfg::SPAWN_ELSEWHERE_TRIES; tries++) {
    int32_t r = (int32_t)irand(distMin, distMax);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * MathConst::DEG2RAD;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    if ((wx*wx + wy*wy) > (int32_t)(worldBoundary * worldBoundary)) continue;

    int32_t dx_bee = wx - (int32_t)bee.wx;
    int32_t dy_bee = wy - (int32_t)bee.wy;
    if ((dx_bee*dx_bee + dy_bee*dy_bee) < (FlowerCfg::BEE_AVOIDANCE_DIST*FlowerCfg::BEE_AVOIDANCE_DIST)) continue;

    bool ok = true;
    for (int k = 0; k < Pool::FLOWER_N; k++) {
      if (k == i) continue;
      if (!flowers[k].alive) continue;
      int32_t dx = wx - flowers[k].wx;
      int32_t dy = wy - flowers[k].wy;
      if ((dx*dx + dy*dy) < (FlowerCfg::SPACING_ELSEWHERE*FlowerCfg::SPACING_ELSEWHERE)) { ok = false; break; }
    }
    if (!ok) continue;

    spawnFlowerAt(i, wx, wy, type);
    return;
  }
  // fallback
  int32_t r = (int32_t)irand(FlowerCfg::FALLBACK_ELSEWHERE_MIN, FlowerCfg::FALLBACK_ELSEWHERE_MAX);
  int32_t a = (int32_t)irand(0, 359);
  float ang = (float)a * MathConst::DEG2RAD;
  spawnFlowerAt(i, (int32_t)(cosf(ang) * (float)r), (int32_t)(sinf(ang) * (float)r), type);
}

void initFlowers() {
  for (int i = 0; i < Pool::FLOWER_N; i++) {
    flowers[i].alive = 0;
    spawnFlowerNearOrigin(i);
  }
}

// -------------------- COLLECTION --------------------
bool tryCollectPollen(uint32_t nowMs) {
  if (survival.pollenCount >= Pool::MAX_POLLEN_CARRY) return false;
  if (hive.isUnloading) return false;

  int32_t bx = (int32_t)bee.wx;
  int32_t by = (int32_t)bee.wy;

  for (int i = 0; i < Pool::FLOWER_N; i++) {
    Flower &f = flowers[i];
    if (!f.alive) continue;

    int32_t dx = bx - f.wx;
    int32_t dy = by - f.wy;
    int32_t hitR = (int32_t)f.r + FlowerCfg::BEE_HIT_RADIUS;

    if (isMagnetActive(nowMs)) {
      hitR *= 2;
    }

    if ((dx*dx + dy*dy) <= hitR*hitR) {
      bool isRare = (f.type == FLOWER_RARE);
      uint8_t pollenGain = isRare ? (1 + FlowerCfg::RARE_POLLEN_BONUS) : 1;
      survival.pollenCount += pollenGain;
      if (survival.pollenCount > Pool::MAX_POLLEN_CARRY) survival.pollenCount = Pool::MAX_POLLEN_CARRY;

      if (isRare) {
        for (int p = 0; p < FlowerCfg::RARE_PARTICLE_COUNT; p++) {
          float angle = (float)p * FlowerCfg::RARE_PARTICLE_ANGLE_STEP;
          float ox = cosf(angle) * FlowerCfg::PARTICLE_OFFSET;
          float oy = sinf(angle) * FlowerCfg::PARTICLE_OFFSET;
          spawnTrailParticle((float)f.wx + ox, (float)f.wy + oy, 0.8f, nowMs, Vfx::VARIANT_GOLD);
        }
      }

      f.alive = 0;
      spawnFlowerElsewhere(i);

      triggerAutoBoost(nowMs);

      if (isRare) {
        triggerCameraShake(nowMs, FlowerCfg::RARE_SHAKE_MAGNITUDE, FlowerCfg::RARE_SHAKE_DURATION);
      }

      if (!buzzer.soundBusy()) {
        buzzer.startSound(isRare ? SND_POWERUP : SND_POLLEN_CHIRP, nowMs);
      }
      return true;
    }
  }
  return false;
}

// -------------------- FLOWER PHYSICS (MAGNET PULL) --------------------
void updateFlowerPhysics(float dt, uint32_t nowMs) {
  if (!isMagnetActive(nowMs)) return;

  static uint32_t lastParticleMs[Pool::FLOWER_N] = {0};

  for (int i = 0; i < Pool::FLOWER_N; i++) {
    Flower &f = flowers[i];
    if (!f.alive) continue;

    int32_t dx = (int32_t)bee.wx - f.wx;
    int32_t dy = (int32_t)bee.wy - f.wy;
    int32_t distSq = dx*dx + dy*dy;

    if (distSq > (Magnet::PULL_RADIUS * Magnet::PULL_RADIUS)) continue;

    float dist = sqrtf((float)distSq);
    if (dist < 1.0f) continue;

    float ux = (float)dx / dist;
    float uy = (float)dy / dist;

    float falloff = 1.0f - (dist / (float)Magnet::PULL_RADIUS);
    float force = Magnet::SPRING_K * falloff * hive.magnet.strength;

    float moveX = ux * force * dt * 60.0f * Magnet::CINEMATIC_PULL_SPEED;
    float moveY = uy * force * dt * 60.0f * Magnet::CINEMATIC_PULL_SPEED;

    f.wx += (int32_t)moveX;
    f.wy += (int32_t)moveY;

    if ((nowMs - lastParticleMs[i]) > FlowerCfg::MAGNET_PARTICLE_INTERVAL) {
      float particleStrength = clampf(force / Magnet::SPRING_K, 0.4f, 1.0f);

      spawnTrailParticle((float)f.wx, (float)f.wy, particleStrength, nowMs, Vfx::VARIANT_CYAN);

      if (dist > FlowerCfg::MAGNET_TRAIL_MIN_DIST) {
        float trailX = (float)f.wx - (ux * FlowerCfg::MAGNET_TRAIL_OFFSET);
        float trailY = (float)f.wy - (uy * FlowerCfg::MAGNET_TRAIL_OFFSET);
        spawnTrailParticle(trailX, trailY, particleStrength * 0.7f, nowMs, Vfx::VARIANT_CYAN);
      }

      lastParticleMs[i] = nowMs;
    }
  }
}

// -------------------- TARGETING --------------------
bool findNearestFlower(int32_t &outWX, int32_t &outWY) {
  int best = -1;
  int64_t bestD2 = 0;

  int32_t bx = (int32_t)bee.wx;
  int32_t by = (int32_t)bee.wy;

  for (int i = 0; i < Pool::FLOWER_N; i++) {
    if (!flowers[i].alive) continue;
    int32_t dx = flowers[i].wx - bx;
    int32_t dy = flowers[i].wy - by;
    int64_t d2 = (int64_t)dx * (int64_t)dx + (int64_t)dy * (int64_t)dy;

    // Radar prioritizes rare flowers (treat them as 60% closer)
    if (flowers[i].type == FLOWER_RARE) {
      d2 = (d2 * 36) / 100;  // 0.6^2 = 0.36
    }

    if (best < 0 || d2 < bestD2) { best = i; bestD2 = d2; }
  }

  if (best < 0) return false;
  outWX = flowers[best].wx;
  outWY = flowers[best].wy;
  return true;
}
