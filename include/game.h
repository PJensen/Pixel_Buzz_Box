// Pixel Buzz Box - Game Header (Domain Declarations)
#pragma once

#include "config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ==================== SHARED GLOBALS (state.cpp) ====================
extern Adafruit_ST7789 tft;
extern GFXcanvas16 canvas;

// ==================== INPUT (input.cpp) ====================
extern int joyCenterX, joyCenterY;
extern int joyMinY, joyMaxY;
extern bool btnPrev;

int readJoyX();
int readJoyY();
bool joyPressedRaw();
int applyDeadzone(int v, int center, int dz);
void calibrateJoystick();
void readNormalizedJoystick(float &nx, float &ny, int &rawDx, int &rawDy);
bool readButtonEdge();
void resetButtonState();

// ==================== BEE (bee.cpp) ====================
extern float beeWX, beeWY;
extern float beeVX, beeVY;
extern float wingPhase;
extern float wingSpeed;
extern uint32_t boostActiveUntilMs;
extern uint32_t boostCooldownUntilMs;

bool isBoosting(uint32_t nowMs);
bool isBoostOnCooldown(uint32_t nowMs);
void triggerAutoBoost(uint32_t nowMs);
void triggerManualBoost(uint32_t nowMs);
void updateBeePhysics(float nx, float ny, int rawDx, int rawDy, float dt, bool boosting);
void updateWingAnimation(float dt);
void resetBee();
void stopBeeMovement();
float getBeeSpeed();

// ==================== FLOWERS (flowers.cpp) ====================
extern Flower flowers[FLOWER_N];
extern uint32_t flowerBornMs[FLOWER_N];

void initFlowerStyle(Flower &f);
void spawnFlowerAt(int i, int32_t wx, int32_t wy);
void spawnFlowerNearOrigin(int i);
void spawnFlowerElsewhere(int i);
void initFlowers();
bool tryCollectPollen(uint32_t nowMs);
bool findNearestFlower(int32_t &outWX, int32_t &outWY);

// ==================== HIVE (hive.cpp) ====================
extern bool isUnloading;
extern uint8_t unloadRemaining;
extern uint8_t unloadTotal;
extern uint32_t unloadNextMs;
extern uint8_t depositsTowardBoost;
extern uint8_t boostCharge;
extern BeltItem beltItems[BELT_ITEM_N];

void spawnBeltItem(uint32_t nowMs);
void updateBeltLifetimes(uint32_t nowMs);
bool anyBeltAlive();
void beginUnload(uint32_t nowMs);
void updateUnload(uint32_t nowMs);
void tryStoreAtHive(uint32_t nowMs);
void resetHive();

// ==================== RADAR (radar.cpp) ====================
extern bool radarActive;
extern uint32_t radarUntilMs;
extern int32_t radarTargetWX, radarTargetWY;
extern bool radarToHive;

void beginRadarPing(uint32_t nowMs);
void updateRadar(uint32_t nowMs);
void resetRadar();

// ==================== VFX (vfx.cpp) ====================
extern TrailParticle trail[TRAIL_MAX];
extern int trailNextIdx;
extern ScorePopup scorePopups[SCORE_POPUP_N];
extern float cameraZoom;
extern float cameraShakeX, cameraShakeY;
extern uint32_t cameraShakeUntilMs;
extern uint32_t cameraShakeDurationMs;
extern float cameraShakeMagnitude;
extern uint32_t hivePulseUntilMs;

int beeScreenCX();
int beeScreenCY();
void triggerCameraShake(uint32_t nowMs, float magnitude, uint32_t durationMs);
void updateCamera(float dt, bool boosting, uint32_t nowMs);
void resetCamera();
void worldToScreen(int32_t wx, int32_t wy, int &sx, int &sy);
void worldToScreenF(float wx, float wy, int &sx, int &sy);
uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt);
void spawnTrailParticle(float wx, float wy, float speedN, uint32_t nowMs);
void updateTrailParticles(uint32_t nowMs);
bool anyTrailAlive();
void spawnScorePopup(uint32_t nowMs, uint8_t value, int sx, int sy);
void updateScorePopups(uint32_t nowMs);
bool anyScorePopupAlive();
void triggerHivePulse(uint32_t nowMs);
void resetVFX();

// ==================== SURVIVAL (survival.cpp) ====================
extern uint8_t pollenCount;
extern uint16_t score;
extern float survivalTimeLeft;
extern bool isGameOver;
extern uint32_t gameOverMs;
extern uint32_t survivalFlashUntilMs;
extern float survivalFlashStartPct;
extern float survivalFlashEndPct;

void updateSurvivalTimer(float dt, uint32_t nowMs);
void addSurvivalTime(uint32_t nowMs, float amount);
void resetSurvival();

// ==================== GRAPHICS (graphics.cpp) ====================
void renderFrame(uint32_t nowMs);
