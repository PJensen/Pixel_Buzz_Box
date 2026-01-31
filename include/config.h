// Pixel Buzz Box - Configuration Header
// Hardware pins, constants, colors, and data structures
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// -------------------- PINS --------------------
// LCD (SPI)
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

// -------------------- DISPLAY --------------------
static const int CANVAS_W = 120;
static const int CANVAS_H = 80;
static const int HUD_H = 28;

// -------------------- GAME CONSTANTS --------------------
static const uint8_t MAX_POLLEN_CARRY = 8;
static const int FLOWER_N = 7;
static const int BELT_ITEM_N = 10;
static const int TRAIL_MAX = 24;
static const int SCORE_POPUP_N = 6;

// Timing
static const uint32_t UNLOAD_TICK_MS = 100;
static const uint16_t UNLOAD_CHIRP_BASE = 760;
static const uint16_t UNLOAD_CHIRP_STEP = 90;
static const uint16_t UNLOAD_CHIRP_MS = 55;
static const uint32_t BELT_LIFE_MS = 14000;
static const uint32_t HIVE_PULSE_MS = 520;
static const uint32_t SCORE_POPUP_LIFE_MS = 1100;
static const uint32_t SURVIVAL_FLASH_MS = 140;

// Survival
static const float SURVIVAL_TIME_MAX = 15.0f;
static const float SURVIVAL_POLLEN_BASE = 0.65f;
static const float SURVIVAL_POLLEN_MULT_STEP = 0.55f;

// World constraints
static const float BOUNDARY_COMFORTABLE = 180.0f;

// Movement physics
static const float SPRING_K_NORMAL = 32.0f;
static const float SPRING_K_BOOST = 48.0f;
static const float DAMPING_NORMAL = 12.0f;
static const float DAMPING_BOOST = 16.0f;
static const float BOOST_IMPULSE = 150.0f;
static const uint32_t BOOST_DURATION_AUTO = 500;
static const uint32_t BOOST_DURATION_MANUAL = 700;
static const uint32_t BOOST_COOLDOWN_AUTO = 5200;
static const uint32_t BOOST_COOLDOWN_MANUAL = 3500;
static const float WING_SPEED_DIVISOR = 520.0f;

// -------------------- RGB565 HELPER --------------------
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

// -------------------- DATA STRUCTURES --------------------
struct Flower {
  int32_t wx, wy;
  uint8_t alive;
  uint8_t r;
  uint16_t petal;
  uint16_t petalLo;
  uint16_t center;
};

struct BeltItem {
  uint32_t bornMs;
  uint8_t alive;
};

struct TrailParticle {
  float wx, wy;
  uint32_t bornMs;
  uint8_t alive;
  uint8_t variant;
  float speedN;
};

struct ScorePopup {
  uint32_t bornMs;
  int16_t baseSX;
  int16_t baseSY;
  int8_t driftX;
  uint8_t value;
  uint8_t alive;
};

// -------------------- UTILITY FUNCTIONS --------------------
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

// -------------------- RNG --------------------
extern uint32_t rngState;

static inline uint32_t xrnd() {
  uint32_t x = rngState;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  return (rngState = x);
}

static inline int irand(int lo, int hi) {
  uint32_t r = xrnd();
  return lo + (int)(r % (uint32_t)(hi - lo + 1));
}

static inline uint32_t hash32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

#endif // CONFIG_H
