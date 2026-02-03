// Pixel Buzz Box - Radar (Targeting and Ping)
#include "game.h"
#include "BuzzSynth.h"

extern BuzzSynth buzzer;

// -------------------- RADAR STATE --------------------
RadarState radar = {
  .active = false,
  .untilMs = 0,
  .targetWX = 0, .targetWY = 0,
  .toHive = false,
  .fullActive = false
};

// -------------------- RADAR PING --------------------
void beginRadarPing(uint32_t nowMs) {
  radar.active = true;
  radar.untilMs = nowMs + Timing::RADAR_DURATION_MS;

  if (survival.pollenCount > 0) {
    // Point to hive when carrying pollen
    radar.toHive = true;
    radar.targetWX = 0;
    radar.targetWY = 0;
  } else {
    // Point to nearest flower when empty
    radar.toHive = false;
    int32_t fx, fy;
    if (findNearestFlower(fx, fy)) {
      radar.targetWX = fx;
      radar.targetWY = fy;
    } else {
      // No flowers, point to hive
      radar.targetWX = 0;
      radar.targetWY = 0;
      radar.toHive = true;
    }
  }

  if (!buzzer.soundBusy()) buzzer.startSound(SND_RADAR, nowMs);
}

// -------------------- RADAR UPDATE --------------------
void updateRadar(uint32_t nowMs) {
  if (!radar.active) return;
  if ((int32_t)(nowMs - radar.untilMs) >= 0) {
    radar.active = false;
  }
}

// -------------------- FULL RADAR (AUTO) --------------------
void updateFullRadar() {
  // Activate persistent radar when at max pollen capacity
  if (survival.pollenCount >= Pool::MAX_POLLEN_CARRY) {
    radar.fullActive = true;
    radar.targetWX = 0;  // Hive is at origin
    radar.targetWY = 0;
    radar.toHive = true;
  } else {
    radar.fullActive = false;
  }
}

// -------------------- RESET --------------------
void resetRadar() {
  radar.active = false;
  radar.untilMs = 0;
  radar.targetWX = 0;
  radar.targetWY = 0;
  radar.toHive = false;
  radar.fullActive = false;
}
