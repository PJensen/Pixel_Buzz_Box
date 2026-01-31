// Pixel Buzz Box - Hive (Unloading, Belt, Deposits)
#include "game.h"
#include "BuzzSynth.h"

extern BuzzSynth buzzer;

// -------------------- UNLOAD STATE --------------------
bool isUnloading = false;
uint8_t unloadRemaining = 0;
uint8_t unloadTotal = 0;
uint32_t unloadNextMs = 0;

// -------------------- DEPOSIT TRACKING --------------------
uint8_t depositsTowardBoost = 0;
uint8_t boostCharge = 0;

// -------------------- BELT STATE --------------------
BeltItem beltItems[BELT_ITEM_N];

// -------------------- BELT FUNCTIONS --------------------
void spawnBeltItem(uint32_t nowMs) {
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

void updateBeltLifetimes(uint32_t nowMs) {
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    if ((uint32_t)(nowMs - beltItems[i].bornMs) > BELT_LIFE_MS) beltItems[i].alive = 0;
  }
}

bool anyBeltAlive() {
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (beltItems[i].alive) return true;
  }
  return false;
}

// -------------------- UNLOAD SEQUENCE --------------------
void beginUnload(uint32_t nowMs) {
  if (isUnloading || pollenCount == 0) return;
  isUnloading = true;
  unloadRemaining = pollenCount;
  unloadTotal = pollenCount;
  unloadNextMs = nowMs;
  buzzer.stopAll();
}

void updateUnload(uint32_t nowMs) {
  if (!isUnloading) return;
  if ((int32_t)(nowMs - unloadNextMs) < 0) return;

  if (unloadRemaining > 0) {
    uint8_t stepIndex = (uint8_t)(unloadTotal - unloadRemaining);
    uint16_t freq = (uint16_t)(UNLOAD_CHIRP_BASE + (uint16_t)stepIndex * UNLOAD_CHIRP_STEP);
    buzzer.playUnloadTone(freq, UNLOAD_CHIRP_MS);
    unloadRemaining--;
    pollenCount = unloadRemaining;
    score = (uint16_t)(score + 1);
    spawnBeltItem(nowMs);
    triggerHivePulse(nowMs);

    // Survival time gain
    float tickMult = 1.0f + (float)stepIndex * SURVIVAL_POLLEN_MULT_STEP;
    float gain = SURVIVAL_POLLEN_BASE * tickMult;
    addSurvivalTime(nowMs, gain);

    unloadNextMs = nowMs + UNLOAD_TICK_MS;
    return;
  }

  isUnloading = false;
  unloadRemaining = 0;

  SoundState& snd = buzzer.getState();
  if (snd.lastUnloadFreq > 0.0f) {
    buzzer.setEventTail(nowMs, snd.lastUnloadFreq, 140);
  }

  if (unloadTotal > 0) {
    int hiveSX, hiveSY;
    worldToScreen(0, 0, hiveSX, hiveSY);
    spawnScorePopup(nowMs, unloadTotal, hiveSX, hiveSY);
  }
  unloadTotal = 0;
}

// -------------------- HIVE INTERACTION --------------------
void tryStoreAtHive(uint32_t nowMs) {
  if (pollenCount == 0) return;
  if (isUnloading) return;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;
  const int32_t hiveR = 22;

  if ((bx*bx + by*by) <= (hiveR*hiveR)) {
    beginUnload(nowMs);
  }
}

// -------------------- RESET --------------------
void resetHive() {
  isUnloading = false;
  unloadRemaining = 0;
  unloadTotal = 0;
  depositsTowardBoost = 0;
  boostCharge = 0;
  for (int i = 0; i < BELT_ITEM_N; i++) beltItems[i].alive = 0;
}
