// Pixel Buzz Box - Main Entry Point
// Board: Raspberry Pi Pico (rp2040 core)
//
// Domain modules:
// - input.cpp    : Joystick and button handling
// - bee.cpp      : Bee position, physics, wings, boost
// - flowers.cpp  : Flower spawning, collection, targeting
// - hive.cpp     : Hive interaction, unloading, belt
// - radar.cpp    : Radar ping and targeting
// - vfx.cpp      : Trails, popups, camera, visual effects
// - survival.cpp : Timer, score, game over state
// - graphics.cpp : All rendering

#include "game.h"
#include "BuzzSynth.h"
#include <SPI.h>
#include <math.h>

// Global sound synthesizer
BuzzSynth buzzer(PIN_BUZZ);

// -------------------- SETUP --------------------
void setup() {
  // Backlight
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  // Joystick button
  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  // Audio
  buzzer.begin();

  // SPI display
  SPI.setSCK(PIN_SCK);
  SPI.setTX(PIN_MOSI);
  SPI.begin();

  tft.init(240, 320);
  tft.setRotation(1);

  // Seed RNG
  rngState ^= (uint32_t)analogRead(PIN_JOY_VRX) << 16;
  rngState ^= (uint32_t)analogRead(PIN_JOY_VRY) << 1;
  rngState ^= (uint32_t)micros();

  tft.fillScreen(COL_BG0);

  // Calibrate input
  calibrateJoystick();
  joyMinY = joyCenterY;
  joyMaxY = joyCenterY;

  // Initialize all domains
  resetBee();
  resetHive();
  resetVFX();
  resetSurvival();
  resetRadar();
  initFlowers();

  renderFrame(millis());
}

// -------------------- LOOP --------------------
void loop() {
  static uint32_t lastMs = millis();
  uint32_t now = millis();
  uint32_t dtMs = now - lastMs;
  if (dtMs > 60) dtMs = 60;
  lastMs = now;
  float dt = (float)dtMs / 1000.0f;

  static bool wasBoosting = false;

  if (!isUnloading) {
    // Read input
    float nx, ny;
    int rawDx, rawDy;
    readNormalizedJoystick(nx, ny, rawDx, rawDy);

    // Check boost state
    bool boosting = isBoosting(now);
    if (boosting && !wasBoosting) {
      triggerCameraShake(now, 6.5f, 180);
    }
    wasBoosting = boosting;

    // Update bee physics and animation
    updateBeePhysics(nx, ny, rawDx, rawDy, dt, boosting);
    updateWingAnimation(dt);

    // Boost trail VFX
    if (boosting && wingSpeed > 0.2f) {
      static uint32_t lastTrailMs = 0;
      if ((uint32_t)(now - lastTrailMs) > 20) {
        float spN = clampf((fabsf(beeVX) + fabsf(beeVY)) / WING_SPEED_DIVISOR, 0.0f, 1.0f);
        spawnTrailParticle(beeWX, beeWY, spN, now);
        spawnTrailParticle(beeWX - beeVX * 0.02f, beeWY - beeVY * 0.02f, spN, now);
        lastTrailMs = now;
      }
    }

    // Update VFX
    updateTrailParticles(now);
    updateScorePopups(now);
    updateCamera(dt, boosting, now);

  } else {
    // During unload: bee at hive, no movement
    wasBoosting = false;
    stopBeeMovement();

    float zoomLerp = clampf(7.0f * dt, 0.0f, 1.0f);
    cameraZoom += (1.0f - cameraZoom) * zoomLerp;
    cameraShakeX = 0.0f;
    cameraShakeY = 0.0f;

    updateBeltLifetimes(now);
    updateUnload(now);
    updateTrailParticles(now);
    updateScorePopups(now);
  }

  // Survival timer
  updateSurvivalTimer(dt, now);

  // Stop sounds on game over
  if (isGameOver) {
    static bool soundStopped = false;
    if (!soundStopped) {
      buzzer.stopAll();
      isUnloading = false;
      unloadRemaining = 0;
      unloadTotal = 0;
      soundStopped = true;
    }
    // Reset flag when game restarts
    if (survivalTimeLeft > 0.0f) soundStopped = false;
  }

  // Button handling
  bool edgeDown = false;
  if (!isUnloading) {
    edgeDown = readButtonEdge();
  } else {
    resetButtonState();
  }

  // Game over restart
  if (isGameOver && edgeDown) {
    // Reset all domains
    resetBee();
    resetHive();
    resetVFX();
    resetSurvival();
    resetRadar();
    initFlowers();

    // Reset sound state
    SoundState& snd = buzzer.getState();
    snd.prevVX = 0.0f;
    snd.prevVY = 0.0f;
    snd.heading = 0.0f;
    snd.turnRateSmooth = 0.0f;
    snd.accelSmooth = 0.0f;
    snd.radialAccelSmooth = 0.0f;
    snd.eventTailUntilMs = 0;
    snd.ambientEnv = 0.0f;
    snd.ambientFreqSmooth = 0.0f;
    snd.lastUnloadFreq = 0.0f;
  }

  // Normal game input
  if (!isGameOver && !isUnloading) {
    if (edgeDown) {
      if (!buzzer.soundBusy()) buzzer.startSound(SND_CLICK, now);
      beginRadarPing(now);
    }

    updateBeltLifetimes(now);
    updateRadar(now);
    tryCollectPollen(now);
    tryStoreAtHive(now);

    // Ambient wing buzz
    float speed = getBeeSpeed();
    buzzer.updateAmbient(now, dt, wingSpeed, beeVX, beeVY, speed);
    buzzer.updateSound(now);
  }

  // Render at adaptive cadence
  static uint32_t lastRenderMs = 0;
  uint32_t renderInterval = 40;
  bool boosting = isBoosting(now);
  bool idle = !isGameOver && !isUnloading && !radarActive && !boosting
              && (wingSpeed < 0.05f) && !anyTrailAlive() && !anyBeltAlive() && !anyScorePopupAlive();
  if (idle) renderInterval = 80;

  if ((uint32_t)(now - lastRenderMs) >= renderInterval) {
    lastRenderMs = now;
    renderFrame(now);
  }

  delay(2);
}
