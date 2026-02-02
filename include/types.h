// Pixel Buzz Box - Data Structures
#pragma once

#include <Arduino.h>

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
