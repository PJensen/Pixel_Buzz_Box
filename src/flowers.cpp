// Pixel Buzz Box - Flowers (Spawning, Styles, Collection)
#include "game.h"
#include "BuzzSynth.h"
#include <math.h>

extern BuzzSynth buzzer;

// -------------------- FLOWER STATE --------------------
Flower flowers[FLOWER_N];
uint32_t flowerBornMs[FLOWER_N];

// -------------------- STYLING --------------------
void initFlowerStyle(Flower &f) {
  struct RGB { uint8_t r, g, b; };

  // Rare flowers have special gold/shimmer colors for visual distinction
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
    // Normal flowers use standard color palette
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
  // Rare flowers are larger for better visual distinction
  if (type == FLOWER_RARE) {
    f.r = (uint8_t)irand(RARE_FLOWER_RADIUS_MIN, RARE_FLOWER_RADIUS_MAX);
  } else {
    f.r = (uint8_t)irand(FLOWER_RADIUS_MIN, FLOWER_RADIUS_MAX);
  }
  f.wx = wx;
  f.wy = wy;
  f.type = type;
  initFlowerStyle(f);
  flowerBornMs[i] = millis();
}

void spawnFlowerNearOrigin(int i) {
  for (int tries = 0; tries < 60; tries++) {
    int32_t r = (int32_t)irand(FLOWER_SPAWN_NEAR_DIST_MIN, FLOWER_SPAWN_NEAR_DIST_MAX);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * 0.0174532925f;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    bool ok = true;
    for (int k = 0; k < i; k++) {
      int32_t dx = wx - flowers[k].wx;
      int32_t dy = wy - flowers[k].wy;
      if ((dx*dx + dy*dy) < (FLOWER_COLLISION_DIST*FLOWER_COLLISION_DIST)) { ok = false; break; }
    }
    if (!ok) continue;

    spawnFlowerAt(i, wx, wy);
    return;
  }
  // fallback
  int32_t r = (int32_t)irand(100, 180);
  int32_t a = (int32_t)irand(0, 359);
  float ang = (float)a * 0.0174532925f;
  spawnFlowerAt(i, (int32_t)(cosf(ang) * (float)r), (int32_t)(sinf(ang) * (float)r));
}

void spawnFlowerElsewhere(int i) {
  // Determine if this should be a rare flower (risk/reward mechanic)
  bool isRare = (irand(0, 99) < RARE_FLOWER_SPAWN_CHANCE);
  FlowerType type = isRare ? FLOWER_RARE : FLOWER_NORMAL;

  // Rare flowers spawn farther out in riskier zones
  int32_t distMin = isRare ? RARE_FLOWER_DIST_MIN : FLOWER_SPAWN_ELSEWHERE_DIST_MIN;
  int32_t distMax = isRare ? RARE_FLOWER_DIST_MAX : ((int)BOUNDARY_COMFORTABLE - FLOWER_SPAWN_ELSEWHERE_MARGIN);

  for (int tries = 0; tries < 80; tries++) {
    int32_t r = (int32_t)irand(distMin, distMax);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * 0.0174532925f;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    if ((wx*wx + wy*wy) > (int32_t)(BOUNDARY_COMFORTABLE * BOUNDARY_COMFORTABLE)) continue;

    int32_t dx_bee = wx - (int32_t)beeWX;
    int32_t dy_bee = wy - (int32_t)beeWY;
    if ((dx_bee*dx_bee + dy_bee*dy_bee) < (FLOWER_BEE_AVOIDANCE_DIST*FLOWER_BEE_AVOIDANCE_DIST)) continue;

    bool ok = true;
    for (int k = 0; k < FLOWER_N; k++) {
      if (k == i) continue;
      if (!flowers[k].alive) continue;
      int32_t dx = wx - flowers[k].wx;
      int32_t dy = wy - flowers[k].wy;
      if ((dx*dx + dy*dy) < (FLOWER_SPACING_ELSEWHERE*FLOWER_SPACING_ELSEWHERE)) { ok = false; break; }
    }
    if (!ok) continue;

    spawnFlowerAt(i, wx, wy, type);
    return;
  }
  // fallback
  int32_t r = (int32_t)irand(80, 200);
  int32_t a = (int32_t)irand(0, 359);
  float ang = (float)a * 0.0174532925f;
  spawnFlowerAt(i, (int32_t)(cosf(ang) * (float)r), (int32_t)(sinf(ang) * (float)r), type);
}

void initFlowers() {
  for (int i = 0; i < FLOWER_N; i++) {
    flowers[i].alive = 0;
    spawnFlowerNearOrigin(i);
  }
}

// -------------------- COLLECTION --------------------
bool tryCollectPollen(uint32_t nowMs) {
  if (pollenCount >= MAX_POLLEN_CARRY) return false;
  if (isUnloading) return false;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;

  for (int i = 0; i < FLOWER_N; i++) {
    Flower &f = flowers[i];
    if (!f.alive) continue;

    int32_t dx = bx - f.wx;
    int32_t dy = by - f.wy;
    int32_t hitR = (int32_t)f.r + BEE_HIT_RADIUS;

    // Pollen Magnet: 2x collection range
    if (pollenMagnetActive) {
      hitR *= 2;
    }

    if ((dx*dx + dy*dy) <= hitR*hitR) {
      // Rare flowers grant bonus pollen (risk/reward)
      bool isRare = (f.type == FLOWER_RARE);
      uint8_t pollenGain = isRare ? (1 + RARE_FLOWER_POLLEN_BONUS) : 1;
      pollenCount += pollenGain;
      if (pollenCount > MAX_POLLEN_CARRY) pollenCount = MAX_POLLEN_CARRY;

      // Spawn gold particle burst for rare flowers
      if (isRare) {
        for (int p = 0; p < 6; p++) {
          float angle = (float)p * 1.047f;  // 60 degrees apart
          float ox = cosf(angle) * 8.0f;
          float oy = sinf(angle) * 8.0f;
          spawnTrailParticle((float)f.wx + ox, (float)f.wy + oy, 0.8f, nowMs, 3);  // variant 3 = gold
        }
      }

      f.alive = 0;
      spawnFlowerElsewhere(i);

      // Auto-boost on flower pickup
      triggerAutoBoost(nowMs);

      // Rare flowers have enhanced feedback
      if (isRare) {
        triggerCameraShake(nowMs, 8.5f, 220);  // Stronger shake than normal
      }

      // Rare flowers play special powerup sound
      if (!buzzer.soundBusy()) {
        buzzer.startSound(isRare ? SND_POWERUP : SND_POLLEN_CHIRP, nowMs);
      }
      return true;
    }
  }
  return false;
}

// -------------------- TARGETING --------------------
bool findNearestFlower(int32_t &outWX, int32_t &outWY) {
  int best = -1;
  int64_t bestD2 = 0;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;

  for (int i = 0; i < FLOWER_N; i++) {
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
