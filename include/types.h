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
