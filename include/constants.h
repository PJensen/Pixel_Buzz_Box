// Pixel Buzz Box - Game Constants and Colors
// Organized by domain namespace - NO LEGACY ALIASES
#pragma once

#include <Arduino.h>

// ============================================================================
// MATH CONSTANTS
// ============================================================================
namespace MathConst {
  constexpr float DEG2RAD = 0.0174532925f;
  constexpr float TAU = 6.2831853f;
  constexpr float PI_F = 3.1415926f;
}

// ============================================================================
// COMPILE-TIME FLAGS
// ============================================================================
#define SOUND_ENABLED 1

// ============================================================================
// DISPLAY
// ============================================================================
namespace Display {
  constexpr int CANVAS_W = 120;
  constexpr int CANVAS_H = 80;
  constexpr int HUD_H = 28;
  constexpr int SCREEN_W = 320;
  constexpr int SCREEN_H = 240;
}

// ============================================================================
// POOL SIZES
// ============================================================================
namespace Pool {
  constexpr uint8_t MAX_POLLEN_CARRY = 8;
  constexpr int FLOWER_N = 7;
  constexpr int BELT_ITEM_N = 10;
  constexpr int TRAIL_MAX = 24;
  constexpr int SCORE_POPUP_N = 6;
  constexpr int WASP_MAX = 10;  // Max capacity; active limit = currentDay
}

// ============================================================================
// TIMING
// ============================================================================
namespace Timing {
  constexpr uint32_t UNLOAD_TICK_MS = 100;
  constexpr uint32_t BELT_LIFE_MS = 14000;
  constexpr uint32_t HIVE_PULSE_MS = 520;
  constexpr uint32_t SCORE_POPUP_LIFE_MS = 1100;
  constexpr uint32_t SURVIVAL_FLASH_MS = 140;
  constexpr uint32_t TRAIL_LIFE_MS = 250;
  constexpr uint32_t RADAR_DURATION_MS = 320;
  constexpr uint32_t EVENT_TAIL_MS = 140;
  constexpr uint32_t RENDER_INTERVAL_ACTIVE_MS = 40;
  constexpr uint32_t RENDER_INTERVAL_IDLE_MS = 80;
  constexpr uint32_t TRAIL_SPAWN_INTERVAL_MS = 20;
  constexpr uint32_t MAX_DELTA_MS = 60;
  constexpr uint32_t LOOP_DELAY_MS = 2;
}

// ============================================================================
// AUDIO
// ============================================================================
namespace Audio {
  constexpr uint16_t UNLOAD_CHIRP_BASE = 760;
  constexpr uint16_t UNLOAD_CHIRP_STEP = 90;
  constexpr uint16_t UNLOAD_CHIRP_MS = 55;
}

// ============================================================================
// SURVIVAL
// ============================================================================
namespace Survival {
  constexpr float TIME_MAX = 15.0f;
  constexpr float POLLEN_BASE = 0.65f;
  constexpr float POLLEN_MULT_STEP = 0.55f;
}

// ============================================================================
// DAY/NIGHT CYCLE
// ============================================================================
namespace DayNight {
  constexpr float CYCLE_DURATION = 54.0f;     // Seconds for full day/night cycle (was 90)
  constexpr float DAWN_START = 0.20f;         // Phase when dawn begins
  constexpr float DAWN_END = 0.30f;           // Phase when full day
  constexpr float DUSK_START = 0.70f;         // Phase when dusk begins
  constexpr float DUSK_END = 0.80f;           // Phase when full night
  constexpr float NIGHT_DAMAGE_RATE = 3.0f;   // Time lost per second at night
  constexpr float NIGHT_WARN_PHASE = 0.65f;   // Start warning before dusk
  constexpr float START_PHASE = 0.20f;        // Start at dawn
  constexpr uint32_t NIGHT_REST_DURATION_MS = 2500;  // Duration of night rest animation
  constexpr float NIGHT_REST_TIME_BONUS = 2.0f;      // Bonus seconds for surviving the night
  constexpr uint32_t DAY_TRANSITION_DURATION_MS = 1800; // Duration of "DAY X" display
}

// ============================================================================
// BONUS
// ============================================================================
namespace Bonus {
  constexpr float SECONDS_PER_CHARGE = 3.0f;
  constexpr float MULTIPLIER_PER_CHARGE = 1.0f;
  constexpr uint8_t CHARGES_PER_ABILITY = 3;
  constexpr uint32_t FLASH_MS = 200;
}

// ============================================================================
// WORLD
// ============================================================================
namespace World {
  constexpr float BOUNDARY_BASE = 180.0f;       // Starting world radius (Day 1)
  constexpr float BOUNDARY_GROWTH = 15.0f;      // Additional radius per day
  constexpr float BOUNDARY_MAX = 300.0f;        // Maximum world radius
  constexpr float BOUNDARY_COMFORTABLE = 180.0f; // Legacy: use getWorldBoundary()
}

// ============================================================================
// PHYSICS
// ============================================================================
namespace Physics {
  constexpr float SPRING_K_NORMAL = 32.0f;
  constexpr float SPRING_K_BOOST = 48.0f;
  constexpr float DAMPING_NORMAL = 12.0f;
  constexpr float DAMPING_BOOST = 16.0f;
  constexpr float CARRY_WEIGHT_SPRING_PENALTY = 0.22f;
  constexpr float CARRY_WEIGHT_DAMPING_PENALTY = 0.18f;
  constexpr float BOOST_IMPULSE = 150.0f;
  constexpr uint32_t BOOST_DURATION_AUTO = 500;
  constexpr uint32_t BOOST_DURATION_MANUAL = 700;
  constexpr uint32_t BOOST_COOLDOWN_AUTO = 5200;
  constexpr uint32_t BOOST_COOLDOWN_MANUAL = 3500;
  constexpr float WING_SPEED_DIVISOR = 520.0f;
  constexpr float WING_HZ_MIN = 3.0f;
  constexpr float WING_HZ_RANGE = 14.0f;
  constexpr float WING_PHASE_WRAP = 1000.0f;
}

// ============================================================================
// CAMERA
// ============================================================================
namespace Camera {
  constexpr float ZOOM_BOOST = 1.18f;
  constexpr float ZOOM_NORMAL = 1.0f;
  constexpr float ZOOM_LERP_SPEED = 3.5f;
  constexpr float SHAKE_MAGNITUDE = 6.5f;
  constexpr uint32_t SHAKE_DURATION_MS = 180;
  constexpr float SHAKE_PHASE_MULT = 0.045f;
  constexpr float SHAKE_FREQ_X = 6.2f;
  constexpr float SHAKE_FREQ_Y = 7.4f;
}

// ============================================================================
// FLOWER
// ============================================================================
namespace FlowerCfg {
  constexpr int RADIUS_MIN = 6;
  constexpr int RADIUS_MAX = 11;
  constexpr int SPAWN_NEAR_DIST_MIN = 90;
  constexpr int SPAWN_NEAR_DIST_MAX = 220;
  constexpr int COLLISION_DIST = 80;
  constexpr int SPAWN_ELSEWHERE_DIST_MIN = 60;
  constexpr int SPAWN_ELSEWHERE_MARGIN = 20;
  constexpr int BEE_AVOIDANCE_DIST = 150;
  constexpr int SPACING_ELSEWHERE = 120;
  constexpr int BEE_HIT_RADIUS = 14;
  constexpr int SPAWN_NEAR_TRIES = 60;
  constexpr int SPAWN_ELSEWHERE_TRIES = 80;
  constexpr int FALLBACK_NEAR_MIN = 100;
  constexpr int FALLBACK_NEAR_MAX = 180;
  constexpr int FALLBACK_ELSEWHERE_MIN = 80;
  constexpr int FALLBACK_ELSEWHERE_MAX = 200;
  constexpr uint8_t RARE_SPAWN_CHANCE = 25;
  constexpr int RARE_DIST_MIN = 140;
  constexpr int RARE_DIST_MAX = 230;
  constexpr uint8_t RARE_POLLEN_BONUS = 2;
  constexpr int RARE_RADIUS_MIN = 9;
  constexpr int RARE_RADIUS_MAX = 13;
  constexpr int RARE_PARTICLE_COUNT = 6;
  constexpr float RARE_PARTICLE_ANGLE_STEP = 1.047f;
  constexpr float PARTICLE_OFFSET = 8.0f;
  constexpr float RARE_SHAKE_MAGNITUDE = 8.5f;
  constexpr uint32_t RARE_SHAKE_DURATION = 220;
  constexpr uint32_t MAGNET_PARTICLE_INTERVAL = 60;
  constexpr float MAGNET_TRAIL_MIN_DIST = 30.0f;
  constexpr float MAGNET_TRAIL_OFFSET = 5.0f;
}

// ============================================================================
// HIVE
// ============================================================================
namespace HiveCfg {
  constexpr int COLLECTION_RADIUS = 22;
}

// ============================================================================
// POPUP
// ============================================================================
namespace PopupCfg {
  constexpr int DRIFT_MIN = -10;
  constexpr int DRIFT_MAX = 10;
}

// ============================================================================
// INPUT
// ============================================================================
namespace Input {
  constexpr int JOY_CENTER_DEFAULT = 512;
  constexpr int JOY_RANGE = 512;
  constexpr int JOY_DEADZONE = 35;
  constexpr int CALIBRATION_SAMPLES = 40;
  constexpr int CALIBRATION_DELAY_MS = 30;
  constexpr float JOY_DOWN_BOOST = 1.20f;
  constexpr uint32_t TRIPLE_CLICK_WINDOW_MS = 500;  // Max time between 3 clicks
}

// ============================================================================
// MAGNET
// ============================================================================
namespace Magnet {
  constexpr uint8_t MIN_CHARGES = 1;
  constexpr uint32_t BASE_DURATION_MS = 800;
  constexpr uint32_t DURATION_PER_CHARGE = 400;
  constexpr uint32_t COOLDOWN_MS = 4000;
  constexpr float BASE_STRENGTH = 1.5f;
  constexpr float STRENGTH_PER_CHARGE = 0.3f;
  constexpr float MAX_STRENGTH = 3.0f;
  constexpr int PULL_RADIUS = 140;
  constexpr float SPRING_K = 12.0f;
  constexpr float CINEMATIC_PULL_SPEED = 0.2f;
  constexpr float CINEMATIC_BEE_SPEED = 0.5f;
}

// ============================================================================
// WASP
// ============================================================================
namespace WaspCfg {
  constexpr int SPAWN_DIST_MIN = 150;
  constexpr int SPAWN_DIST_MAX = 220;
  constexpr int SPAWN_SCORE_BASE = 5;      // First wasp spawns at this score
  constexpr int SPAWN_SCORE_INCREMENT = 10; // Each additional wasp needs +10 more
  constexpr uint32_t RESPAWN_DELAY_MS = 5000;
  constexpr float PATROL_SPEED = 60.0f;
  constexpr float HUNT_SPEED = 140.0f;
  constexpr float SPRING_K = 28.0f;
  constexpr float DAMPING = 10.0f;
  constexpr float STUN_DRIFT = 0.95f;
  constexpr int DETECTION_RADIUS = 100;
  constexpr int LOSE_INTEREST_RADIUS = 200;
  constexpr int PATROL_RADIUS = 120;
  constexpr uint32_t PATROL_CHANGE_MS = 2000;
  constexpr int HIT_RADIUS = 18;
  constexpr uint32_t STUN_DURATION_MS = 1500;
  constexpr uint32_t INVULN_MS = 1000;
  constexpr float TIME_PENALTY = 2.0f;
  constexpr float TIME_PENALTY_NO_POLLEN = 3.0f;
  constexpr uint8_t POLLEN_PENALTY = 1;
  constexpr uint32_t BEE_STUN_MS = 1000;
  constexpr float WING_HZ_BASE = 8.0f;
  constexpr float WING_HZ_RANGE = 12.0f;
  constexpr float WING_PHASE_WRAP = 100.0f;
  constexpr int DEATH_PARTICLE_COUNT = 4;
  constexpr float DEATH_PARTICLE_SPREAD = 8.0f;
  constexpr int SCREEN_MARGIN = 20;
}

// ============================================================================
// VFX
// ============================================================================
namespace Vfx {
  constexpr int VARIANT_NORMAL = 0;
  constexpr int VARIANT_GOLD = 3;
  constexpr int VARIANT_CYAN = 4;
  constexpr int VARIANT_RED = 5;
  constexpr uint32_t TRAIL_LIFE_MS = 300;
  constexpr float ALPHA_HIGH = 0.6f;
  constexpr float ALPHA_MID = 0.3f;
  constexpr float ALPHA_SPARKLE = 0.8f;
  constexpr int GLOW_RADIUS_OUTER = 5;
  constexpr int GLOW_RADIUS_MID = 3;
  constexpr int GLOW_RADIUS_CORE = 2;
  constexpr int SPARKLE_OFFSET = 3;
  constexpr float POPUP_FLOAT_HEIGHT = 28.0f;
  constexpr float POPUP_SWAY_FREQ = 0.018f;
  constexpr int POPUP_BASE_OFFSET = 6;
  constexpr float POPUP_SIZE_T1 = 0.18f;
  constexpr float POPUP_SIZE_T2 = 0.72f;
  constexpr float POPUP_HIGHLIGHT_T = 0.75f;
  constexpr float TETHER_MIN_DIST = 2.0f;
  constexpr int TETHER_ANIM_SPEED = 80;
  constexpr int TETHER_ANIM_CYCLE = 12;
  constexpr float TETHER_DOT_SPACING = 8.0f;
  constexpr uint32_t RADAR_BLINK_PERIOD = 400;
  constexpr int RADAR_RING_RADIUS = 18;
  constexpr int RADAR_ARROW_DIST = 32;
  constexpr int RADAR_DOT_START = 8;
  constexpr int RADAR_DOT_SPACING = 5;
  constexpr float RADAR_ARROW_BACK = 7.0f;
  constexpr float RADAR_ARROW_SPREAD = 4.0f;
  constexpr int RADAR_TEXT_OFFSET_X = 36;
  constexpr int RADAR_TEXT_OFFSET_Y = -8;
  constexpr int RADAR_PING_R0 = 14;
  constexpr float RADAR_PING_EXPAND = 26.0f;
  constexpr float RADAR_PING_INNER_T = 0.35f;
  constexpr int RADAR_PING_R1 = 10;
  constexpr float RADAR_PING_INNER_EXPAND = 30.0f;
  constexpr int RADAR_PING_ARROW_DIST = 36;
  constexpr float RADAR_PING_ARROW_BACK = 9.0f;
  constexpr float RADAR_PING_ARROW_SPREAD = 5.0f;
  constexpr int RADAR_PING_TEXT_X = 40;
  constexpr int RADAR_PING_TEXT_Y = -10;
  constexpr float MAGNET_PULSE_BASE = 3.0f;
  constexpr float MAGNET_PULSE_PER_CHARGE = 0.5f;
  constexpr int MAGNET_RING_BASE = 2;
  constexpr int MAGNET_RING_MIN = 2;
  constexpr int MAGNET_RING_MAX = 5;
  constexpr float MAGNET_RING_SPEED = 0.003f;
  constexpr int MAGNET_RING_R0 = 20;
  constexpr int MAGNET_RING_SPACING = 12;
  constexpr float MAGNET_RING_EXPAND = 30.0f;
  constexpr float MAGNET_THICK_THRESHOLD = 2.0f;
  constexpr int MAGNET_SPARKLE_COUNT = 8;
  constexpr float MAGNET_SPARKLE_SPEED = 0.003f;
  constexpr int MAGNET_CORE_R = 8;
  constexpr int MAGNET_LINE_ANIM_SPEED = 30;
  constexpr float MAGNET_LINE_SPACING = 6.0f;
  constexpr float MAGNET_ARROW_POS = 0.85f;
}

// ============================================================================
// RGB565 HELPER
// ============================================================================
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ============================================================================
// COLORS
// ============================================================================
namespace Color {
  static const uint16_t BG0        = rgb565(  4,  8, 18);
  static const uint16_t BG1        = rgb565(  8, 12, 26);
  static const uint16_t GRID       = rgb565( 18, 28, 52);
  static const uint16_t GRID2      = rgb565( 12, 18, 34);
  static const uint16_t STAR       = rgb565(240,240,240);
  static const uint16_t STAR2      = rgb565(180,210,255);
  static const uint16_t STAR3      = rgb565(255,230,180);
  static const uint16_t WHITE      = rgb565(245,245,245);
  static const uint16_t YEL        = rgb565(255,220, 40);
  static const uint16_t BLK        = rgb565( 20, 20, 20);
  static const uint16_t WING       = rgb565(180,220,255);
  static const uint16_t HIVE       = rgb565( 90,140, 90);
  static const uint16_t HUD_BG     = rgb565(  0,  0,  0);
  static const uint16_t POLLEN     = rgb565(255,235,110);
  static const uint16_t POLLEN_HI  = rgb565(255,255,210);
  static const uint16_t SHADOW     = rgb565(  0,  0,  0);
  static const uint16_t SHADOW_RIM = rgb565( 20, 20, 20);
  static const uint16_t UI_DIM     = rgb565(120,140,170);
  static const uint16_t UI_GO      = rgb565( 80,210,140);
  static const uint16_t UI_WARN    = rgb565(255,120,120);
  static const uint16_t TETHER_PRIMARY = rgb565(120, 100, 220);
  static const uint16_t TETHER_ACCENT = rgb565(140, 160, 255);
  static const uint16_t MAGNET_CORE = rgb565(150, 220, 255);
  static const uint16_t MAGNET_ARROW = rgb565(150, 230, 255);

  // Sky colors (interpolate based on dayPhase)
  static const uint16_t SKY_DAY_TOP     = rgb565(100, 160, 220);  // Light blue
  static const uint16_t SKY_DAY_BOT     = rgb565(160, 200, 240);  // Lighter blue
  static const uint16_t SKY_DAWN_TOP    = rgb565(255, 140, 100);  // Orange-pink
  static const uint16_t SKY_DAWN_BOT    = rgb565(255, 180, 140);  // Peach
  static const uint16_t SKY_DUSK_TOP    = rgb565(180,  80, 120);  // Purple-pink
  static const uint16_t SKY_DUSK_BOT    = rgb565(255, 130, 100);  // Orange
  static const uint16_t SKY_NIGHT_TOP   = rgb565( 10,  15,  35);  // Deep blue-black
  static const uint16_t SKY_NIGHT_BOT   = rgb565( 25,  35,  60);  // Dark blue

  // Sun and moon
  static const uint16_t SUN_CORE        = rgb565(255, 240, 180);  // Warm yellow
  static const uint16_t SUN_GLOW        = rgb565(255, 200, 100);  // Orange glow
  static const uint16_t MOON_CORE       = rgb565(230, 230, 240);  // Pale white
  static const uint16_t MOON_GLOW       = rgb565(180, 190, 220);  // Blue-ish glow

  // Outdoor ground colors
  static const uint16_t GRASS_DARK      = rgb565( 40,  80,  35);  // Dark grass
  static const uint16_t GRASS_MID       = rgb565( 60, 120,  50);  // Medium grass
  static const uint16_t GRASS_LIGHT     = rgb565( 90, 160,  70);  // Light grass tip
  static const uint16_t GRASS_NIGHT     = rgb565( 20,  40,  25);  // Night grass
  static const uint16_t GROUND_BASE     = rgb565( 50,  70,  40);  // Ground between grass

  // Cloud colors
  static const uint16_t CLOUD_WHITE     = rgb565(245, 248, 255);  // Bright cloud
  static const uint16_t CLOUD_GRAY      = rgb565(200, 210, 225);  // Cloud shadow
  static const uint16_t CLOUD_NIGHT     = rgb565( 60,  70,  90);  // Night cloud

  // Floating pollen/seeds
  static const uint16_t SEED_WHITE      = rgb565(255, 255, 240);  // Dandelion seed
  static const uint16_t SEED_YELLOW     = rgb565(255, 230, 150);  // Pollen particle
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
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

// ============================================================================
// RNG
// ============================================================================
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
