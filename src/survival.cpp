// Pixel Buzz Box - Survival (Timer, Game Over, Scoring)
#include "game.h"

// -------------------- POLLEN/SCORE STATE --------------------
uint8_t pollenCount = 0;
uint16_t score = 0;

// -------------------- SURVIVAL STATE --------------------
float survivalTimeLeft = SURVIVAL_TIME_MAX;
bool isGameOver = false;
uint32_t gameOverMs = 0;

// -------------------- SURVIVAL FLASH STATE --------------------
uint32_t survivalFlashUntilMs = 0;
float survivalFlashStartPct = 0.0f;
float survivalFlashEndPct = 0.0f;

// -------------------- TIMER UPDATE --------------------
void updateSurvivalTimer(float dt, uint32_t nowMs) {
  if (isGameOver) return;

  survivalTimeLeft -= dt;
  if (survivalTimeLeft <= 0.0f) {
    survivalTimeLeft = 0.0f;
    isGameOver = true;
    gameOverMs = nowMs;
  }
}

// -------------------- TIME GAIN --------------------
void addSurvivalTime(uint32_t nowMs, float amount) {
  float before = survivalTimeLeft;
  float after = clampf(survivalTimeLeft + amount, 0.0f, SURVIVAL_TIME_MAX);
  survivalTimeLeft = after;

  // Flash effect
  survivalFlashStartPct = clampf(before / SURVIVAL_TIME_MAX, 0.0f, 1.0f);
  survivalFlashEndPct = clampf(after / SURVIVAL_TIME_MAX, 0.0f, 1.0f);
  survivalFlashUntilMs = nowMs + SURVIVAL_FLASH_MS;
}

// -------------------- RESET --------------------
void resetSurvival() {
  pollenCount = 0;
  score = 0;
  survivalTimeLeft = SURVIVAL_TIME_MAX;
  isGameOver = false;
  gameOverMs = 0;
  survivalFlashUntilMs = 0;
  survivalFlashStartPct = 0.0f;
  survivalFlashEndPct = 0.0f;
}
