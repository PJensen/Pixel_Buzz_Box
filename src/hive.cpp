// Pixel Buzz Box - Hive (Unloading, Belt, Deposits)
#include "game.h"
#include "BuzzSynth.h"

extern BuzzSynth buzzer;

// -------------------- FORWARD DECLARATIONS --------------------
static void checkAndActivateMagnet();

// -------------------- HIVE STATE --------------------
HiveState hive = {
  .isUnloading = false,
  .unloadRemaining = 0,
  .unloadTotal = 0,
  .unloadNextMs = 0,
  .depositsTowardBoost = 0,
  .boostCharge = 0,
  .bonusPoints = 0.0f,
  .totalBonusCharges = 0,
  .pollenMagnetActive = false,
  .bonusFlashUntilMs = 0,
  .magnet = { .active = false, .activeUntilMs = 0, .cooldownUntilMs = 0, .chargesConsumed = 0, .strength = 1.0f }
};

// -------------------- LOCAL STATE (not exported) --------------------
static uint16_t unloadScoreGained = 0;
static uint16_t lastMagnetActivationThreshold = 0;

// -------------------- BELT STATE --------------------
BeltItem beltItems[Pool::BELT_ITEM_N];

// -------------------- BELT FUNCTIONS --------------------
void spawnBeltItem(uint32_t nowMs) {
  int freeIdx = -1;
  uint32_t oldest = 0xFFFFFFFFu;
  int oldestIdx = 0;

  for (int i = 0; i < Pool::BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) { freeIdx = i; break; }
    if (beltItems[i].bornMs < oldest) { oldest = beltItems[i].bornMs; oldestIdx = i; }
  }

  int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
  beltItems[idx].alive = 1;
  beltItems[idx].bornMs = nowMs;
}

void updateBeltLifetimes(uint32_t nowMs) {
  for (int i = 0; i < Pool::BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    if ((uint32_t)(nowMs - beltItems[i].bornMs) > Timing::BELT_LIFE_MS) beltItems[i].alive = 0;
  }
}

bool anyBeltAlive() {
  for (int i = 0; i < Pool::BELT_ITEM_N; i++) {
    if (beltItems[i].alive) return true;
  }
  return false;
}

// -------------------- UNLOAD SEQUENCE --------------------
void beginUnload(uint32_t nowMs) {
  if (hive.isUnloading || survival.pollenCount == 0) return;
  hive.isUnloading = true;
  hive.unloadRemaining = survival.pollenCount;
  hive.unloadTotal = survival.pollenCount;
  unloadScoreGained = 0;
  hive.unloadNextMs = nowMs;
  buzzer.stopAll();
}

void updateUnload(uint32_t nowMs) {
  if (!hive.isUnloading) return;
  if ((int32_t)(nowMs - hive.unloadNextMs) < 0) return;

  if (hive.unloadRemaining > 0) {
    uint8_t stepIndex = (uint8_t)(hive.unloadTotal - hive.unloadRemaining);
    uint16_t freq = (uint16_t)(Audio::UNLOAD_CHIRP_BASE + (uint16_t)stepIndex * Audio::UNLOAD_CHIRP_STEP);
    buzzer.playUnloadTone(freq, Audio::UNLOAD_CHIRP_MS);
    hive.unloadRemaining--;
    survival.pollenCount = hive.unloadRemaining;

    // Score with multiplier
    float multiplier = getScoreMultiplier();
    uint16_t pointsGained = (uint16_t)multiplier;
    survival.score = (uint16_t)(survival.score + pointsGained);
    unloadScoreGained = (uint16_t)(unloadScoreGained + pointsGained);

    spawnBeltItem(nowMs);
    triggerHivePulse(nowMs);

    // Survival time gain + overage collection
    float tickMult = 1.0f + (float)stepIndex * Survival::POLLEN_MULT_STEP;
    float gain = Survival::POLLEN_BASE * tickMult;
    float overage = addSurvivalTime(nowMs, gain);

    // Add overage to bonus points
    if (overage > 0.0f) {
      hive.bonusPoints += overage;
      updateBonusSystem();
      hive.bonusFlashUntilMs = nowMs + Bonus::FLASH_MS;
    }

    hive.unloadNextMs = nowMs + Timing::UNLOAD_TICK_MS;
    return;
  }

  hive.isUnloading = false;
  hive.unloadRemaining = 0;

  // Kill any wasps visible on screen! Strategic mechanic.
  int waspsKilled = killOnScreenWasps(nowMs);
  if (waspsKilled > 0) {
    triggerCameraShake(nowMs, 4.0f + (float)waspsKilled * 2.0f, 200);
    if (!buzzer.soundBusy()) {
      buzzer.startSound(SND_WASP_KILL, nowMs);
    }
  }

  // Update bonus system
  updateBonusSystem();

  SoundState& snd = buzzer.getState();
  if (snd.lastUnloadFreq > 0.0f) {
    buzzer.setEventTail(nowMs, snd.lastUnloadFreq, Timing::EVENT_TAIL_MS);
  }

  if (hive.unloadTotal > 0) {
    int hiveSX, hiveSY;
    worldToScreen(0, 0, hiveSX, hiveSY);
    float multiplier = getScoreMultiplier();
    spawnScorePopup(nowMs, hive.unloadTotal, multiplier, hiveSX, hiveSY);
  }
  hive.unloadTotal = 0;
  unloadScoreGained = 0;
}

// -------------------- BONUS SYSTEM --------------------
void updateBonusSystem() {
  // Convert bonus points to charges
  uint16_t newTotalCharges = (uint16_t)(hive.bonusPoints / Bonus::SECONDS_PER_CHARGE);

  // Update total charges
  if (newTotalCharges != hive.totalBonusCharges) {
    hive.totalBonusCharges = newTotalCharges;
  }

  // Update display progress (0-2 for "x3 N/3" display)
  hive.depositsTowardBoost = (uint8_t)(hive.totalBonusCharges % Bonus::CHARGES_PER_ABILITY);
}

float getScoreMultiplier() {
  return 1.0f + ((float)hive.totalBonusCharges * Bonus::MULTIPLIER_PER_CHARGE);
}

// -------------------- MAGNET ABILITY FUNCTIONS --------------------
bool isMagnetActive(uint32_t nowMs) {
  return hive.magnet.active && ((int32_t)(nowMs - hive.magnet.activeUntilMs) < 0);
}

bool isMagnetOnCooldown(uint32_t nowMs) {
  return (int32_t)(hive.magnet.cooldownUntilMs - nowMs) > 0;
}

bool canActivateMagnet(uint32_t nowMs) {
  return hive.totalBonusCharges >= Magnet::MIN_CHARGES
         && !isMagnetActive(nowMs)
         && !isMagnetOnCooldown(nowMs)
         && !hive.isUnloading;
}

void triggerMagnet(uint32_t nowMs) {
  if (!canActivateMagnet(nowMs)) return;

  // Consume ALL available charges
  hive.magnet.chargesConsumed = hive.totalBonusCharges;

  // Calculate duration (scales linearly)
  uint32_t duration = Magnet::BASE_DURATION_MS
                     + (hive.magnet.chargesConsumed * Magnet::DURATION_PER_CHARGE);

  // Calculate strength (scales with diminishing returns, capped)
  hive.magnet.strength = Magnet::BASE_STRENGTH
                  + ((float)hive.magnet.chargesConsumed * Magnet::STRENGTH_PER_CHARGE);
  hive.magnet.strength = clampf(hive.magnet.strength, Magnet::BASE_STRENGTH, Magnet::MAX_STRENGTH);

  // Set timing
  hive.magnet.activeUntilMs = nowMs + duration;
  hive.magnet.cooldownUntilMs = nowMs + duration + Magnet::COOLDOWN_MS;
  hive.magnet.active = true;

  // Deduct charges from pool
  hive.bonusPoints -= (float)hive.magnet.chargesConsumed * Bonus::SECONDS_PER_CHARGE;
  hive.totalBonusCharges = 0;
  hive.depositsTowardBoost = 0;

  // Visual/audio feedback
  triggerCameraShake(nowMs, 5.0f, 300);
  if (!buzzer.soundBusy()) {
    buzzer.startSound(SND_MAGNET_ACTIVATE, nowMs);
  }
}

void updateMagnet(uint32_t nowMs) {
  if (!hive.magnet.active) return;

  // Check expiration
  if ((int32_t)(nowMs - hive.magnet.activeUntilMs) >= 0) {
    hive.magnet.active = false;
    hive.magnet.chargesConsumed = 0;
    hive.magnet.strength = 1.0f;
  }
}

// -------------------- HIVE INTERACTION --------------------
void tryStoreAtHive(uint32_t nowMs) {
  if (survival.pollenCount == 0) return;
  if (hive.isUnloading) return;

  int32_t bx = (int32_t)bee.wx;
  int32_t by = (int32_t)bee.wy;

  if ((bx*bx + by*by) <= (HiveCfg::COLLECTION_RADIUS*HiveCfg::COLLECTION_RADIUS)) {
    beginUnload(nowMs);
  }
}

// -------------------- RESET --------------------
void resetHive() {
  hive.isUnloading = false;
  hive.unloadRemaining = 0;
  hive.unloadTotal = 0;
  hive.depositsTowardBoost = 0;
  hive.boostCharge = 0;
  hive.bonusPoints = 0.0f;
  hive.totalBonusCharges = 0;
  lastMagnetActivationThreshold = 0;
  hive.pollenMagnetActive = false;
  hive.bonusFlashUntilMs = 0;
  hive.magnet.active = false;
  hive.magnet.activeUntilMs = 0;
  hive.magnet.cooldownUntilMs = 0;
  hive.magnet.chargesConsumed = 0;
  hive.magnet.strength = 1.0f;
  for (int i = 0; i < Pool::BELT_ITEM_N; i++) beltItems[i].alive = 0;
}
