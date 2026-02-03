// Pixel Buzz Box - Survival (Timer, Game Over, Scoring)
#include "game.h"

// -------------------- SURVIVAL STATE --------------------
SurvivalState survival = {
  .pollenCount = 0,
  .score = 0,
  .timeLeft = Survival::TIME_MAX,
  .isGameOver = false,
  .gameOverMs = 0,
  .flashUntilMs = 0,
  .flashStartPct = 0.0f,
  .flashEndPct = 0.0f,
  .dayPhase = DayNight::START_PHASE,
  .diedAtNight = false,
  .isNightResting = false,
  .nightRestStartMs = 0,
  .nightsSurvived = 0,
  .currentDay = 1,
  .isDayTransition = true,  // Start with day transition
  .dayTransitionStartMs = 0
};

// -------------------- DAY/NIGHT HELPERS --------------------
static bool isNightTime(float dayPhase) {
  return dayPhase < DayNight::DAWN_START || dayPhase >= DayNight::DUSK_END;
}

// -------------------- WORLD BOUNDARY (grows each day) --------------------
float getWorldBoundary() {
  // Day 1 = base, each subsequent day adds BOUNDARY_GROWTH
  float boundary = World::BOUNDARY_BASE + (survival.currentDay - 1) * World::BOUNDARY_GROWTH;
  return (boundary < World::BOUNDARY_MAX) ? boundary : World::BOUNDARY_MAX;
}

static bool isBeeAtHive() {
  int32_t bx = (int32_t)bee.wx;
  int32_t by = (int32_t)bee.wy;
  return (bx*bx + by*by) <= (HiveCfg::COLLECTION_RADIUS * HiveCfg::COLLECTION_RADIUS);
}

// -------------------- DAY TRANSITION --------------------
void beginDayTransition(uint32_t nowMs) {
  if (survival.isDayTransition) return;
  survival.isDayTransition = true;
  survival.dayTransitionStartMs = nowMs;
}

void updateDayTransition(uint32_t nowMs) {
  if (!survival.isDayTransition) return;

  uint32_t elapsed = nowMs - survival.dayTransitionStartMs;
  if (elapsed >= DayNight::DAY_TRANSITION_DURATION_MS) {
    survival.isDayTransition = false;
  }
}

bool isDayTransition() {
  return survival.isDayTransition;
}

float getDayTransitionProgress(uint32_t nowMs) {
  if (!survival.isDayTransition) return 0.0f;
  uint32_t elapsed = nowMs - survival.dayTransitionStartMs;
  return clampf((float)elapsed / (float)DayNight::DAY_TRANSITION_DURATION_MS, 0.0f, 1.0f);
}

// -------------------- NIGHT REST --------------------
void beginNightRest(uint32_t nowMs) {
  if (survival.isNightResting) return;
  survival.isNightResting = true;
  survival.nightRestStartMs = nowMs;
  survival.nightsSurvived++;
}

void updateNightRest(uint32_t nowMs) {
  if (!survival.isNightResting) return;

  uint32_t elapsed = nowMs - survival.nightRestStartMs;
  if (elapsed >= DayNight::NIGHT_REST_DURATION_MS) {
    // Night rest complete - new day begins at dawn!
    survival.isNightResting = false;
    survival.dayPhase = DayNight::DAWN_START;  // Wake up at dawn
    survival.diedAtNight = false;
    survival.currentDay++;  // Increment day counter

    // Bonus time for surviving the night
    addSurvivalTime(nowMs, DayNight::NIGHT_REST_TIME_BONUS);

    // Start day transition to show "DAY X"
    beginDayTransition(nowMs);
  }
}

bool isNightResting() {
  return survival.isNightResting;
}

float getNightRestProgress(uint32_t nowMs) {
  if (!survival.isNightResting) return 0.0f;
  uint32_t elapsed = nowMs - survival.nightRestStartMs;
  return clampf((float)elapsed / (float)DayNight::NIGHT_REST_DURATION_MS, 0.0f, 1.0f);
}

// -------------------- TIMER UPDATE --------------------
void updateSurvivalTimer(float dt, uint32_t nowMs) {
  if (survival.isGameOver) return;

  // Handle day transition animation (DAY X display)
  if (survival.isDayTransition) {
    updateDayTransition(nowMs);
    return;  // Time is paused during day transition!
  }

  // Handle night rest animation
  if (survival.isNightResting) {
    updateNightRest(nowMs);
    return;  // Time is paused during night rest!
  }

  // Pause survival timer during magnet (cinematic moment!)
  if (isMagnetActive(nowMs)) return;

  // Advance day/night cycle
  survival.dayPhase += dt / DayNight::CYCLE_DURATION;
  if (survival.dayPhase >= 1.0f) {
    survival.dayPhase -= 1.0f;  // Wrap around
  }

  // Check for night rest trigger: bee at hive during night
  if (isNightTime(survival.dayPhase) && isBeeAtHive()) {
    beginNightRest(nowMs);
    return;
  }

  // Night damage: bees out after dark lose time rapidly!
  if (isNightTime(survival.dayPhase)) {
    survival.timeLeft -= dt * DayNight::NIGHT_DAMAGE_RATE;
    survival.diedAtNight = true;  // Track that night contributed to death
  } else {
    survival.timeLeft -= dt;
    survival.diedAtNight = false;
  }

  if (survival.timeLeft <= 0.0f) {
    survival.timeLeft = 0.0f;
    survival.isGameOver = true;
    survival.gameOverMs = nowMs;
  }
}

// -------------------- TIME GAIN --------------------
float addSurvivalTime(uint32_t nowMs, float amount) {
  float before = survival.timeLeft;
  float after = clampf(survival.timeLeft + amount, 0.0f, Survival::TIME_MAX);
  float overage = (survival.timeLeft + amount) - after;
  survival.timeLeft = after;

  // Flash effect
  survival.flashStartPct = clampf(before / Survival::TIME_MAX, 0.0f, 1.0f);
  survival.flashEndPct = clampf(after / Survival::TIME_MAX, 0.0f, 1.0f);
  survival.flashUntilMs = nowMs + Timing::SURVIVAL_FLASH_MS;

  return overage;
}

// -------------------- TIME PENALTY (WASP HIT) --------------------
void applySurvivalPenalty(float amount) {
  survival.timeLeft = clampf(survival.timeLeft - amount, 0.0f, Survival::TIME_MAX);
}

// -------------------- POLLEN PENALTY (WASP HIT) --------------------
uint8_t applyPollenPenalty(uint8_t amount) {
  uint8_t lost = (amount > survival.pollenCount) ? survival.pollenCount : amount;
  survival.pollenCount -= lost;
  return lost;
}

// -------------------- RESET --------------------
void resetSurvival() {
  survival.pollenCount = 0;
  survival.score = 0;
  survival.timeLeft = Survival::TIME_MAX;
  survival.isGameOver = false;
  survival.gameOverMs = 0;
  survival.flashUntilMs = 0;
  survival.flashStartPct = 0.0f;
  survival.flashEndPct = 0.0f;
  survival.dayPhase = DayNight::START_PHASE;  // Start at mid-morning
  survival.diedAtNight = false;
  survival.isNightResting = false;
  survival.nightRestStartMs = 0;
  survival.nightsSurvived = 0;
  survival.currentDay = 1;
  survival.isDayTransition = true;  // Show "DAY 1" at game start
  survival.dayTransitionStartMs = millis();
}
