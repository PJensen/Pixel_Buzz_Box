// Pixel Buzz Box - Game Header (Domain Declarations)
#pragma once

#include "config.h"
#include "highscore.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ==================== SHARED GLOBALS (state.cpp) ====================
extern Adafruit_ST7789 tft;
extern GFXcanvas16 canvas;

// ==================== INPUT (input.cpp) ====================
extern InputState input;

int readJoyX();
int readJoyY();
bool joyPressedRaw();
int applyDeadzone(int v, int center, int dz);
void calibrateJoystick();
void readNormalizedJoystick(float &nx, float &ny, int &rawDx, int &rawDy);
bool readButtonEdge();
void resetButtonState();
bool checkTripleClick(uint32_t nowMs);
void resetTripleClickState();

// ==================== BEE (bee.cpp) ====================
extern BeeState bee;

bool isBoosting(uint32_t nowMs);
bool isBoostOnCooldown(uint32_t nowMs);
bool isBeeStunned(uint32_t nowMs);
void stunBee(uint32_t nowMs);
void triggerAutoBoost(uint32_t nowMs);
void triggerManualBoost(uint32_t nowMs);
void updateBeePhysics(float nx, float ny, int rawDx, int rawDy, float dt, bool boosting, uint32_t nowMs);
void updateWingAnimation(float dt);
void resetBee();
void stopBeeMovement();
float getBeeSpeed();

// ==================== FLOWERS (flowers.cpp) ====================
extern Flower flowers[Pool::FLOWER_N];
extern uint32_t flowerBornMs[Pool::FLOWER_N];

void initFlowerStyle(Flower &f);
void spawnFlowerAt(int i, int32_t wx, int32_t wy, FlowerType type = FLOWER_NORMAL);
void spawnFlowerNearOrigin(int i);
void spawnFlowerElsewhere(int i);
void initFlowers();
bool tryCollectPollen(uint32_t nowMs);
bool findNearestFlower(int32_t &outWX, int32_t &outWY);
void updateFlowerPhysics(float dt, uint32_t nowMs);

// ==================== HIVE (hive.cpp) ====================
extern HiveState hive;
extern BeltItem beltItems[Pool::BELT_ITEM_N];

void spawnBeltItem(uint32_t nowMs);
void updateBeltLifetimes(uint32_t nowMs);
bool anyBeltAlive();
void beginUnload(uint32_t nowMs);
void updateUnload(uint32_t nowMs);
void tryStoreAtHive(uint32_t nowMs);
void updateBonusSystem();
float getScoreMultiplier();
bool isMagnetActive(uint32_t nowMs);
bool isMagnetOnCooldown(uint32_t nowMs);
bool canActivateMagnet(uint32_t nowMs);
void triggerMagnet(uint32_t nowMs);
void updateMagnet(uint32_t nowMs);
void resetHive();

// ==================== RADAR (radar.cpp) ====================
extern RadarState radar;

void beginRadarPing(uint32_t nowMs);
void updateRadar(uint32_t nowMs);
void updateFullRadar();
void resetRadar();

// ==================== VFX (vfx.cpp) ====================
extern TrailParticle trail[Pool::TRAIL_MAX];
extern int trailNextIdx;
extern ScorePopup scorePopups[Pool::SCORE_POPUP_N];
extern CameraState camera;
extern uint32_t hivePulseUntilMs;

int beeScreenCX();
int beeScreenCY();
void triggerCameraShake(uint32_t nowMs, float magnitude, uint32_t durationMs);
void updateCamera(float dt, bool boosting, uint32_t nowMs);
void resetCamera();
void worldToScreen(int32_t wx, int32_t wy, int &sx, int &sy);
void worldToScreenF(float wx, float wy, int &sx, int &sy);
uint32_t worldCellSeed(int32_t cx, int32_t cy, uint32_t salt);
void spawnTrailParticle(float wx, float wy, float speedN, uint32_t nowMs, int8_t forceVariant = -1);
void updateTrailParticles(uint32_t nowMs);
bool anyTrailAlive();
void spawnScorePopup(uint32_t nowMs, uint8_t pollen, float mult, int screenX, int screenY);
void updateScorePopups(uint32_t nowMs);
bool anyScorePopupAlive();
void triggerHivePulse(uint32_t nowMs);
void resetVFX();

// ==================== SURVIVAL (survival.cpp) ====================
extern SurvivalState survival;

void updateSurvivalTimer(float dt, uint32_t nowMs);
float addSurvivalTime(uint32_t nowMs, float amount);
void applySurvivalPenalty(float amount);
uint8_t applyPollenPenalty(uint8_t amount);
void resetSurvival();

// Night rest (safe at hive during night)
void beginNightRest(uint32_t nowMs);
void updateNightRest(uint32_t nowMs);
bool isNightResting();
float getNightRestProgress(uint32_t nowMs);

// Day transition (DAY X display)
void beginDayTransition(uint32_t nowMs);
void updateDayTransition(uint32_t nowMs);
bool isDayTransition();
float getDayTransitionProgress(uint32_t nowMs);

// ==================== WASP (wasp.cpp) ====================
extern Wasp wasps[Pool::WASP_MAX];
extern uint32_t beeInvulnUntilMs;

void checkWaspSpawning(uint32_t nowMs);
void updateWasps(uint32_t nowMs, float dt);
bool checkWaspCollision(uint32_t nowMs);
int killOnScreenWasps(uint32_t nowMs);
bool isAnyWaspHunting();
bool isBeeInvulnerable(uint32_t nowMs);
void resetWasps();

// ==================== GRAPHICS (src/gfx/) ====================
// See include/gfx.h for renderFrame()
