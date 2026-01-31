// Pixel Buzz Box - Radar (Targeting and Ping)
#include "game.h"
#include "BuzzSynth.h"

extern BuzzSynth buzzer;

// -------------------- RADAR STATE --------------------
bool radarActive = false;
uint32_t radarUntilMs = 0;
int32_t radarTargetWX = 0;
int32_t radarTargetWY = 0;
bool radarToHive = false;

// -------------------- RADAR PING --------------------
void beginRadarPing(uint32_t nowMs) {
  radarActive = true;
  radarUntilMs = nowMs + RADAR_DURATION_MS;

  if (pollenCount > 0) {
    // Point to hive when carrying pollen
    radarToHive = true;
    radarTargetWX = 0;
    radarTargetWY = 0;
  } else {
    // Point to nearest flower when empty
    radarToHive = false;
    int32_t fx, fy;
    if (findNearestFlower(fx, fy)) {
      radarTargetWX = fx;
      radarTargetWY = fy;
    } else {
      // No flowers, point to hive
      radarTargetWX = 0;
      radarTargetWY = 0;
      radarToHive = true;
    }
  }

  if (!buzzer.soundBusy()) buzzer.startSound(SND_RADAR, nowMs);
}

// -------------------- RADAR UPDATE --------------------
void updateRadar(uint32_t nowMs) {
  if (!radarActive) return;
  if ((int32_t)(nowMs - radarUntilMs) >= 0) {
    radarActive = false;
  }
}

// -------------------- RESET --------------------
void resetRadar() {
  radarActive = false;
  radarUntilMs = 0;
  radarTargetWX = 0;
  radarTargetWY = 0;
  radarToHive = false;
}
