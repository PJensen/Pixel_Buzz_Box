// Maeve Box — Infinite World Radar + Boost (Pico + ST7789 + Joystick + Buzzer)
// Board: Raspberry Pi Pico (rp2040 core)
// Arduino: 1.8.19
//
// Core idea:
// - Bee stays centered; the *world* scrolls.
// - Hive lives at world origin (0,0).
// - Flowers exist in world coords (infinite-ish).
// - Click = radar ping (nearest flower / hive if carrying). If stick is pushed + boost charge, click triggers boost.
// - Every 3 deposits earns 1 boost charge (max 1). Boost has a cooldown.
// - Conveyor belt HUD bottom-right (moonbugs vibe).
// - High-contrast, moving starfield + subtle world grid for anchoring.

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

// -------------------- PINS --------------------
// LCD (your existing wiring)
static const int PIN_BL   = 16;
static const int PIN_CS   = 17;
static const int PIN_SCK  = 18;
static const int PIN_MOSI = 19;
static const int PIN_DC   = 20;
static const int PIN_RST  = 21;

// Joystick
static const int PIN_JOY_SW  = 22; // GP22 (phys 29)
static const int PIN_JOY_VRX = 26; // GP26/ADC0 (phys 31)
static const int PIN_JOY_VRY = 27; // GP27/ADC1 (phys 32)

// Buzzer
static const int PIN_BUZZ = 15;   // GP15 (phys 20)

Adafruit_ST7789 tft(&SPI, PIN_CS, PIN_DC, PIN_RST);

// -------------------- TYPE DEFINITIONS (must come before any functions for Arduino) --------------------
// FLOWERS
struct Flower {
  int32_t wx, wy;
  uint8_t alive;
  uint8_t r;
  uint16_t petal;
  uint16_t petalLo;
  uint16_t center;
};

// BELT HUD
struct BeltItem {
  uint32_t bornMs;
  uint8_t alive;
};

// SOUND SCHEDULER
enum SndMode : uint8_t {
  SND_IDLE = 0,
  SND_CLICK,
  SND_RADAR,
  SND_POLLEN_CHIRP,
  SND_POWERUP,
};

// -------------------- UTIL --------------------
static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// RGB565 helper
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// -------------------- COLORS --------------------
static const uint16_t COL_BG0        = rgb565(  4,  8, 18);
static const uint16_t COL_BG1        = rgb565(  8, 12, 26);
static const uint16_t COL_GRID       = rgb565( 18, 28, 52);
static const uint16_t COL_GRID2      = rgb565( 12, 18, 34);
static const uint16_t COL_STAR       = rgb565(240,240,240);
static const uint16_t COL_STAR2      = rgb565(180,210,255);
static const uint16_t COL_STAR3      = rgb565(255,230,180);
static const uint16_t COL_WHITE      = rgb565(245,245,245);
static const uint16_t COL_YEL        = rgb565(255,220, 40);
static const uint16_t COL_BLK        = rgb565( 20, 20, 20);
static const uint16_t COL_WING       = rgb565(180,220,255);
static const uint16_t COL_HIVE       = rgb565( 90,140, 90);
static const uint16_t COL_HUD_BG     = rgb565(  0,  0,  0);
static const uint16_t COL_POLLEN     = rgb565(255,235,110);
static const uint16_t COL_POLLEN_HI  = rgb565(255,255,210);
static const uint16_t COL_SHADOW     = rgb565(  0,  0,  0);
static const uint16_t COL_SHADOW_RIM = rgb565( 20, 20, 20);
static const uint16_t COL_UI_DIM     = rgb565(120,140,170);
static const uint16_t COL_UI_GO      = rgb565( 80,210,140);
static const uint16_t COL_UI_WARN    = rgb565(255,120,120);

static const int HUD_H = 28;

// -------------------- CANVAS (micro-tiles) --------------------
// Exact fit for 240x320: 2 cols x 4 rows
static const int CANVAS_W = 120;
static const int CANVAS_H = 80;
static GFXcanvas16 canvas(CANVAS_W, CANVAS_H);

// -------------------- RNG --------------------
static uint32_t rngState = 0xA5A5F00Du;
static inline uint32_t xrnd() {
  uint32_t x = rngState;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  return (rngState = x);
}
static inline int irand(int lo, int hi) { // inclusive
  uint32_t r = xrnd();
  return lo + (int)(r % (uint32_t)(hi - lo + 1));
}
static inline uint32_t hash32(uint32_t x) {
  // a small integer hash
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

// -------------------- FLOWERS --------------------
static const int FLOWER_N = 7;
static Flower flowers[FLOWER_N];

// -------------------- BELT HUD --------------------
static const int BELT_ITEM_N = 10;
static BeltItem beltItems[BELT_ITEM_N];
static const uint32_t BELT_LIFE_MS = 14000;

// -------------------- SOUND SCHEDULER --------------------
static SndMode sndMode = SND_IDLE;
static uint8_t sndStep = 0;
static uint32_t sndNextMs = 0;

// -------------------- BOOST --------------------
static uint8_t depositsTowardBoost = 0;
static uint8_t boostCharge = 0; // 0 or 1
static uint32_t boostActiveUntilMs = 0;
static uint32_t boostCooldownUntilMs = 0;

// -------------------- RADAR --------------------
static bool radarActive = false;
static uint32_t radarUntilMs = 0;
static int32_t radarTargetWX = 0;
static int32_t radarTargetWY = 0;
static bool radarToHive = false;

// Button edge tracking
static bool btnPrev = false;

// -------------------- WING ANIM + SHADOW --------------------
static float wingPhase = 0.0f;
static float wingSpeed = 0.0f;

// -------------------- BOOST TRAIL EFFECT --------------------
struct TrailParticle {
  float wx, wy;
  uint32_t bornMs;
  uint8_t alive;
  uint8_t variant; // 0-2 for visual variety
};
static const int TRAIL_MAX = 24; // doubled for more impact
static TrailParticle trail[TRAIL_MAX];
static int trailNextIdx = 0;

// -------------------- SURVIVAL TIMER --------------------
static const float SURVIVAL_TIME_MAX = 30.0f; // seconds
static float survivalTimeLeft = SURVIVAL_TIME_MAX;
static bool isGameOver = false;
static uint32_t gameOverMs = 0;

// -------------------- WORLD CONSTRAINTS --------------------
static const float GRAVITY_BASE_STRENGTH = 0.8f;   // base gravity multiplier
static const float GRAVITY_DISTANCE_POWER = 1.5f;  // how much gravity increases with distance (1.5 = square root scaling)
static const float GRAVITY_REFERENCE_DIST = 100.0f; // reference distance for scaling
// Gravity zones for visual feedback
static const float GRAVITY_ZONE_SAFE = 150.0f;   // green zone - easy movement
static const float GRAVITY_ZONE_WARN = 300.0f;   // yellow zone - getting harder
static const float GRAVITY_ZONE_DANGER = 450.0f; // red zone - very hard to escape

// -------------------- INPUT --------------------
static int readJoyX() { return analogRead(PIN_JOY_VRX); } // 0..1023
static int readJoyY() { return analogRead(PIN_JOY_VRY); } // 0..1023
static bool joyPressedRaw() { return digitalRead(PIN_JOY_SW) == LOW; }

static int joyCenterX = 512;
static int joyCenterY = 512;
static int joyMinY = 1023;
static int joyMaxY = 0;

static int applyDeadzone(int v, int center, int dz) {
  int d = v - center;
  if (d > -dz && d < dz) return 0;
  return d;
}

static void calibrateJoystick() {
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

// -------------------- WORLD + GAME STATE --------------------
// Bee lives in world-space, but renders fixed at center of playfield.
static float beeWX = 0.0f, beeWY = 0.0f;
static float beeVX = 0.0f, beeVY = 0.0f;

static bool hasPollen = false;
static uint16_t score = 0;

// Bee render center (screen-space)
static inline int beeScreenCX() { return tft.width() / 2; }
static inline int beeScreenCY() { return (tft.height() + HUD_H) / 2; } // center of playfield

static void initFlowerStyle(Flower &f) {
  // store petals as RGB table so we can derive a darker tint cheaply once
  struct RGB { uint8_t r,g,b; };
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

static void spawnFlowerAt(int i, int32_t wx, int32_t wy) {
  Flower &f = flowers[i];
  f.alive = 1;
  f.r = (uint8_t)irand(6, 11);
  f.wx = wx;
  f.wy = wy;
  initFlowerStyle(f);
}

static void spawnFlowerNearOrigin(int i) {
  // Spawn in a friendly band around the hive so there's always something nearby at start.
  // Keep them out of the hive radius but not far.
  for (int tries = 0; tries < 60; tries++) {
    int32_t r = (int32_t)irand(90, 220);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * 0.0174532925f;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    // avoid clustering too hard
    bool ok = true;
    for (int k = 0; k < i; k++) {
      int32_t dx = wx - flowers[k].wx;
      int32_t dy = wy - flowers[k].wy;
      if ((dx*dx + dy*dy) < (80*80)) { ok = false; break; }
    }
    if (!ok) continue;

    spawnFlowerAt(i, wx, wy);
    return;
  }
  // fallback - safe spawn near origin
  int32_t r = (int32_t)irand(100, 180);
  int32_t a = (int32_t)irand(0, 359);
  float ang = (float)a * 0.0174532925f;
  spawnFlowerAt(i, (int32_t)(cosf(ang) * (float)r), (int32_t)(sinf(ang) * (float)r));
}

static void spawnFlowerElsewhere(int i) {
  // Respawn away from current bee position but within reasonable range.
  // Try to place offscreen but reachable (within warning zone).
  for (int tries = 0; tries < 80; tries++) {
    // Random position within warning zone (still reachable)
    int32_t r = (int32_t)irand(60, (int)GRAVITY_ZONE_WARN - 40);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * 0.0174532925f;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    // Keep within warning zone
    if ((wx*wx + wy*wy) > (int32_t)(GRAVITY_ZONE_WARN * GRAVITY_ZONE_WARN)) continue;

    // Try to keep away from bee (so radar is useful)
    int32_t dx_bee = wx - (int32_t)beeWX;
    int32_t dy_bee = wy - (int32_t)beeWY;
    if ((dx_bee*dx_bee + dy_bee*dy_bee) < (150*150)) continue;

    // avoid overlapping other flowers
    bool ok = true;
    for (int k = 0; k < FLOWER_N; k++) {
      if (k == i) continue;
      if (!flowers[k].alive) continue;
      int32_t dx = wx - flowers[k].wx;
      int32_t dy = wy - flowers[k].wy;
      if ((dx*dx + dy*dy) < (120*120)) { ok = false; break; }
    }
    if (!ok) continue;

    spawnFlowerAt(i, wx, wy);
    return;
  }
  // fallback: spawn near origin
  int32_t r = (int32_t)irand(80, 200);
  int32_t a = (int32_t)irand(0, 359);
  float ang = (float)a * 0.0174532925f;
  spawnFlowerAt(i, (int32_t)(cosf(ang) * (float)r), (int32_t)(sinf(ang) * (float)r));
}

static void initFlowers() {
  for (int i = 0; i < FLOWER_N; i++) {
    flowers[i].alive = 0;
    spawnFlowerNearOrigin(i);
  }
}

// -------------------- FUNCTION IMPLEMENTATIONS --------------------
// Belt HUD functions
static void spawnBeltItem(uint32_t nowMs) {
  int freeIdx = -1;
  uint32_t oldest = 0xFFFFFFFFu;
  int oldestIdx = 0;

  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) { freeIdx = i; break; }
    if (beltItems[i].bornMs < oldest) { oldest = beltItems[i].bornMs; oldestIdx = i; }
  }

  int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
  beltItems[idx].alive = 1;
  beltItems[idx].bornMs = nowMs;
}

static void updateBeltLifetimes(uint32_t nowMs) {
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    if ((uint32_t)(nowMs - beltItems[i].bornMs) > BELT_LIFE_MS) beltItems[i].alive = 0;
  }
}

// Sound functions
static void startSound(SndMode m, uint32_t nowMs) {
  sndMode = m;
  sndStep = 0;
  sndNextMs = nowMs;
}

static void updateSound(uint32_t nowMs) {
  if (sndMode == SND_IDLE) return;
  if ((int32_t)(nowMs - sndNextMs) < 0) return;

  switch (sndMode) {
    case SND_CLICK:
      if (sndStep == 0) {
        tone(PIN_BUZZ, 1500, 18);
        sndNextMs = nowMs + 22;
        sndStep++;
      } else {
        noTone(PIN_BUZZ);
        sndMode = SND_IDLE;
      }
      break;

    case SND_RADAR:
      // percussive tick + two pips
      if (sndStep == 0) {
        tone(PIN_BUZZ, 1200, 14);
        sndNextMs = nowMs + 18;
        sndStep++;
      } else if (sndStep == 1) {
        tone(PIN_BUZZ, 760, 50);
        sndNextMs = nowMs + 60;
        sndStep++;
      } else if (sndStep == 2) {
        tone(PIN_BUZZ, 980, 55);
        sndNextMs = nowMs + 70;
        sndStep++;
      } else {
        noTone(PIN_BUZZ);
        sndMode = SND_IDLE;
      }
      break;

    case SND_POLLEN_CHIRP:
      if (sndStep == 0) {
        tone(PIN_BUZZ, 720, 55);
        sndNextMs = nowMs + 65;
        sndStep++;
      } else if (sndStep == 1) {
        tone(PIN_BUZZ, 880, 55);
        sndNextMs = nowMs + 65;
        sndStep++;
      } else if (sndStep == 2) {
        tone(PIN_BUZZ, 620, 80);
        sndNextMs = nowMs + 95;
        sndStep++;
      } else {
        noTone(PIN_BUZZ);
        sndMode = SND_IDLE;
      }
      break;

    case SND_POWERUP:
      // short rising arpeggio
      if (sndStep == 0) {
        tone(PIN_BUZZ, 520, 70);
        sndNextMs = nowMs + 78;
        sndStep++;
      } else if (sndStep == 1) {
        tone(PIN_BUZZ, 760, 70);
        sndNextMs = nowMs + 78;
        sndStep++;
      } else if (sndStep == 2) {
        tone(PIN_BUZZ, 1040, 90);
        sndNextMs = nowMs + 105;
        sndStep++;
      } else {
        noTone(PIN_BUZZ);
        sndMode = SND_IDLE;
      }
      break;

    default:
      sndMode = SND_IDLE;
      noTone(PIN_BUZZ);
      break;
  }
}

static bool soundBusy() {
  return sndMode != SND_IDLE;
}

// -------------------- WORLD HELPERS --------------------
static inline void worldToScreen(int32_t wx, int32_t wy, int &sx, int &sy) {
  sx = beeScreenCX() + (int)(wx - (int32_t)beeWX);
  sy = beeScreenCY() + (int)(wy - (int32_t)beeWY);
}

static inline uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt) {
  return hash32((uint32_t)cx * 73856093u ^ (uint32_t)cy * 19349663u ^ salt);
}

// -------------------- DRAWING PRIMITIVES --------------------
static void drawBeeShadow(Adafruit_GFX &g, int x, int y) {
  int sy = y + 14;
  float s = 0.5f + 0.5f * sinf(wingPhase);
  int rx = 10 + (int)(3 * (1.0f - s)) + (int)(2 * wingSpeed);
  int ry = 3  + (int)(2 * (1.0f - s));

  for (int yy = -ry; yy <= ry; yy++) {
    float yf = (float)yy / (float)ry;
    float inside = 1.0f - yf * yf;
    if (inside <= 0.0f) continue;
    int span = (int)(rx * sqrtf(inside));

    int yrow = sy + yy;
    for (int xx = -span; xx <= span; xx++) {
      int xcol = x + xx;
      if (((xcol + yrow) & 1) == 0) g.drawPixel(xcol, yrow, COL_SHADOW);
    }
  }
  g.drawFastHLine(x - rx + 2, sy, rx * 2 - 4, COL_SHADOW_RIM);
}

// Trail particle functions
static void spawnTrailParticle(float wx, float wy, uint32_t nowMs) {
  trail[trailNextIdx].wx = wx;
  trail[trailNextIdx].wy = wy;
  trail[trailNextIdx].bornMs = nowMs;
  trail[trailNextIdx].alive = 1;
  trail[trailNextIdx].variant = (uint8_t)(xrnd() % 3);
  trailNextIdx = (trailNextIdx + 1) % TRAIL_MAX;
}

static void updateTrailParticles(uint32_t nowMs) {
  const uint32_t TRAIL_LIFE_MS = 250; // fade out after 250ms
  for (int i = 0; i < TRAIL_MAX; i++) {
    if (!trail[i].alive) continue;
    if ((uint32_t)(nowMs - trail[i].bornMs) > TRAIL_LIFE_MS) {
      trail[i].alive = 0;
    }
  }
}

static void drawTrailParticles(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  const uint32_t TRAIL_LIFE_MS = 300; // slightly longer life
  for (int i = 0; i < TRAIL_MAX; i++) {
    if (!trail[i].alive) continue;
    uint32_t age = nowMs - trail[i].bornMs;
    if (age > TRAIL_LIFE_MS) continue;

    // Convert world to screen
    int sx = beeScreenCX() + (int)(trail[i].wx - beeWX);
    int sy = beeScreenCY() + (int)(trail[i].wy - beeWY);

    // Fade out based on age
    float t = (float)age / (float)TRAIL_LIFE_MS;
    float alpha = 1.0f - t * t; // quadratic fade looks better

    // Boost glow - bright yellow to orange gradient
    uint8_t r = (uint8_t)(255 * alpha);
    uint8_t g_val = (uint8_t)(220 * alpha);
    uint8_t b = (uint8_t)(60 * alpha);

    // Multi-layer glow for impact!
    if (alpha > 0.6f) {
      // Outer glow (larger, dimmer)
      uint16_t outerGlow = rgb565(r / 3, g_val / 3, b / 3);
      g.fillCircle(sx + ox, sy + oy, 5, outerGlow);

      // Mid glow
      uint16_t midGlow = rgb565(r / 2, g_val / 2, b / 2);
      g.fillCircle(sx + ox, sy + oy, 3, midGlow);

      // Core (bright)
      uint16_t core = rgb565(r, g_val, b);
      g.fillCircle(sx + ox, sy + oy, 2, core);

      // Sparkle variants
      if (trail[i].variant == 0 && alpha > 0.8f) {
        // Cross sparkle
        uint16_t sparkle = rgb565(255, 255, 200);
        g.drawPixel(sx - 3 + ox, sy + oy, sparkle);
        g.drawPixel(sx + 3 + ox, sy + oy, sparkle);
        g.drawPixel(sx + ox, sy - 3 + oy, sparkle);
        g.drawPixel(sx + ox, sy + 3 + oy, sparkle);
      }
    } else if (alpha > 0.3f) {
      // Mid-fade: smaller but still visible
      uint16_t midGlow = rgb565(r / 2, g_val / 2, b / 2);
      g.fillCircle(sx + ox, sy + oy, 3, midGlow);

      uint16_t core = rgb565(r, g_val, b);
      g.fillCircle(sx + ox, sy + oy, 1, core);
    } else {
      // Fading out: just a dot
      uint16_t dim = rgb565(r, g_val, b);
      g.drawPixel(sx + ox, sy + oy, dim);
    }
  }
}

static void drawBee(Adafruit_GFX &g, int x, int y) {
  uint16_t body = hasPollen ? rgb565(255, 245, 140) : COL_YEL;

  float s = sinf(wingPhase);
  int flap = (int)(s * (2 + (int)(3 * wingSpeed)));
  int wH   = 4 + (int)(2 * (0.5f + 0.5f * s));
  int wW   = 7 + (int)(2 * wingSpeed);

  g.fillEllipse(x - 6, y - 9 + flap, wW, wH, COL_WING);
  g.fillEllipse(x + 2, y - 10 - flap/2, wW, wH, COL_WING);
  g.drawEllipse(x - 6, y - 9 + flap, wW, wH, COL_WHITE);
  g.drawEllipse(x + 2, y - 10 - flap/2, wW, wH, COL_WHITE);

  if (s > 0.35f) {
    g.drawPixel(x - 9, y - 12 + flap, COL_POLLEN_HI);
    g.drawPixel(x + 5, y - 13 - flap/2, COL_POLLEN_HI);
  }

  g.fillEllipse(x, y, 12, 8, body);
  g.fillRect(x - 9, y - 6, 4, 12, COL_BLK);
  g.fillRect(x - 1, y - 6, 4, 12, COL_BLK);
  g.drawEllipse(x, y, 12, 8, COL_WHITE);

  g.fillCircle(x + 11, y - 1, 5, COL_BLK);
  g.drawCircle(x + 11, y - 1, 5, COL_WHITE);

  g.fillTriangle(x - 13, y, x - 18, y - 2, x - 18, y + 2, COL_BLK);

  if (hasPollen) {
    g.fillCircle(x + 3, y + 8, 3, COL_YEL);
    g.drawCircle(x + 3, y + 8, 3, COL_WHITE);
  }
}

static void drawHive(Adafruit_GFX &g, int x, int y) {
  g.drawCircle(x, y, 12, COL_HIVE);
  g.drawCircle(x, y,  7, COL_HIVE);
  g.drawCircle(x, y,  2, COL_HIVE);
}

static void drawFlower(Adafruit_GFX &g, int x, int y, const Flower &f) {
  if (!f.alive) return;
  int r = (int)f.r;

  // subtle shadow underlay
  int sx = x + 1;
  int sy = y + 1;
  g.fillCircle(sx - r, sy, r, f.petalLo);
  g.fillCircle(sx + r, sy, r, f.petalLo);
  g.fillCircle(sx, sy - r, r, f.petalLo);
  g.fillCircle(sx, sy + r, r, f.petalLo);
  g.fillCircle(sx, sy, r, f.petalLo);

  // petals
  g.fillCircle(x - r, y, r, f.petal);
  g.fillCircle(x + r, y, r, f.petal);
  g.fillCircle(x, y - r, r, f.petal);
  g.fillCircle(x, y + r, r, f.petal);
  g.fillCircle(x, y, r, f.petal);

  int cr = r / 2 + 2;
  g.fillCircle(x, y, cr, f.center);
  g.drawCircle(x, y, cr, COL_WHITE);

  g.drawPixel(x - 1, y - 1, COL_POLLEN_HI);
  g.drawPixel(x - 2, y - 1, COL_WHITE);
}

// -------------------- BACKGROUND --------------------
static void drawGravityZones(Adafruit_GFX &g, int ox, int oy) {
  // Simple outer penumbra - just shows the comfortable play area
  int hiveX = beeScreenCX() + ox;
  int hiveY = beeScreenCY() + oy;

  // Draw a subtle outer boundary circle
  // Only shows when bee is getting far from center
  float distToHive = sqrtf(beeWX * beeWX + beeWY * beeWY);

  if (distToHive > GRAVITY_ZONE_SAFE * 0.5f) {
    // Simple faint boundary ring
    uint16_t boundaryColor = rgb565(40, 60, 80);
    g.drawCircle(hiveX, hiveY, (int)GRAVITY_ZONE_WARN, boundaryColor);
  }
}

static void drawWorldGrid(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  // Subtle world-space grid lines (anchor + motion). Spacing in world units.
  const int GRID = 160;
  const int GRID2 = 80;

  // tile screen rect
  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + CANVAS_W - 1;
  int sy1 = tileY + CANVAS_H - 1;

  // world rect
  int32_t wx0 = (int32_t)beeWX + (sx0 - beeScreenCX());
  int32_t wy0 = (int32_t)beeWY + (sy0 - beeScreenCY());
  int32_t wx1 = (int32_t)beeWX + (sx1 - beeScreenCX());
  int32_t wy1 = (int32_t)beeWY + (sy1 - beeScreenCY());

  // vertical lines
  int32_t gx0 = (int32_t)floorf((float)wx0 / (float)GRID2) * GRID2;
  for (int32_t gx = gx0; gx <= wx1; gx += GRID2) {
    int sx = beeScreenCX() + (int)(gx - (int32_t)beeWX);
    if (sx < sx0 || sx > sx1) continue;
    bool major = ((gx % GRID) == 0);
    uint16_t c = major ? COL_GRID : COL_GRID2;
    g.drawFastVLine(sx + ox, sy0 + oy, CANVAS_H, c);
  }

  // horizontal lines
  int32_t gy0 = (int32_t)floorf((float)wy0 / (float)GRID2) * GRID2;
  for (int32_t gy = gy0; gy <= wy1; gy += GRID2) {
    int sy = beeScreenCY() + (int)(gy - (int32_t)beeWY);
    if (sy < sy0 || sy > sy1) continue;
    bool major = ((gy % GRID) == 0);
    uint16_t c = major ? COL_GRID : COL_GRID2;
    g.drawFastHLine(sx0 + ox, sy + oy, CANVAS_W, c);
  }
}

static void drawStarLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy,
                          float parallax, int cell, uint16_t cA, uint16_t cB, uint32_t salt) {
  // Stable hashed starfield in world-space, with parallax.
  // parallax < 1.0 makes a "deeper" layer.

  // effective camera for this layer
  float camX = beeWX * parallax;
  float camY = beeWY * parallax;

  // screen rect
  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + CANVAS_W - 1;
  int sy1 = tileY + CANVAS_H - 1;

  // world rect for the layer
  int32_t wx0 = (int32_t)camX + (sx0 - beeScreenCX());
  int32_t wy0 = (int32_t)camY + (sy0 - beeScreenCY());
  int32_t wx1 = (int32_t)camX + (sx1 - beeScreenCX());
  int32_t wy1 = (int32_t)camY + (sy1 - beeScreenCY());

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)cell);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)cell);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)cell);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)cell);

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, salt);
      // probability gate (controls density)
      if ((h & 0x7u) != 0u) continue; // 1/8 cells

      int px = (int)(h & 0xFFu) % cell;
      int py = (int)((h >> 8) & 0xFFu) % cell;

      int32_t wx = cx * cell + px;
      int32_t wy = cy * cell + py;

      int sx = beeScreenCX() + (int)(wx - (int32_t)camX);
      int sy = beeScreenCY() + (int)(wy - (int32_t)camY);

      if (sx < sx0 || sx > sx1 || sy < sy0 || sy > sy1) continue;

      uint16_t c = ((h >> 16) & 1u) ? cA : cB;
      g.drawPixel(sx + ox, sy + oy, c);

      // tiny occasional sparkle cross
      if (((h >> 20) & 0xFu) == 0u) {
        g.drawPixel(sx - 1 + ox, sy + oy, c);
        g.drawPixel(sx + 1 + ox, sy + oy, c);
      }
    }
  }
}

static void drawScreenAnchor(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  // A fixed, screen-space anchor around the bee so the player feels centered.
  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;
  uint16_t c = rgb565(40, 70, 90);

  // faint crosshair ticks
  g.drawFastHLine(cx - 26, cy, 12, c);
  g.drawFastHLine(cx + 15, cy, 12, c);
  g.drawFastVLine(cx, cy - 26, 12, c);
  g.drawFastVLine(cx, cy + 15, 12, c);

  // soft pulse ring
  float t = (float)(nowMs % 1200u) / 1200.0f;
  int r = 22 + (int)(6.0f * sinf(t * 6.2831853f));
  g.drawCircle(cx, cy, r, rgb565(35, 55, 70));
}

// -------------------- HUD + BELT --------------------
static void drawHUDInTile(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  // Only paint the HUD region (0..HUD_H) within this tile.
  int y0 = tileY;
  int y1 = tileY + CANVAS_H - 1;
  if (y0 > (HUD_H - 1) || y1 < 0) return;

  int h = CANVAS_H;
  int topClip = 0;
  int botClip = CANVAS_H;

  if (tileY < 0) {
    topClip = -tileY;
  }
  if (tileY + CANVAS_H > HUD_H) {
    botClip = HUD_H - tileY;
  }

  // Fill only the HUD slice in this tile
  g.fillRect(0, topClip, CANVAS_W, botClip - topClip, COL_HUD_BG);

  // We only render text when tile includes y=0 (top row) to avoid split text.
  if (tileY != 0) return;

  g.setCursor(6 + ox, 6 + oy);
  g.setTextColor(COL_WHITE);
  g.setTextSize(2);
  g.print("POLLEN ");
  g.print(score);

  // Carry
  g.setTextSize(1);
  g.setCursor(6 + ox, 20 + oy);
  g.setTextColor(hasPollen ? COL_YEL : COL_UI_DIM);
  g.print(hasPollen ? "CARRY" : "EMPTY");

  // Boost status (right side)
  int bx = tft.width() - 92;
  g.setCursor(bx + ox, 6 + oy);
  g.setTextColor(boostCharge ? COL_UI_GO : COL_UI_DIM);
  g.print("BOOST ");
  g.print(boostCharge ? "READY" : "--");

  g.setCursor(bx + ox, 16 + oy);
  g.setTextColor(COL_UI_DIM);
  g.print("x3 ");
  g.print((int)depositsTowardBoost);

  // Cooldown indicator (tiny)
  uint32_t now = millis();
  bool cd = (int32_t)(boostCooldownUntilMs - now) > 0;
  g.setCursor(bx + ox, 24 + oy);
  g.setTextColor(cd ? COL_UI_WARN : COL_UI_DIM);
  g.print(cd ? "COOLDN" : "");
}

static void drawBeltHUD(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  // Screen-space conveyor belt in bottom-right.
  // Moved up to make room for survival bar at bottom
  int x0 = tft.width()  - 122;
  int y0 = tft.height() - 56;  // moved up 10 pixels
  int x1 = tft.width()  - 6;
  int y1 = tft.height() - 20;  // moved up 10 pixels

  // Draw only if this tile overlaps belt rect
  int tx0 = -ox; // tileX
  int ty0 = -oy; // tileY
  int tx1 = tx0 + CANVAS_W - 1;
  int ty1 = ty0 + CANVAS_H - 1;
  if (x1 < tx0 || x0 > tx1 || y1 < ty0 || y0 > ty1) return;

  uint16_t panel = rgb565(6, 10, 16);
  uint16_t edge  = rgb565(40, 70, 40);
  g.fillRoundRect(x0 + ox, y0 + oy, (x1 - x0), (y1 - y0), 6, panel);
  g.drawRoundRect(x0 + ox, y0 + oy, (x1 - x0), (y1 - y0), 6, edge);

  // Track
  int ty = y0 + 20;
  int txA = x0 + 14;
  int txB = x1 - 14;
  g.drawLine(txA + ox, ty + oy, txB + ox, ty + oy, rgb565(34, 54, 34));
  g.drawLine(txA + ox, ty + 2 + oy, txB + ox, ty + 2 + oy, rgb565(22, 34, 22));

  // Items flow right
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    uint32_t age = nowMs - beltItems[i].bornMs;
    if (age > BELT_LIFE_MS) continue;

    float t = (float)age / (float)BELT_LIFE_MS;
    t = clampf(t, 0.0f, 1.0f);
    float u = 1.0f - (1.0f - t) * (1.0f - t);

    int x = (int)(txA + u * (float)(txB - txA));
    int y = ty + 1;

    int r = (age < 220) ? 4 : (t < 0.85f ? 3 : 2);
    g.fillCircle(x + ox, y + oy, r, COL_POLLEN);
    g.drawCircle(x + ox, y + oy, r, (t < 0.75f) ? COL_WHITE : COL_YEL);
    g.drawPixel(x + 1 + ox, y - 1 + oy, COL_POLLEN_HI);
  }

  // Label
  g.setTextSize(1);
  g.setTextColor(COL_UI_DIM);
  g.setCursor(x0 + 10 + ox, y0 + 6 + oy);
  g.print("DELIVERIES");
}

static void drawSurvivalBar(Adafruit_GFX &g, int ox, int oy) {
  // Survival timer bar at very bottom of screen
  // Full width bar showing time remaining
  int barW = tft.width() - 12;
  int barH = 6;
  int x0 = 6;
  int y0 = tft.height() - 8;  // 2 pixels from bottom edge

  // Draw only if this tile overlaps bar rect
  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + CANVAS_W - 1;
  int ty1 = ty0 + CANVAS_H - 1;
  if ((x0 + barW) < tx0 || x0 > tx1 || (y0 + barH) < ty0 || y0 > ty1) return;

  // Calculate fill percentage
  float pct = clampf(survivalTimeLeft / SURVIVAL_TIME_MAX, 0.0f, 1.0f);
  int fillW = (int)(pct * (float)barW);

  // Color based on percentage
  uint16_t fillColor;
  if (pct > 0.80f) {
    fillColor = COL_UI_GO;  // Green
  } else if (pct > 0.40f) {
    fillColor = COL_YEL;    // Yellow
  } else if (pct > 0.20f) {
    fillColor = rgb565(255, 140, 0);  // Orange
  } else {
    fillColor = COL_UI_WARN;  // Red
  }

  // Background (empty bar)
  uint16_t bgColor = rgb565(20, 20, 25);
  uint16_t borderColor = rgb565(60, 70, 80);

  g.fillRect(x0 + ox, y0 + oy, barW, barH, bgColor);
  g.drawRect(x0 + ox, y0 + oy, barW, barH, borderColor);

  // Filled portion
  if (fillW > 0) {
    g.fillRect(x0 + ox, y0 + oy, fillW, barH, fillColor);
  }

  // Flash if critical
  if (pct <= 0.20f && (millis() % 400) < 200) {
    g.drawRect(x0 + ox, y0 + oy, barW, barH, COL_UI_WARN);
    g.drawRect(x0 + 1 + ox, y0 + 1 + oy, barW - 2, barH - 2, COL_UI_WARN);
  }
}

// -------------------- GAME OVER SCREEN --------------------
static void drawGameOver(Adafruit_GFX &g, int ox, int oy) {
  // Center panel for game over
  int panelW = 180;
  int panelH = 100;
  int panelX = (tft.width() - panelW) / 2;
  int panelY = (tft.height() - panelH) / 2 - 20;

  // Draw only if this tile overlaps panel
  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + CANVAS_W - 1;
  int ty1 = ty0 + CANVAS_H - 1;
  if ((panelX + panelW) < tx0 || panelX > tx1 || (panelY + panelH) < ty0 || panelY > ty1) return;

  // Panel background
  uint16_t panelBg = rgb565(30, 40, 60);
  uint16_t panelBorder = rgb565(120, 180, 220);
  g.fillRoundRect(panelX + ox, panelY + oy, panelW, panelH, 8, panelBg);
  g.drawRoundRect(panelX + ox, panelY + oy, panelW, panelH, 8, panelBorder);
  g.drawRoundRect(panelX + 1 + ox, panelY + 1 + oy, panelW - 2, panelH - 2, 7, panelBorder);

  // Happy messages (rotate based on score)
  const char* messages[] = {
    "Bee-autiful!",
    "Buzz-tastic!",
    "Sweet Flying!",
    "You're the Bee!",
    "Amazing Work!",
    "Pollen Master!"
  };
  int msgIdx = score % 6;

  // Title
  g.setTextSize(2);
  g.setTextColor(COL_YEL);
  int titleW = strlen(messages[msgIdx]) * 12;
  g.setCursor(panelX + (panelW - titleW) / 2 + ox, panelY + 12 + oy);
  g.print(messages[msgIdx]);

  // Score
  g.setTextSize(3);
  g.setTextColor(COL_WHITE);
  g.setCursor(panelX + panelW / 2 - 30 + ox, panelY + 38 + oy);
  g.print(score);

  // Sub text
  g.setTextSize(1);
  g.setTextColor(COL_UI_DIM);
  g.setCursor(panelX + 28 + ox, panelY + 66 + oy);
  g.print("pollen delivered");

  // Restart prompt (blink)
  if ((millis() % 800) < 400) {
    g.setTextSize(1);
    g.setTextColor(COL_UI_GO);
    g.setCursor(panelX + 30 + ox, panelY + 82 + oy);
    g.print("Press to play again");
  }
}

// -------------------- RADAR DRAW --------------------
static void drawRadarOverlay(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  if (!radarActive) return;
  if ((int32_t)(nowMs - radarUntilMs) >= 0) { radarActive = false; return; }

  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;

  float t = 1.0f - (float)(radarUntilMs - nowMs) / 320.0f; // 0..1
  t = clampf(t, 0.0f, 1.0f);

  // Direction to target in world
  float dx = (float)radarTargetWX - beeWX;
  float dy = (float)radarTargetWY - beeWY;
  float len = sqrtf(dx*dx + dy*dy);

  if (len < 1.0f) len = 1.0f;
  float ux = dx / len;
  float uy = dy / len;

  // Ping ring
  int r0 = 16 + (int)(t * 22.0f);
  uint16_t rc = radarToHive ? COL_HIVE : COL_YEL;
  g.drawCircle(cx, cy, r0, rc);
  g.drawCircle(cx, cy, r0 + 4, COL_WHITE);

  // Arrow
  int ax = cx + (int)(ux * 34.0f);
  int ay = cy + (int)(uy * 34.0f);
  g.drawLine(cx, cy, ax, ay, COL_WHITE);

  // Arrow head
  float px = -uy;
  float py = ux;
  int hx1 = ax - (int)(ux * 9.0f) + (int)(px * 5.0f);
  int hy1 = ay - (int)(uy * 9.0f) + (int)(py * 5.0f);
  int hx2 = ax - (int)(ux * 9.0f) - (int)(px * 5.0f);
  int hy2 = ay - (int)(uy * 9.0f) - (int)(py * 5.0f);
  g.fillTriangle(ax, ay, hx1, hy1, hx2, hy2, rc);

  // Distance tick (tiny)
  g.setTextSize(1);
  g.setTextColor(COL_UI_DIM);
  g.setCursor(cx + 40, cy - 10);
  g.print((int)len);
}

// -------------------- TARGETING --------------------
static bool findNearestFlower(int32_t &outWX, int32_t &outWY) {
  int best = -1;
  int64_t bestD2 = 0;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;

  for (int i = 0; i < FLOWER_N; i++) {
    if (!flowers[i].alive) continue;
    int32_t dx = flowers[i].wx - bx;
    int32_t dy = flowers[i].wy - by;
    int64_t d2 = (int64_t)dx * (int64_t)dx + (int64_t)dy * (int64_t)dy;
    if (best < 0 || d2 < bestD2) { best = i; bestD2 = d2; }
  }

  if (best < 0) return false;
  outWX = flowers[best].wx;
  outWY = flowers[best].wy;
  return true;
}

static void beginRadarPing(uint32_t nowMs) {
  radarActive = true;
  radarUntilMs = nowMs + 320;

  if (hasPollen) {
    radarToHive = true;
    radarTargetWX = 0;
    radarTargetWY = 0;
  } else {
    radarToHive = false;
    int32_t fx, fy;
    if (findNearestFlower(fx, fy)) {
      radarTargetWX = fx;
      radarTargetWY = fy;
    } else {
      // shouldn't happen, but don't crash the UI
      radarTargetWX = 0;
      radarTargetWY = 0;
      radarToHive = true;
    }
  }

  // sound
  if (!soundBusy()) startSound(SND_RADAR, nowMs);
}

// -------------------- INTERACTIONS --------------------
static void tryCollectPollen(uint32_t nowMs) {
  if (hasPollen) return;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;

  for (int i = 0; i < FLOWER_N; i++) {
    Flower &f = flowers[i];
    if (!f.alive) continue;

    int32_t dx = bx - f.wx;
    int32_t dy = by - f.wy;
    int32_t hitR = (int32_t)f.r + 14;
    if ((dx*dx + dy*dy) <= hitR*hitR) {
      hasPollen = true;
      f.alive = 0;
      // respawn elsewhere immediately
      spawnFlowerElsewhere(i);

      // Auto-boost on flower pickup!
      boostActiveUntilMs = nowMs + 500;  // 0.5 second boost
      boostCooldownUntilMs = nowMs + 1200; // 1.2 second cooldown

      // chirp
      if (!soundBusy()) startSound(SND_POLLEN_CHIRP, nowMs);
      return;
    }
  }
}

static void tryStoreAtHive(uint32_t nowMs) {
  if (!hasPollen) return;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;
  const int32_t hiveR = 22;

  if ((bx*bx + by*by) <= (hiveR*hiveR)) {
    hasPollen = false;
    score++;
    spawnBeltItem(nowMs);

    // Reset survival timer on successful delivery!
    survivalTimeLeft = SURVIVAL_TIME_MAX;
  }
}

// -------------------- RENDER FRAME --------------------
static void renderFrame(uint32_t nowMs) {
  // Precompute hive screen pos (may be offscreen)
  int hiveSX, hiveSY;
  worldToScreen(0, 0, hiveSX, hiveSY);

  // Iterate tiles
  for (int tileY = 0; tileY < tft.height(); tileY += CANVAS_H) {
    for (int tileX = 0; tileX < tft.width(); tileX += CANVAS_W) {
      int ox = -tileX;
      int oy = -tileY;

      // base
      canvas.fillScreen(COL_BG0);

      // subtle vignette-ish blocks (cheap contrast)
      // (kept very light so grid + stars do the work)
      if ((tileX ^ tileY) & 0x80) {
        canvas.fillRect(0, 0, CANVAS_W, CANVAS_H, COL_BG1);
      }

      // background layers
      drawStarLayer(canvas, tileX, tileY, ox, oy, 0.25f, 48,  COL_STAR2, COL_STAR3, 0xA11CEu);
      drawStarLayer(canvas, tileX, tileY, ox, oy, 0.55f, 36,  COL_STAR,  COL_STAR2, 0xBEEFu);
      drawWorldGrid(canvas, tileX, tileY, ox, oy);
      drawGravityZones(canvas, ox, oy);
      drawScreenAnchor(canvas, ox, oy, nowMs);

      // hive (only if near screen)
      if (hiveSX >= -40 && hiveSX <= tft.width() + 40 && hiveSY >= HUD_H - 40 && hiveSY <= tft.height() + 40) {
        drawHive(canvas, hiveSX + ox, hiveSY + oy);
      }

      // flowers
      for (int i = 0; i < FLOWER_N; i++) {
        if (!flowers[i].alive) continue;
        int sx, sy;
        worldToScreen(flowers[i].wx, flowers[i].wy, sx, sy);
        if (sx < -30 || sx > tft.width() + 30 || sy < HUD_H - 30 || sy > tft.height() + 30) continue;
        drawFlower(canvas, sx + ox, sy + oy, flowers[i]);
      }

      // boost trail particles (behind bee)
      drawTrailParticles(canvas, ox, oy, nowMs);

      // bee shadow + bee (always on screen)
      int bcX = beeScreenCX();
      int bcY = beeScreenCY();
      drawBeeShadow(canvas, bcX + ox, bcY + oy);
      drawBee(canvas, bcX + ox, bcY + oy);

      // radar overlay (screen-space)
      drawRadarOverlay(canvas, ox, oy, nowMs);

      // belt HUD (screen-space)
      drawBeltHUD(canvas, ox, oy, nowMs);

      // survival bar (screen-space)
      drawSurvivalBar(canvas, ox, oy);

      // HUD (top)
      drawHUDInTile(canvas, tileX, tileY, ox, oy);

      // Game over screen (overlays everything)
      if (isGameOver) {
        drawGameOver(canvas, ox, oy);
      }

      // blit tile
      tft.drawRGBBitmap(tileX, tileY, canvas.getBuffer(), CANVAS_W, CANVAS_H);
    }
  }
}

// -------------------- SETUP --------------------
void setup() {
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  pinMode(PIN_BUZZ, OUTPUT);
  digitalWrite(PIN_BUZZ, LOW);

  SPI.setSCK(PIN_SCK);
  SPI.setTX(PIN_MOSI);
  SPI.begin();

  tft.init(240, 320);
  tft.setRotation(1);

  // Seed RNG from ADC noise + micros
  rngState ^= (uint32_t)analogRead(PIN_JOY_VRX) << 16;
  rngState ^= (uint32_t)analogRead(PIN_JOY_VRY) << 1;
  rngState ^= (uint32_t)micros();

  tft.fillScreen(COL_BG0);

  calibrateJoystick();
  joyMinY = joyCenterY;
  joyMaxY = joyCenterY;

  for (int i = 0; i < BELT_ITEM_N; i++) beltItems[i].alive = 0;
  for (int i = 0; i < TRAIL_MAX; i++) trail[i].alive = 0;

  initFlowers();

  // Start at hive
  beeWX = 0.0f;
  beeWY = 0.0f;
  beeVX = 0.0f;
  beeVY = 0.0f;

  hasPollen = false;
  score = 0;

  depositsTowardBoost = 0;
  boostCharge = 0;
  boostActiveUntilMs = 0;
  boostCooldownUntilMs = 0;

  radarActive = false;
  radarUntilMs = 0;

  renderFrame(millis());
}

// -------------------- LOOP --------------------
void loop() {
  static uint32_t lastMs = millis();
  uint32_t now = millis();
  uint32_t dtMs = now - lastMs;
  if (dtMs > 60) dtMs = 60;
  lastMs = now;
  float dt = (float)dtMs / 1000.0f;

  // Maintain observed extremes for Y (helps asymmetry)
  int rawX = readJoyX();
  int rawY = readJoyY();
  if (rawY < joyMinY) joyMinY = rawY;
  if (rawY > joyMaxY) joyMaxY = rawY;

  // Deadzone
  const int dead = 35;
  int dx = applyDeadzone(rawX, joyCenterX, dead);
  int dy = applyDeadzone(rawY, joyCenterY, dead);

  // Normalize X
  float nx = -(float)clampi(dx, -512, 512) / 512.0f;

  // Normalize Y with auto-cal asymmetry
  int upSpan   = joyCenterY - joyMinY;
  int downSpan = joyMaxY - joyCenterY;
  if (upSpan < 1) upSpan = 1;
  if (downSpan < 1) downSpan = 1;

  float nyRaw;
  const float DOWN_BOOST = 1.20f;
  if (dy >= 0) {
    nyRaw = (float)dy / (float)downSpan;
    nyRaw *= DOWN_BOOST;
  } else {
    nyRaw = (float)dy / (float)upSpan;
  }
  nyRaw = clampf(nyRaw, -1.0f, 1.0f);
  float ny = -nyRaw;

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

  // Determine if boost is active
  bool boosting = (int32_t)(now - boostActiveUntilMs) < 0;
  bool boostCD  = (int32_t)(boostCooldownUntilMs - now) > 0;

  // Base movement
  float maxSpeed = boosting ? 520.0f : 240.0f; // world units/s
  float accel    = boosting ? 12.0f : 8.0f;    // responsiveness

  // Desired velocity (world-space)
  float desVX = nx * maxSpeed;
  float desVY = ny * maxSpeed;

  // If stick is centered, ease to 0
  if (dx == 0) desVX = 0.0f;
  if (dy == 0) desVY = 0.0f;

  // Smooth approach
  float k = accel * dt;
  if (k > 1.0f) k = 1.0f;
  beeVX += (desVX - beeVX) * k;
  beeVY += (desVY - beeVY) * k;

  // Gentle gravity toward hive - only when far from center
  // This is a HELPER, not forced motion - bee can rest peacefully near hive
  if (!isGameOver) {
    float distToHive = sqrtf(beeWX * beeWX + beeWY * beeWY);

    // Only apply gravity when beyond comfortable play area (150 pixels)
    const float GRAVITY_DEADBAND = 150.0f;
    if (distToHive > GRAVITY_DEADBAND) {
      // Gentle nudge back toward center
      float dirX = -beeWX / distToHive;  // unit vector toward center
      float dirY = -beeWY / distToHive;

      // Weak force - just a gentle reminder, not a prison
      float gravityForce = 15.0f;

      beeVX += dirX * gravityForce * dt;
      beeVY += dirY * gravityForce * dt;
    }
  }

  // Integrate
  beeWX += beeVX * dt;
  beeWY += beeVY * dt;

  // Wing animation driven by speed
  float sp = fabsf(beeVX) + fabsf(beeVY);
  float spN = clampf(sp / 520.0f, 0.0f, 1.0f);
  wingSpeed = spN;
  float hz = 3.0f + 14.0f * wingSpeed;
  wingPhase += 2.0f * 3.1415926f * hz * dt;
  if (wingPhase > 1000.0f) wingPhase -= 1000.0f;

  // Boost trail VFX - much more prominent!
  if (boosting && wingSpeed > 0.2f) {
    // Spawn trail particles every ~20ms for denser trail
    static uint32_t lastTrailMs = 0;
    if ((uint32_t)(now - lastTrailMs) > 20) {
      // Spawn 2 particles per frame for extra density
      spawnTrailParticle(beeWX, beeWY, now);
      spawnTrailParticle(beeWX - beeVX * 0.02f, beeWY - beeVY * 0.02f, now); // slightly offset
      lastTrailMs = now;
    }
  }
  updateTrailParticles(now);

  // Survival timer countdown
  if (!isGameOver) {
    survivalTimeLeft -= dt;
    if (survivalTimeLeft <= 0.0f) {
      survivalTimeLeft = 0.0f;
      isGameOver = true;
      gameOverMs = now;
      // Stop all sounds immediately!
      noTone(PIN_BUZZ);
      sndMode = SND_IDLE;
    }
  }

  // --- Button handling ---
  bool b = joyPressedRaw();
  bool edgeDown = (b && !btnPrev);
  btnPrev = b;

  // Game over restart on button press
  if (isGameOver && edgeDown) {
    // Reset game
    beeWX = 0.0f;
    beeWY = 0.0f;
    beeVX = 0.0f;
    beeVY = 0.0f;
    survivalTimeLeft = SURVIVAL_TIME_MAX;
    isGameOver = false;
    hasPollen = false;
    score = 0;
    initFlowers();
    // Clear trail
    for (int i = 0; i < TRAIL_MAX; i++) trail[i].alive = 0;
    // Continue to render the reset state
  }

  // Don't process normal game input during game over
  // But DO continue to render!
  if (!isGameOver) {

    // "stick pushed" test for click => boost (if available)
    bool stickPushed = (fabsf(nx) > 0.20f || fabsf(ny) > 0.20f);

    if (edgeDown) {
      // Click always gives some percussive feedback.
      if (!soundBusy()) startSound(SND_CLICK, now);

      if (stickPushed && boostCharge > 0 && !boostCD) {
        // Trigger boost
        boostCharge = 0;
        boostActiveUntilMs = now + 700;
        boostCooldownUntilMs = now + 3500;

        // impulse in current direction
        float dirX = nx;
        float dirY = ny;
        float dlen = sqrtf(dirX*dirX + dirY*dirY);
        if (dlen < 0.001f) { dirX = 1.0f; dirY = 0.0f; dlen = 1.0f; }
        dirX /= dlen; dirY /= dlen;
        beeVX += dirX * 220.0f;
        beeVY += dirY * 220.0f;

        // also do a radar ping toward the current target (feels good)
        beginRadarPing(now);
      } else {
        // Radar ping
        beginRadarPing(now);
      }
    }

    // Interactions
    updateBeltLifetimes(now);
    tryCollectPollen(now);
    tryStoreAtHive(now);

    // Sound: ambient wing buzz (only when no event sounds active) - stops when bee stops!
    if (!soundBusy()) {
      if (wingSpeed > 0.08f) {
        int freq = 90 + (int)(wingSpeed * 360.0f);
        freq = clampi(freq, 80, 520);
        tone(PIN_BUZZ, freq);
      } else {
        noTone(PIN_BUZZ);
      }
    }
    updateSound(now);
  }  // End of normal game input processing

  // Render at a stable cadence
  static uint32_t lastRenderMs = 0;
  if ((uint32_t)(now - lastRenderMs) >= 40) { // ~25 fps
    lastRenderMs = now;
    renderFrame(now);
  }

  delay(2);
}
