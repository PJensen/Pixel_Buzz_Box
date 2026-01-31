// Pixel Buzz Box - Game Constants and Colors
#pragma once

#include <Arduino.h>

// -------------------- DISPLAY --------------------
static const int CANVAS_W = 120;
static const int CANVAS_H = 80;
static const int HUD_H = 28;

// -------------------- ARRAY SIZES --------------------
static const uint8_t MAX_POLLEN_CARRY = 8;
static const int FLOWER_N = 7;
static const int BELT_ITEM_N = 10;
static const int TRAIL_MAX = 24;
static const int SCORE_POPUP_N = 6;

// -------------------- TIMING --------------------
static const uint32_t UNLOAD_TICK_MS = 100;
static const uint16_t UNLOAD_CHIRP_BASE = 760;
static const uint16_t UNLOAD_CHIRP_STEP = 90;
static const uint16_t UNLOAD_CHIRP_MS = 55;
static const uint32_t BELT_LIFE_MS = 14000;
static const uint32_t HIVE_PULSE_MS = 520;
static const uint32_t SCORE_POPUP_LIFE_MS = 1100;
static const uint32_t SURVIVAL_FLASH_MS = 140;
static const uint32_t TRAIL_LIFE_MS = 250;
static const uint32_t RADAR_DURATION_MS = 320;
static const uint32_t EVENT_TAIL_MS = 140;

// -------------------- RENDER TIMING --------------------
static const uint32_t RENDER_INTERVAL_ACTIVE_MS = 40;   // ~25 FPS
static const uint32_t RENDER_INTERVAL_IDLE_MS = 80;     // ~12.5 FPS
static const uint32_t TRAIL_SPAWN_INTERVAL_MS = 20;
static const uint32_t MAX_DELTA_MS = 60;
static const uint32_t LOOP_DELAY_MS = 2;

// -------------------- SURVIVAL --------------------
static const float SURVIVAL_TIME_MAX = 15.0f;
static const float SURVIVAL_POLLEN_BASE = 0.65f;
static const float SURVIVAL_POLLEN_MULT_STEP = 0.55f;

// -------------------- WORLD CONSTRAINTS --------------------
static const float BOUNDARY_COMFORTABLE = 180.0f;

// -------------------- MOVEMENT PHYSICS --------------------
static const float SPRING_K_NORMAL = 32.0f;
static const float SPRING_K_BOOST = 48.0f;
static const float DAMPING_NORMAL = 12.0f;
static const float DAMPING_BOOST = 16.0f;
static const float CARRY_WEIGHT_SPRING_PENALTY = 0.22f;
static const float CARRY_WEIGHT_DAMPING_PENALTY = 0.18f;
static const float BOOST_IMPULSE = 150.0f;
static const uint32_t BOOST_DURATION_AUTO = 500;
static const uint32_t BOOST_DURATION_MANUAL = 700;
static const uint32_t BOOST_COOLDOWN_AUTO = 5200;
static const uint32_t BOOST_COOLDOWN_MANUAL = 3500;

// -------------------- WING ANIMATION --------------------
static const float WING_SPEED_DIVISOR = 520.0f;
static const float WING_HZ_MIN = 3.0f;
static const float WING_HZ_RANGE = 14.0f;      // Hz = MIN + RANGE * speed
static const float WING_PHASE_WRAP = 1000.0f;

// -------------------- CAMERA --------------------
static const float CAMERA_ZOOM_BOOST = 1.22f;
static const float CAMERA_ZOOM_NORMAL = 1.0f;
static const float CAMERA_ZOOM_LERP_SPEED = 7.0f;
static const float CAMERA_SHAKE_MAGNITUDE = 6.5f;
static const uint32_t CAMERA_SHAKE_DURATION_MS = 180;
static const float CAMERA_SHAKE_PHASE_MULT = 0.045f;
static const float CAMERA_SHAKE_FREQ_X = 6.2f;
static const float CAMERA_SHAKE_FREQ_Y = 7.4f;

// -------------------- FLOWER SPAWNING --------------------
static const int FLOWER_RADIUS_MIN = 6;
static const int FLOWER_RADIUS_MAX = 11;
static const int FLOWER_SPAWN_NEAR_DIST_MIN = 90;
static const int FLOWER_SPAWN_NEAR_DIST_MAX = 220;
static const int FLOWER_COLLISION_DIST = 80;
static const int FLOWER_SPAWN_ELSEWHERE_DIST_MIN = 60;
static const int FLOWER_SPAWN_ELSEWHERE_MARGIN = 20;
static const int FLOWER_BEE_AVOIDANCE_DIST = 150;
static const int FLOWER_SPACING_ELSEWHERE = 120;
static const int BEE_HIT_RADIUS = 14;

// -------------------- HIVE --------------------
static const int HIVE_COLLECTION_RADIUS = 22;

// -------------------- SCORE POPUP --------------------
static const int SCORE_POPUP_DRIFT_MIN = -10;
static const int SCORE_POPUP_DRIFT_MAX = 10;

// -------------------- INPUT --------------------
static const int JOY_CENTER_DEFAULT = 512;
static const int JOY_RANGE = 512;
static const int JOY_DEADZONE = 35;
static const int JOY_CALIBRATION_SAMPLES = 40;
static const int JOY_CALIBRATION_DELAY_MS = 30;
static const float JOY_DOWN_BOOST = 1.20f;

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
