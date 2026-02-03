// Pixel Buzz Box - Data Structures
#pragma once

#include <Arduino.h>

// Forward declarations for circular dependencies
struct BeeState;
struct HiveState;
struct MagnetState;
struct RadarState;
struct CameraState;
struct SurvivalState;
struct InputState;

// -------------------- GROUPED STATE STRUCTS --------------------

struct BeeState {
  float wx, wy;                   // World position
  float vx, vy;                   // Velocity
  float wingPhase;                // Wing animation phase
  float wingSpeed;                // Wing animation speed
  uint32_t boostActiveUntilMs;    // Boost active until
  uint32_t boostCooldownUntilMs;  // Boost cooldown until
  uint32_t stunnedUntilMs;        // Stunned until (wasp hit)
};

struct MagnetState {
  bool active;
  uint32_t activeUntilMs;
  uint32_t cooldownUntilMs;
  uint16_t chargesConsumed;
  float strength;
};

struct HiveState {
  bool isUnloading;
  uint8_t unloadRemaining;
  uint8_t unloadTotal;
  uint32_t unloadNextMs;
  uint8_t depositsTowardBoost;
  uint8_t boostCharge;
  float bonusPoints;
  uint16_t totalBonusCharges;
  bool pollenMagnetActive;
  uint32_t bonusFlashUntilMs;
  MagnetState magnet;
};

struct RadarState {
  bool active;
  uint32_t untilMs;
  int32_t targetWX, targetWY;
  bool toHive;
  bool fullActive;
};

struct CameraState {
  float zoom;
  float shakeX, shakeY;
  uint32_t shakeUntilMs;
  uint32_t shakeDurationMs;
  float shakeMagnitude;
};

struct SurvivalState {
  uint8_t pollenCount;
  uint16_t score;
  float timeLeft;
  bool isGameOver;
  uint32_t gameOverMs;
  uint32_t flashUntilMs;
  float flashStartPct;
  float flashEndPct;
  float dayPhase;           // 0.0 = midnight, 0.25 = dawn, 0.5 = noon, 0.75 = dusk
  bool diedAtNight;         // True if bee died from being out after dark
  bool isNightResting;      // True when bee is safe at hive during night transition
  uint32_t nightRestStartMs; // When night rest animation began
  uint8_t nightsSurvived;   // Count of successful nights (bonus display)
  uint8_t currentDay;       // Current day number (starts at 1)
  bool isDayTransition;     // True during "DAY X" display
  uint32_t dayTransitionStartMs; // When day transition began
};

struct InputState {
  int centerX, centerY;
  int minY, maxY;
  bool btnPrev;
};

// -------------------- GAME ENTITIES --------------------
enum FlowerType : uint8_t {
  FLOWER_NORMAL = 0,
  FLOWER_RARE = 1
};

struct Flower {
  int32_t wx, wy;       // World position
  uint8_t alive;
  uint8_t r;            // Radius
  FlowerType type;      // Normal or rare
  uint16_t petal;       // Petal color
  uint16_t petalLo;     // Darker petal color
  uint16_t center;      // Center color
};

struct BeltItem {
  uint32_t bornMs;
  uint8_t alive;
};

struct TrailParticle {
  float wx, wy;         // World position
  uint32_t bornMs;
  uint8_t alive;
  uint8_t variant;
  float speedN;         // Normalized speed
};

struct ScorePopup {
  uint32_t bornMs;
  int16_t baseSX;       // Screen X
  int16_t baseSY;       // Screen Y
  int8_t driftX;
  uint8_t value;        // Pollen count
  float multiplier;     // Score multiplier
  uint8_t alive;
};

// -------------------- WASP (PREDATOR) --------------------
enum WaspState : uint8_t {
  WASP_PATROL = 0,      // Random wandering
  WASP_HUNTING = 1,     // Chasing bee
  WASP_STUNNED = 2      // After hitting bee (cooldown)
};

struct Wasp {
  float wx, wy;           // World position
  float vx, vy;           // Velocity
  float targetWX, targetWY; // AI target point
  uint8_t alive;
  WaspState state;
  float wingPhase;        // Animation
  uint32_t stunnedUntilMs;
  uint32_t lastStateChangeMs;
  uint32_t deathMs;       // When wasp was killed (for respawn timing)
};
