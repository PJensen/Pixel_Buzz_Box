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

// -------------------- FORWARD DECLARATIONS --------------------
static inline int clampi(int v, int lo, int hi);
static inline float clampf(float v, float lo, float hi);
static inline uint8_t clampu8(int v);
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
static inline uint32_t xrnd();
static inline int irand(int lo, int hi);
static inline uint32_t hash32(uint32_t x);

static int readJoyX();
static int readJoyY();
static bool joyPressedRaw();
static int applyDeadzone(int v, int center, int dz);
static void calibrateJoystick();

static inline int beeScreenCX();
static inline int beeScreenCY();
static inline void triggerCameraShake(uint32_t nowMs, float magnitude, uint32_t durationMs);

static void initFlowerStyle(Flower &f);
static void spawnFlowerAt(int i, int32_t wx, int32_t wy);
static void spawnFlowerNearOrigin(int i);
static void spawnFlowerElsewhere(int i);
static void initFlowers();

static void spawnBeltItem(uint32_t nowMs);
static void updateBeltLifetimes(uint32_t nowMs);

static void startSound(SndMode m, uint32_t nowMs);
static void updateSound(uint32_t nowMs);
static bool soundBusy();

static inline void worldToScreen(int32_t wx, int32_t wy, int &sx, int &sy);
static inline void worldToScreenF(float wx, float wy, int &sx, int &sy);
static inline uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt);

static void drawBoostAura(Adafruit_GFX &g, int x, int y, uint32_t nowMs);
static void drawPollenSparkles(Adafruit_GFX &g, int x, int y, uint32_t nowMs);
static void drawBeeShadow(Adafruit_GFX &g, int x, int y);
static void spawnTrailParticle(float wx, float wy, float speedN, uint32_t nowMs);
static void updateTrailParticles(uint32_t nowMs);
static void drawTrailParticles(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
static void drawPollenOrbit(Adafruit_GFX &g, int x, int y);
static void drawBee(Adafruit_GFX &g, int x, int y);
static void drawHive(Adafruit_GFX &g, int x, int y);
static void drawHivePulse(Adafruit_GFX &g, int x, int y, uint32_t nowMs);
static void spawnScorePopup(uint32_t nowMs, uint8_t value, int sx, int sy);
static void updateScorePopups(uint32_t nowMs);
static void drawScorePopups(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);

static void drawStarLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, float parallax, int count, uint16_t c1, uint16_t c2, uint32_t salt);
static void drawNebulaLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);
static void drawWorldGrid(Adafruit_GFX &g, int ox, int oy);
static void drawBoundaryZone(Adafruit_GFX &g, int ox, int oy);
static void drawScreenAnchor(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
static void drawFlower(Adafruit_GFX &g, int x, int y, const Flower &f, uint32_t nowMs, uint32_t bornMs);

static void drawBeltHUD(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
static void drawHUDInTile(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy);
static void drawSurvivalBar(Adafruit_GFX &g, int ox, int oy);
static void drawGameOver(Adafruit_GFX &g, int ox, int oy);
static void drawRadarOverlay(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);

static bool findNearestFlower(int32_t &outWX, int32_t &outWY);
static void beginRadarPing(uint32_t nowMs);
static void tryCollectPollen(uint32_t nowMs);
static void tryStoreAtHive(uint32_t nowMs);
static void beginUnload(uint32_t nowMs);
static void updateUnload(uint32_t nowMs);
static bool anyTrailAlive();
static bool anyBeltAlive();
static bool anyScorePopupAlive();
static void renderFrame(uint32_t nowMs);

void setup();
void loop();

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
static inline uint8_t clampu8(int v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)v;
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
static const uint8_t MAX_POLLEN_CARRY = 8;
static const uint32_t UNLOAD_TICK_MS = 100;
static const uint16_t UNLOAD_CHIRP_BASE = 580;
static const uint16_t UNLOAD_CHIRP_STEP = 70;
static const uint16_t UNLOAD_CHIRP_MS = 55;

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
static uint32_t flowerBornMs[FLOWER_N];

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
  float speedN;
};
static const int TRAIL_MAX = 24; // doubled for more impact
static TrailParticle trail[TRAIL_MAX];
static int trailNextIdx = 0;

// -------------------- SURVIVAL TIMER --------------------
static const float SURVIVAL_TIME_MAX = 30.0f; // seconds
static float survivalTimeLeft = SURVIVAL_TIME_MAX;
static bool isGameOver = false;
static uint32_t gameOverMs = 0;

// -------------------- HIVE DROP-OFF VFX --------------------
static uint32_t hivePulseUntilMs = 0;
static const uint32_t HIVE_PULSE_MS = 520;

// -------------------- SCORE POPUP VFX --------------------
struct ScorePopup {
  uint32_t bornMs;
  int16_t baseSX;
  int16_t baseSY;
  int8_t driftX;
  uint8_t value;
  uint8_t alive;
};
static const int SCORE_POPUP_N = 6;
static const uint32_t SCORE_POPUP_LIFE_MS = 1100;
static ScorePopup scorePopups[SCORE_POPUP_N];

// -------------------- WORLD CONSTRAINTS --------------------
// Spring-based exploration area - joystick maps to target within this radius
static const float BOUNDARY_COMFORTABLE = 180.0f; // max exploration radius from hive

// -------------------- MOVEMENT PHYSICS --------------------
// Spring constants for smooth motion
static const float SPRING_K_NORMAL = 32.0f;       // spring stiffness (normal)
static const float SPRING_K_BOOST = 48.0f;        // spring stiffness (boosting)
static const float DAMPING_NORMAL = 12.0f;        // damping coefficient (normal)
static const float DAMPING_BOOST = 16.0f;         // damping coefficient (boosting)

// Boost mechanics
static const float BOOST_IMPULSE = 150.0f;        // velocity boost on manual trigger (was 220)
static const uint32_t BOOST_DURATION_AUTO = 500;  // auto-boost duration on flower pickup (ms)
static const uint32_t BOOST_DURATION_MANUAL = 700; // manual boost duration (ms)
static const uint32_t BOOST_COOLDOWN_AUTO = 5200; // cooldown after auto-boost (ms)
static const uint32_t BOOST_COOLDOWN_MANUAL = 3500; // cooldown after manual boost (ms)

// Wing animation
static const float WING_SPEED_DIVISOR = 520.0f;   // for normalizing velocity to wing speed

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

static uint8_t pollenCount = 0;
static uint16_t score = 0;
static bool isUnloading = false;
static uint8_t unloadRemaining = 0;
static uint8_t unloadTotal = 0;
static uint32_t unloadNextMs = 0;

// Bee render center (screen-space)
static float cameraZoom = 1.0f;
static float cameraShakeX = 0.0f;
static float cameraShakeY = 0.0f;
static uint32_t cameraShakeUntilMs = 0;
static uint32_t cameraShakeDurationMs = 0;
static float cameraShakeMagnitude = 0.0f;
static inline int beeScreenCX() { return tft.width() / 2 + (int)cameraShakeX; }
static inline int beeScreenCY() { return (tft.height() + HUD_H) / 2 + (int)cameraShakeY; } // center of playfield

static inline void triggerCameraShake(uint32_t nowMs, float magnitude, uint32_t durationMs) {
  cameraShakeUntilMs = nowMs + durationMs;
  cameraShakeDurationMs = durationMs;
  cameraShakeMagnitude = magnitude;
}

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
  flowerBornMs[i] = millis();
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
  // Respawn away from current bee position but within bounded world.
  for (int tries = 0; tries < 80; tries++) {
    // Random position within comfortable exploration zone
    int32_t r = (int32_t)irand(60, (int)BOUNDARY_COMFORTABLE - 20);
    int32_t a = (int32_t)irand(0, 359);
    float ang = (float)a * 0.0174532925f;
    int32_t wx = (int32_t)(cosf(ang) * (float)r);
    int32_t wy = (int32_t)(sinf(ang) * (float)r);

    // Keep within comfortable zone (flowers always easily reachable)
    if ((wx*wx + wy*wy) > (int32_t)(BOUNDARY_COMFORTABLE * BOUNDARY_COMFORTABLE)) continue;

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
  float dx = (float)wx - beeWX;
  float dy = (float)wy - beeWY;
  sx = beeScreenCX() + (int)(dx * cameraZoom);
  sy = beeScreenCY() + (int)(dy * cameraZoom);
}

static inline void worldToScreenF(float wx, float wy, int &sx, int &sy) {
  float dx = wx - beeWX;
  float dy = wy - beeWY;
  sx = beeScreenCX() + (int)(dx * cameraZoom);
  sy = beeScreenCY() + (int)(dy * cameraZoom);
}

static inline uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt) {
  return hash32((uint32_t)cx * 73856093u ^ (uint32_t)cy * 19349663u ^ salt);
}

// -------------------- DRAWING PRIMITIVES --------------------
static void drawBoostAura(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  float t = (float)(nowMs % 900u) / 900.0f;
  int r = 14 + (int)(4.0f * sinf(t * 6.2831853f));
  uint16_t c1 = rgb565(255, 210, 60);
  uint16_t c2 = rgb565(255, 240, 140);
  g.drawCircle(x, y, r, c1);
  g.drawCircle(x, y, r + 2, c2);
  g.drawCircle(x, y, r - 2, c1);
}

static void drawPollenSparkles(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  if (pollenCount == 0) return;
  int sparkles = clampi(4 + (int)pollenCount, 4, 12);
  for (int i = 0; i < sparkles; i++) {
    uint32_t h = hash32(((uint32_t)nowMs >> 4) + (uint32_t)i * 977u);
    int dx = (int)(h & 0x1Fu) - 15;
    int dy = (int)((h >> 5) & 0x1Fu) - 15;
    if ((dx * dx + dy * dy) > 160) continue;
    uint16_t c = (i & 1) ? COL_POLLEN_HI : COL_WHITE;
    if (((h >> 11) & 1u) == 0u) g.drawPixel(x + dx, y + dy, c);
  }
}

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
static void spawnTrailParticle(float wx, float wy, float speedN, uint32_t nowMs) {
  trail[trailNextIdx].wx = wx;
  trail[trailNextIdx].wy = wy;
  trail[trailNextIdx].bornMs = nowMs;
  trail[trailNextIdx].alive = 1;
  trail[trailNextIdx].variant = (uint8_t)(xrnd() % 3);
  trail[trailNextIdx].speedN = clampf(speedN, 0.0f, 1.0f);
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
    int sx, sy;
    worldToScreenF(trail[i].wx, trail[i].wy, sx, sy);

    // Fade out based on age
    float t = (float)age / (float)TRAIL_LIFE_MS;
    float alpha = 1.0f - t * t; // quadratic fade looks better

    float speedT = trail[i].speedN;
    uint8_t baseR = (uint8_t)(255 - (int)(115.0f * speedT));
    uint8_t baseG = (uint8_t)(220 - (int)(120.0f * speedT));
    uint8_t baseB = (uint8_t)(60 + (int)(195.0f * speedT));

    uint8_t r = (uint8_t)(baseR * alpha);
    uint8_t g_val = (uint8_t)(baseG * alpha);
    uint8_t b = (uint8_t)(baseB * alpha);

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

static void drawPollenOrbit(Adafruit_GFX &g, int x, int y) {
  if (pollenCount == 0) return;
  int count = pollenCount;
  float base = wingPhase * 1.4f;
  int ring = 10 + (count / 3) * 2;
  int ringY = ring - 2;

  for (int i = 0; i < count; i++) {
    float ang = base + (6.2831853f * (float)i) / (float)count;
    int px = x + (int)(cosf(ang) * (float)ring);
    int py = y + 5 + (int)(sinf(ang) * (float)ringY);
    g.fillCircle(px, py, 2, COL_POLLEN);
    g.drawPixel(px + 1, py - 1, COL_POLLEN_HI);
  }

  if (pollenCount >= MAX_POLLEN_CARRY) {
    g.drawCircle(x, y + 2, ring + 4, COL_POLLEN_HI);
  }
}

static void drawBee(Adafruit_GFX &g, int x, int y) {
  float load = clampf((float)pollenCount / (float)MAX_POLLEN_CARRY, 0.0f, 1.0f);
  uint8_t bodyR = 255;
  uint8_t bodyG = (uint8_t)(220 + (int)(25.0f * load));
  uint8_t bodyB = (uint8_t)(40 + (int)(120.0f * load));
  uint16_t body = rgb565(bodyR, bodyG, bodyB);

  float s = sinf(wingPhase);
  int flap = (int)(s * (2 + (int)(3 * wingSpeed)));
  int wH   = 4 + (int)(2 * (0.5f + 0.5f * s));
  int wW   = 7 + (int)(2 * wingSpeed);
  uint8_t wr = clampu8(170 + (int)(55.0f * (0.5f + 0.5f * s)));
  uint8_t wg = clampu8(215 + (int)(35.0f * (0.5f + 0.5f * s)));
  uint8_t wb = 255;
  uint16_t wingCol = rgb565(wr, wg, wb);

  g.fillEllipse(x - 6, y - 9 + flap, wW, wH, wingCol);
  g.fillEllipse(x + 2, y - 10 - flap/2, wW, wH, wingCol);
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

  drawPollenOrbit(g, x, y);
}

static void drawHive(Adafruit_GFX &g, int x, int y) {
  g.drawCircle(x, y, 12, COL_HIVE);
  g.drawCircle(x, y,  7, COL_HIVE);
  g.drawCircle(x, y,  2, COL_HIVE);
}

static void drawHivePulse(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  if ((int32_t)(nowMs - hivePulseUntilMs) >= 0) return;
  float t = 1.0f - (float)(hivePulseUntilMs - nowMs) / (float)HIVE_PULSE_MS;
  t = clampf(t, 0.0f, 1.0f);
  int r = 10 + (int)(t * 26.0f);
  uint16_t c1 = rgb565(140, 220, 150);
  uint16_t c2 = rgb565(220, 255, 230);
  g.drawCircle(x, y, r, c1);
  g.drawCircle(x, y, r + 4, c2);
  if ((nowMs & 0x3u) == 0u) {
    g.drawCircle(x, y, r - 2, COL_WHITE);
  }
}

static void spawnScorePopup(uint32_t nowMs, uint8_t value, int sx, int sy) {
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
  scorePopups[idx].driftX = (int8_t)irand(-10, 10);
}

static void updateScorePopups(uint32_t nowMs) {
  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) continue;
    if ((uint32_t)(nowMs - scorePopups[i].bornMs) > SCORE_POPUP_LIFE_MS) {
      scorePopups[i].alive = 0;
    }
  }
}

static void drawScorePopups(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  char buf[8];
  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) continue;
    uint32_t age = nowMs - scorePopups[i].bornMs;
    if (age > SCORE_POPUP_LIFE_MS) continue;

    float t = (float)age / (float)SCORE_POPUP_LIFE_MS;
    t = clampf(t, 0.0f, 1.0f);

    // Ease-out float with slight sideways sway
    float u = 1.0f - (1.0f - t) * (1.0f - t);
    int floatY = (int)(28.0f * u);
    int sway = (int)(sinf((float)age * 0.018f + (float)scorePopups[i].driftX) * 2.0f);

    int cx = (int)scorePopups[i].baseSX + scorePopups[i].driftX + sway;
    int cy = (int)scorePopups[i].baseSY - 6 - floatY;

    int size;
    if (t < 0.18f) size = 1;
    else if (t < 0.72f) size = 2;
    else size = 3; // pulse at the end

    snprintf(buf, sizeof(buf), "+%d", (int)scorePopups[i].value);
    int len = (int)strlen(buf);
    int textW = len * 6 * size;
    int textH = 8 * size;
    int x0 = cx - textW / 2;
    int y0 = cy - textH / 2;
    int x1 = x0 + textW - 1;
    int y1 = y0 + textH - 1;

    // Draw only if this tile overlaps text rect
    int tx0 = -ox;
    int ty0 = -oy;
    int tx1 = tx0 + CANVAS_W - 1;
    int ty1 = ty0 + CANVAS_H - 1;
    if (x1 < tx0 || x0 > tx1 || y1 < ty0 || y0 > ty1) continue;

    g.setTextWrap(false);
    g.setTextSize(size);

    // Shadow for readability
    g.setTextColor(COL_SHADOW);
    g.setCursor(x0 + 1 + ox, y0 + 1 + oy);
    g.print(buf);

    // Main color (yellow)
    uint16_t mainCol = (t > 0.75f) ? COL_POLLEN_HI : COL_YEL;
    g.setTextColor(mainCol);
    g.setCursor(x0 + ox, y0 + oy);
    g.print(buf);

    // Pulse glow near the end
    if (t > 0.72f) {
      g.setTextColor(COL_WHITE);
      g.setCursor(x0 - 1 + ox, y0 + oy);
      g.print(buf);
      g.setCursor(x0 + 1 + ox, y0 - 1 + oy);
      g.print(buf);
    }

    g.setTextWrap(true);
  }
}

static void drawFlower(Adafruit_GFX &g, int x, int y, const Flower &f, uint32_t nowMs, uint32_t bornMs) {
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

  // quick bloom pop on spawn
  uint32_t age = nowMs - bornMs;
  if (age < 420) {
    float t = (float)age / 420.0f;
    t = clampf(t, 0.0f, 1.0f);
    int growR = 1 + (int)(t * (float)(r + 2));
    uint16_t bloomCore = rgb565(255, 245, 200);
    g.fillCircle(x, y, growR, bloomCore);
    g.drawCircle(x, y, growR + 2, COL_WHITE);

    float ringT = 1.0f - t;
    int br = r + 8 + (int)(ringT * 10.0f);
    uint16_t bc = rgb565(255, 235, 200);
    uint16_t bc2 = rgb565(255, 250, 230);
    g.drawCircle(x, y, br, bc);
    g.drawCircle(x, y, br + 4, bc2);
    if ((age & 0x3u) == 0u) {
      g.drawCircle(x, y, br - 2, COL_WHITE);
      g.drawCircle(x, y, br + 1, COL_POLLEN_HI);
    }
    if ((age & 0x7u) == 0u) {
      int sparkR = br + 6;
      g.drawPixel(x + sparkR, y, bc2);
      g.drawPixel(x - sparkR, y, bc2);
      g.drawPixel(x, y + sparkR, bc2);
      g.drawPixel(x, y - sparkR, bc2);
    }
  }
}

// -------------------- BACKGROUND --------------------
static void drawBoundaryZone(Adafruit_GFX &g, int ox, int oy) {
  // Show exploration boundary - helps player understand the play area
  int hiveX = beeScreenCX() + ox;
  int hiveY = beeScreenCY() + oy;

  float distFromCenter = sqrtf(beeWX * beeWX + beeWY * beeWY);

  // Only show boundary when getting close to edge
  if (distFromCenter > BOUNDARY_COMFORTABLE * 0.6f) {
    // Subtle boundary ring showing max joystick reach
    uint16_t boundaryColor = rgb565(50, 70, 90);
    g.drawCircle(hiveX, hiveY, (int)(BOUNDARY_COMFORTABLE * cameraZoom), boundaryColor);
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
  int32_t wx0 = (int32_t)(beeWX + (float)(sx0 - beeScreenCX()) / cameraZoom);
  int32_t wy0 = (int32_t)(beeWY + (float)(sy0 - beeScreenCY()) / cameraZoom);
  int32_t wx1 = (int32_t)(beeWX + (float)(sx1 - beeScreenCX()) / cameraZoom);
  int32_t wy1 = (int32_t)(beeWY + (float)(sy1 - beeScreenCY()) / cameraZoom);

  // vertical lines
  int32_t gx0 = (int32_t)floorf((float)wx0 / (float)GRID2) * GRID2;
  for (int32_t gx = gx0; gx <= wx1; gx += GRID2) {
    int sx = beeScreenCX() + (int)(((float)gx - beeWX) * cameraZoom);
    if (sx < sx0 || sx > sx1) continue;
    bool major = ((gx % GRID) == 0);
    uint16_t c = major ? COL_GRID : COL_GRID2;
    g.drawFastVLine(sx + ox, sy0 + oy, CANVAS_H, c);
  }

  // horizontal lines
  int32_t gy0 = (int32_t)floorf((float)wy0 / (float)GRID2) * GRID2;
  for (int32_t gy = gy0; gy <= wy1; gy += GRID2) {
    int sy = beeScreenCY() + (int)(((float)gy - beeWY) * cameraZoom);
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
  int32_t wx0 = (int32_t)(camX + (float)(sx0 - beeScreenCX()) / cameraZoom);
  int32_t wy0 = (int32_t)(camY + (float)(sy0 - beeScreenCY()) / cameraZoom);
  int32_t wx1 = (int32_t)(camX + (float)(sx1 - beeScreenCX()) / cameraZoom);
  int32_t wy1 = (int32_t)(camY + (float)(sy1 - beeScreenCY()) / cameraZoom);

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

      int sx = beeScreenCX() + (int)(((float)wx - camX) * cameraZoom);
      int sy = beeScreenCY() + (int)(((float)wy - camY) * cameraZoom);

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

static void drawNebulaLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  float driftX = sinf((float)nowMs * 0.00012f) * 22.0f;
  float driftY = cosf((float)nowMs * 0.00010f) * 18.0f;
  float camX = beeWX * 0.35f + driftX;
  float camY = beeWY * 0.35f + driftY;
  const int cell = 64;

  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + CANVAS_W - 1;
  int sy1 = tileY + CANVAS_H - 1;

  int32_t wx0 = (int32_t)(camX + (float)(sx0 - beeScreenCX()) / cameraZoom);
  int32_t wy0 = (int32_t)(camY + (float)(sy0 - beeScreenCY()) / cameraZoom);
  int32_t wx1 = (int32_t)(camX + (float)(sx1 - beeScreenCX()) / cameraZoom);
  int32_t wy1 = (int32_t)(camY + (float)(sy1 - beeScreenCY()) / cameraZoom);

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)cell);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)cell);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)cell);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)cell);

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, 0xD1B00Bu);
      if ((h & 0x0Fu) != 0u) continue; // 1/16 cells

      int px = (int)(h & 0x3Fu);
      int py = (int)((h >> 6) & 0x3Fu);
      int32_t wx = cx * cell + px;
      int32_t wy = cy * cell + py;

      int sx = beeScreenCX() + (int)(((float)wx - camX) * cameraZoom);
      int sy = beeScreenCY() + (int)(((float)wy - camY) * cameraZoom);
      if (sx < sx0 || sx > sx1 || sy < sy0 || sy > sy1) continue;

      uint8_t r = clampu8(40 + (int)((h >> 12) & 0x1Fu));
      uint8_t gcol = clampu8(40 + (int)((h >> 17) & 0x1Fu));
      uint8_t b = clampu8(70 + (int)((h >> 22) & 0x3Fu));
      uint16_t c = rgb565(r, gcol, b);

      g.drawPixel(sx + ox, sy + oy, c);
      if ((h & 0x100u) != 0u) {
        g.drawPixel(sx + 1 + ox, sy + oy, c);
        g.drawPixel(sx + ox, sy + 1 + oy, c);
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
  g.setTextColor(pollenCount ? COL_YEL : COL_UI_DIM);
  if (pollenCount) {
    g.print("CARRY ");
    g.print((int)pollenCount);
  } else {
    g.print("EMPTY ");
    g.print("0");
  }
  g.print("/");
  g.print((int)MAX_POLLEN_CARRY);

  // Pollen rack (2x4 dots)
  int rackX = 84 + ox;
  int rackY = 20 + oy;
  int idx = 0;
  for (int ry = 0; ry < 2; ry++) {
    for (int rx = 0; rx < 4; rx++) {
      if (idx >= MAX_POLLEN_CARRY) break;
      int cx = rackX + rx * 6;
      int cy = rackY + ry * 6;
      uint16_t c = (idx < pollenCount) ? COL_POLLEN : COL_UI_DIM;
      g.fillCircle(cx, cy, 2, c);
      if (idx < pollenCount) g.drawPixel(cx + 1, cy - 1, COL_POLLEN_HI);
      idx++;
    }
  }

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
  int panelW = 200;
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

  g.setTextWrap(false);

  // Title
  g.setTextSize(2);
  g.setTextColor(COL_YEL);
  int titleW = strlen(messages[msgIdx]) * 12;
  int titleX = panelX + (panelW - titleW) / 2;
  g.setCursor(titleX + ox, panelY + 12 + oy);
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

  g.setTextWrap(true);
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

  // Ping rings
  int r0 = 14 + (int)(t * 26.0f);
  uint16_t rc = radarToHive ? COL_HIVE : COL_YEL;
  g.drawCircle(cx, cy, r0, rc);
  g.drawCircle(cx, cy, r0 + 4, COL_WHITE);
  if (t > 0.35f) {
    int r1 = 10 + (int)((t - 0.35f) * 30.0f);
    g.drawCircle(cx, cy, r1, COL_UI_DIM);
  }

  // Dashed vector + distance ticks
  int ax = cx + (int)(ux * 36.0f);
  int ay = cy + (int)(uy * 36.0f);
  for (int i = 6; i < 36; i += 6) {
    int sx = cx + (int)(ux * (float)i);
    int sy = cy + (int)(uy * (float)i);
    g.drawPixel(sx, sy, COL_WHITE);
  }
  for (int i = 12; i <= 36; i += 8) {
    int tx = cx + (int)(ux * (float)i);
    int ty = cy + (int)(uy * (float)i);
    int px = (int)(-uy * 2.0f);
    int py = (int)(ux * 2.0f);
    g.drawLine(tx - px, ty - py, tx + px, ty + py, COL_UI_DIM);
  }

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

  if (pollenCount > 0) {
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
  if (pollenCount >= MAX_POLLEN_CARRY) return;
  if (isUnloading) return;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;

  for (int i = 0; i < FLOWER_N; i++) {
    Flower &f = flowers[i];
    if (!f.alive) continue;

    int32_t dx = bx - f.wx;
    int32_t dy = by - f.wy;
    int32_t hitR = (int32_t)f.r + 14;
    if ((dx*dx + dy*dy) <= hitR*hitR) {
      pollenCount++;
      f.alive = 0;
      // respawn elsewhere immediately
      spawnFlowerElsewhere(i);

      // Auto-boost on flower pickup!
      boostActiveUntilMs = nowMs + BOOST_DURATION_AUTO;
      boostCooldownUntilMs = nowMs + BOOST_COOLDOWN_AUTO;

      // chirp
      if (!soundBusy()) startSound(SND_POLLEN_CHIRP, nowMs);
      return;
    }
  }
}

static void beginUnload(uint32_t nowMs) {
  if (isUnloading || pollenCount == 0) return;
  isUnloading = true;
  unloadRemaining = pollenCount;
  unloadTotal = pollenCount;
  unloadNextMs = nowMs;
  noTone(PIN_BUZZ);
  sndMode = SND_IDLE;
}

static void updateUnload(uint32_t nowMs) {
  if (!isUnloading) return;
  if ((int32_t)(nowMs - unloadNextMs) < 0) return;

  if (unloadRemaining > 0) {
    uint8_t stepIndex = (uint8_t)(unloadTotal - unloadRemaining);
    uint16_t freq = (uint16_t)(UNLOAD_CHIRP_BASE + (uint16_t)stepIndex * UNLOAD_CHIRP_STEP);
    tone(PIN_BUZZ, freq, UNLOAD_CHIRP_MS);
    unloadRemaining--;
    pollenCount = unloadRemaining;
    score = (uint16_t)(score + 1);
    spawnBeltItem(nowMs);
    hivePulseUntilMs = nowMs + HIVE_PULSE_MS;
    unloadNextMs = nowMs + UNLOAD_TICK_MS;
    return;
  }

  isUnloading = false;
  unloadRemaining = 0;
  if (unloadTotal > 0) {
    int hiveSX, hiveSY;
    worldToScreen(0, 0, hiveSX, hiveSY);
    spawnScorePopup(nowMs, unloadTotal, hiveSX, hiveSY);
  }
  unloadTotal = 0;
  survivalTimeLeft = SURVIVAL_TIME_MAX;
}

static void tryStoreAtHive(uint32_t nowMs) {
  if (pollenCount == 0) return;
  if (isUnloading) return;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;
  const int32_t hiveR = 22;

  if ((bx*bx + by*by) <= (hiveR*hiveR)) {
    beginUnload(nowMs);
  }
}

static bool anyTrailAlive() {
  for (int i = 0; i < TRAIL_MAX; i++) {
    if (trail[i].alive) return true;
  }
  return false;
}

static bool anyBeltAlive() {
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (beltItems[i].alive) return true;
  }
  return false;
}

static bool anyScorePopupAlive() {
  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (scorePopups[i].alive) return true;
  }
  return false;
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
      drawNebulaLayer(canvas, tileX, tileY, ox, oy, nowMs);
      drawWorldGrid(canvas, tileX, tileY, ox, oy);
      drawBoundaryZone(canvas, ox, oy);
      drawScreenAnchor(canvas, ox, oy, nowMs);

      // hive (only if near screen)
      if (hiveSX >= -40 && hiveSX <= tft.width() + 40 && hiveSY >= HUD_H - 40 && hiveSY <= tft.height() + 40) {
        drawHive(canvas, hiveSX + ox, hiveSY + oy);
        drawHivePulse(canvas, hiveSX + ox, hiveSY + oy, nowMs);
      }

      // flowers
      for (int i = 0; i < FLOWER_N; i++) {
        if (!flowers[i].alive) continue;
        int sx, sy;
        worldToScreen(flowers[i].wx, flowers[i].wy, sx, sy);
        if (sx < -30 || sx > tft.width() + 30 || sy < HUD_H - 30 || sy > tft.height() + 30) continue;
        drawFlower(canvas, sx + ox, sy + oy, flowers[i], nowMs, flowerBornMs[i]);
      }

      // boost trail particles (behind bee)
      drawTrailParticles(canvas, ox, oy, nowMs);

      // bee shadow + bee (always on screen)
      int bcX = beeScreenCX();
      int bcY = beeScreenCY();
      int bob = (int)(sinf((float)nowMs * 0.008f) * 2.0f);
      if ((int32_t)(nowMs - boostActiveUntilMs) < 0) {
        drawBoostAura(canvas, bcX + ox, bcY + oy + bob, nowMs);
      }
      drawBeeShadow(canvas, bcX + ox, bcY + oy + bob);
      drawBee(canvas, bcX + ox, bcY + oy + bob);
      drawPollenSparkles(canvas, bcX + ox, bcY + oy + bob, nowMs);
      drawScorePopups(canvas, ox, oy, nowMs);

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
  for (int i = 0; i < SCORE_POPUP_N; i++) scorePopups[i].alive = 0;

  initFlowers();

  // Start at hive
  beeWX = 0.0f;
  beeWY = 0.0f;
  beeVX = 0.0f;
  beeVY = 0.0f;

  pollenCount = 0;
  score = 0;
  isUnloading = false;
  unloadRemaining = 0;
  unloadTotal = 0;

  depositsTowardBoost = 0;
  boostCharge = 0;
  boostActiveUntilMs = 0;
  boostCooldownUntilMs = 0;

  radarActive = false;
  radarUntilMs = 0;
  hivePulseUntilMs = 0;

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

  bool boosting = false;
  static bool wasBoosting = false;

  if (!isUnloading) {
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
    boosting = (int32_t)(now - boostActiveUntilMs) < 0;
    if (boosting && !wasBoosting) {
      triggerCameraShake(now, 6.5f, 180);
    }
    wasBoosting = boosting;

    // ---- SPRING-BASED MOVEMENT ----
    // Joystick maps to target position in world space (bounded exploration area)
    const float roamRadius = BOUNDARY_COMFORTABLE; // max distance from hive
    float targetWX = nx * roamRadius;
    float targetWY = ny * roamRadius;

    // When joystick neutral, target snaps to hive center (implicit centering!)
    if (dx == 0) targetWX = 0.0f;
    if (dy == 0) targetWY = 0.0f;

    // Spring constants (higher = more responsive, boost increases responsiveness)
    const float springK = boosting ? SPRING_K_BOOST : SPRING_K_NORMAL;
    const float damping = boosting ? DAMPING_BOOST : DAMPING_NORMAL;

    // Spring force: F = k * (target - current) - damping * velocity
    float forceX = springK * (targetWX - beeWX) - damping * beeVX;
    float forceY = springK * (targetWY - beeWY) - damping * beeVY;

    beeVX += forceX * dt;
    beeVY += forceY * dt;

    // Integrate
    beeWX += beeVX * dt;
    beeWY += beeVY * dt;

    // Wing animation driven by speed
    float sp = fabsf(beeVX) + fabsf(beeVY);
    float spN = clampf(sp / WING_SPEED_DIVISOR, 0.0f, 1.0f);
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
        spawnTrailParticle(beeWX, beeWY, spN, now);
        spawnTrailParticle(beeWX - beeVX * 0.02f, beeWY - beeVY * 0.02f, spN, now); // slightly offset
        lastTrailMs = now;
      }
    }
    updateTrailParticles(now);
    updateScorePopups(now);

    // Camera zoom + shake response
    float targetZoom = boosting ? 1.22f : 1.0f;
    float zoomLerp = clampf(7.0f * dt, 0.0f, 1.0f);
    cameraZoom += (targetZoom - cameraZoom) * zoomLerp;

    if ((int32_t)(now - cameraShakeUntilMs) < 0 && cameraShakeDurationMs > 0) {
      float t = (float)(cameraShakeUntilMs - now) / (float)cameraShakeDurationMs;
      t = clampf(t, 0.0f, 1.0f);
      float amp = cameraShakeMagnitude * t * t;
      float phase = (float)now * 0.045f;
      cameraShakeX = sinf(phase * 6.2f) * amp;
      cameraShakeY = cosf(phase * 7.4f) * amp;
    } else {
      cameraShakeX = 0.0f;
      cameraShakeY = 0.0f;
    }
  } else {
    wasBoosting = false;
    beeWX = 0.0f;
    beeWY = 0.0f;
    beeVX = 0.0f;
    beeVY = 0.0f;
    wingSpeed = 0.0f;
    float zoomLerp = clampf(7.0f * dt, 0.0f, 1.0f);
    cameraZoom += (1.0f - cameraZoom) * zoomLerp;
    cameraShakeX = 0.0f;
    cameraShakeY = 0.0f;
    updateBeltLifetimes(now);
    updateUnload(now);
    updateTrailParticles(now);
    updateScorePopups(now);
  }

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
      isUnloading = false;
      unloadRemaining = 0;
      unloadTotal = 0;
    }
  }

  // --- Button handling ---
  bool edgeDown = false;
  if (!isUnloading) {
    bool b = joyPressedRaw();
    edgeDown = (b && !btnPrev);
    btnPrev = b;
  } else {
    btnPrev = false;
  }

  // Game over restart on button press
  if (isGameOver && edgeDown) {
    // Reset game
    beeWX = 0.0f;
    beeWY = 0.0f;
    beeVX = 0.0f;
    beeVY = 0.0f;
    survivalTimeLeft = SURVIVAL_TIME_MAX;
    isGameOver = false;
    pollenCount = 0;
    score = 0;
    isUnloading = false;
    unloadRemaining = 0;
    unloadTotal = 0;
    initFlowers();
    hivePulseUntilMs = 0;
    // Clear trail
    for (int i = 0; i < TRAIL_MAX; i++) trail[i].alive = 0;
    for (int i = 0; i < SCORE_POPUP_N; i++) scorePopups[i].alive = 0;
    // Continue to render the reset state
  }

  // Don't process normal game input during game over
  // But DO continue to render!
  if (!isGameOver && !isUnloading) {

    if (edgeDown) {
      // Click always gives some percussive feedback.
      if (!soundBusy()) startSound(SND_CLICK, now);
      // Radar ping
      beginRadarPing(now);
    }

    // Interactions
    updateBeltLifetimes(now);
    tryCollectPollen(now);
    tryStoreAtHive(now);

    // Sound: ambient wing buzz (only when no event sounds active) - stops when bee stops!
    if (!soundBusy()) {
      if (wingSpeed > 0.08f) {
        int freq = 90 + (int)(wingSpeed * 360.0f);
        int vib = (int)(sinf((float)now * 0.018f) * (6.0f + wingSpeed * 12.0f));
        freq += vib;
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
  uint32_t renderInterval = 40;
  bool idle = !isGameOver && !isUnloading && !radarActive && !boosting && (wingSpeed < 0.05f) && !anyTrailAlive() && !anyBeltAlive() && !anyScorePopupAlive();
  if (idle) renderInterval = 80; // ease CPU/GPU when calm
  if ((uint32_t)(now - lastRenderMs) >= renderInterval) {
    lastRenderMs = now;
    renderFrame(now);
  }

  delay(2);
}
