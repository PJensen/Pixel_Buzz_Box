// Pixel Buzz Box - Hive (Unloading, Belt, Deposits)
#include "game.h"
#include "BuzzSynth.h"

extern BuzzSynth buzzer;

// -------------------- FORWARD DECLARATIONS --------------------
static void checkAndActivateMagnet();

// -------------------- UNLOAD STATE --------------------
bool isUnloading = false;
uint8_t unloadRemaining = 0;
uint8_t unloadTotal = 0;
uint16_t unloadScoreGained = 0;
uint32_t unloadNextMs = 0;

// -------------------- DEPOSIT TRACKING --------------------
uint8_t depositsTowardBoost = 0;
uint8_t boostCharge = 0;

// -------------------- BONUS SYSTEM --------------------
float bonusPoints = 0.0f;
uint16_t totalBonusCharges = 0;
uint16_t lastMagnetActivationThreshold = 0;
bool pollenMagnetActive = false;
uint32_t bonusFlashUntilMs = 0;

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
  unloadScoreGained = 0;
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

    // Score with multiplier
    float multiplier = getScoreMultiplier();
    uint16_t pointsGained = (uint16_t)multiplier;
    score = (uint16_t)(score + pointsGained);
    unloadScoreGained = (uint16_t)(unloadScoreGained + pointsGained);

    spawnBeltItem(nowMs);
    triggerHivePulse(nowMs);

    // Survival time gain + overage collection
    float tickMult = 1.0f + (float)stepIndex * SURVIVAL_POLLEN_MULT_STEP;
    float gain = SURVIVAL_POLLEN_BASE * tickMult;
    float overage = addSurvivalTime(nowMs, gain);

    // Add overage to bonus points
    if (overage > 0.0f) {
      bonusPoints += overage;
      updateBonusSystem();
      bonusFlashUntilMs = nowMs + BONUS_FLASH_MS;
    }

    unloadNextMs = nowMs + UNLOAD_TICK_MS;
    return;
  }

  isUnloading = false;
  unloadRemaining = 0;

  // Update bonus system
  updateBonusSystem();

  // Deactivate magnet if it was active (trip complete)
  // Then check if we should activate for next trip
  if (pollenMagnetActive) {
    pollenMagnetActive = false;
  }
  checkAndActivateMagnet();

  SoundState& snd = buzzer.getState();
  if (snd.lastUnloadFreq > 0.0f) {
    buzzer.setEventTail(nowMs, snd.lastUnloadFreq, EVENT_TAIL_MS);
  }

  if (unloadTotal > 0) {
    int hiveSX, hiveSY;
    worldToScreen(0, 0, hiveSX, hiveSY);
    float multiplier = getScoreMultiplier();
    spawnScorePopup(nowMs, unloadTotal, multiplier, hiveSX, hiveSY);
  }
  unloadTotal = 0;
  unloadScoreGained = 0;
}

// -------------------- BONUS SYSTEM --------------------
void updateBonusSystem() {
  // Convert bonus points to charges
  uint16_t newTotalCharges = (uint16_t)(bonusPoints / BONUS_SECONDS_PER_CHARGE);

  // Update total charges
  if (newTotalCharges != totalBonusCharges) {
    totalBonusCharges = newTotalCharges;
  }

  // Update display progress (0-2 for "x3 N/3" display)
  depositsTowardBoost = (uint8_t)(totalBonusCharges % BONUS_CHARGES_PER_ABILITY);

  // Update boost charge indicator (shows if ability is ready)
  boostCharge = pollenMagnetActive ? 1 : 0;
}

void checkAndActivateMagnet() {
  // Calculate current ability threshold (which multiple of 3 are we at/past?)
  uint16_t currentThreshold = (totalBonusCharges / BONUS_CHARGES_PER_ABILITY) * BONUS_CHARGES_PER_ABILITY;

  // Activate if we've reached a new threshold
  if (!pollenMagnetActive && currentThreshold >= BONUS_CHARGES_PER_ABILITY && currentThreshold > lastMagnetActivationThreshold) {
    pollenMagnetActive = true;
    lastMagnetActivationThreshold = currentThreshold;
  }
}

float getScoreMultiplier() {
  return 1.0f + ((float)totalBonusCharges * BONUS_MULTIPLIER_PER_CHARGE);
}

// -------------------- HIVE INTERACTION --------------------
void tryStoreAtHive(uint32_t nowMs) {
  if (pollenCount == 0) return;
  if (isUnloading) return;

  int32_t bx = (int32_t)beeWX;
  int32_t by = (int32_t)beeWY;

  if ((bx*bx + by*by) <= (HIVE_COLLECTION_RADIUS*HIVE_COLLECTION_RADIUS)) {
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
  bonusPoints = 0.0f;
  totalBonusCharges = 0;
  lastMagnetActivationThreshold = 0;
  pollenMagnetActive = false;
  bonusFlashUntilMs = 0;
  for (int i = 0; i < BELT_ITEM_N; i++) beltItems[i].alive = 0;
}
