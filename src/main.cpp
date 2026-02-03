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
// - gfx/         : Graphics rendering (background, entities, effects, ui)

#include "game.h"
#include "gfx.h"
#include "BuzzSynth.h"
#include <SPI.h>
#include <math.h>

// -------------------- SHARED GLOBALS --------------------
uint32_t rngState = 0xA5A5F00Du;
Adafruit_ST7789 tft(&SPI, PIN_CS, PIN_DC, PIN_RST);
GFXcanvas16 canvas(Display::CANVAS_W, Display::CANVAS_H);

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

  tft.fillScreen(Color::BG0);

  // Calibrate input
  calibrateJoystick();
  input.minY = input.centerY;
  input.maxY = input.centerY;

  // Initialize high scores from flash
  initHighScores();

  // Initialize all domains
  resetBee();
  resetHive();
  resetVFX();
  resetSurvival();
  resetRadar();
  resetWasps();
  initFlowers();

  renderFrame(millis());
}

// -------------------- LOOP --------------------
void loop() {
  static uint32_t lastMs = millis();
  uint32_t now = millis();
  uint32_t dtMs = now - lastMs;
  if (dtMs > Timing::MAX_DELTA_MS) dtMs = Timing::MAX_DELTA_MS;
  lastMs = now;
  float dt = (float)dtMs / 1000.0f;

  static bool wasBoosting = false;

  if (!hive.isUnloading && !isNightResting() && !isDayTransition() && !isMagnetActive(now)) {
    // Read input
    float nx, ny;
    int rawDx, rawDy;
    readNormalizedJoystick(nx, ny, rawDx, rawDy);

    // Check boost state
    bool boosting = isBoosting(now);
    if (boosting && !wasBoosting) {
      triggerCameraShake(now, Camera::SHAKE_MAGNITUDE, Camera::SHAKE_DURATION_MS);
    }
    wasBoosting = boosting;

    // Update bee physics and animation
    updateBeePhysics(nx, ny, rawDx, rawDy, dt, boosting, now);
    updateWingAnimation(dt);

    // Boost trail VFX
    if (boosting && bee.wingSpeed > 0.2f) {
      static uint32_t lastTrailMs = 0;
      if ((uint32_t)(now - lastTrailMs) > Timing::TRAIL_SPAWN_INTERVAL_MS) {
        float spN = clampf((fabsf(bee.vx) + fabsf(bee.vy)) / Physics::WING_SPEED_DIVISOR, 0.0f, 1.0f);
        spawnTrailParticle(bee.wx, bee.wy, spN, now);
        spawnTrailParticle(bee.wx - bee.vx * 0.02f, bee.wy - bee.vy * 0.02f, spN, now);
        lastTrailMs = now;
      }
    }

    // Update VFX
    updateTrailParticles(now);
    updateScorePopups(now);
    updateCamera(dt, boosting, now);

  } else if (isNightResting() || isDayTransition()) {
    // During night rest or day transition: bee frozen, no inputs
    wasBoosting = false;
    stopBeeMovement();
    buzzer.stopAll();
    updateTrailParticles(now);
    updateScorePopups(now);

  } else if (isMagnetActive(now)) {
    // Magnet cinematic: slow bee movement, slow flower pull, everything else frozen
    float nx, ny;
    int rawDx, rawDy;
    readNormalizedJoystick(nx, ny, rawDx, rawDy);
    updateBeePhysics(nx, ny, rawDx, rawDy, dt, false, now);
    updateWingAnimation(dt);

    updateFlowerPhysics(dt, now);
    tryCollectPollen(now);

    // Keep ambient sound running
    float speed = getBeeSpeed();
    buzzer.updateAmbient(now, dt, bee.wingSpeed, bee.vx, bee.vy, speed);
    buzzer.updateSound(now);

  } else {
    // During unload: bee at hive, no movement
    wasBoosting = false;
    stopBeeMovement();

    float zoomLerp = clampf(Camera::ZOOM_LERP_SPEED * dt, 0.0f, 1.0f);
    camera.zoom += (1.0f - camera.zoom) * zoomLerp;
    camera.shakeX = 0.0f;
    camera.shakeY = 0.0f;

    updateBeltLifetimes(now);
    updateUnload(now);
    updateTrailParticles(now);
    updateScorePopups(now);
  }

  // Survival timer
  updateSurvivalTimer(dt, now);

  // Stop sounds on game over and check for high score
  static bool wasGameOver = false;
  static bool soundStopped = false;
  static bool highScoreChecked = false;

  // Reset flags when transitioning from game over to playing
  if (wasGameOver && !survival.isGameOver) {
    soundStopped = false;
    highScoreChecked = false;
  }
  wasGameOver = survival.isGameOver;

  if (survival.isGameOver) {
    if (!soundStopped) {
      buzzer.stopAll();
      hive.isUnloading = false;
      hive.unloadRemaining = 0;
      hive.unloadTotal = 0;
      soundStopped = true;
    }
    // Check for high score entry (once per game over)
    if (!highScoreChecked) {
      if (isHighScore(survival.score)) {
        beginHighScoreEntry(survival.score, survival.currentDay, survival.diedAtNight);
      }
      highScoreChecked = true;
    }
  }

  // Button handling
  bool edgeDown = false;
  if (!hive.isUnloading && !isNightResting() && !isDayTransition()) {
    edgeDown = readButtonEdge();
  } else {
    resetButtonState();
  }

  // High score entry input (during game over)
  if (survival.isGameOver && isHighScoreEntryActive()) {
    float nx, ny;
    int rawDx, rawDy;
    readNormalizedJoystick(nx, ny, rawDx, rawDy);
    // Convert normalized joystick to -100..100 range for entry input
    int8_t joyX = (int8_t)(nx * 100.0f);
    int8_t joyY = (int8_t)(ny * 100.0f);
    updateHighScoreEntry(edgeDown, joyX, joyY);
    // Check if save animation complete
    isHighScoreEntryComplete();
  }

  // Game over restart (only if high score entry is complete)
  if (survival.isGameOver && edgeDown && !isHighScoreEntryActive()) {
    // Reset all domains
    resetBee();
    resetHive();
    resetVFX();
    resetSurvival();
    resetRadar();
    resetWasps();
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

  // Normal game input (paused during magnet cinematic and night rest)
  if (!survival.isGameOver && !hive.isUnloading && !isMagnetActive(now) && !isNightResting() && !isDayTransition()) {
    if (edgeDown) {
      if (canActivateMagnet(now)) {
        // Priority 1: Magnet activation
        triggerMagnet(now);
      } else {
        // Priority 2: Radar ping fallback
        if (!buzzer.soundBusy()) buzzer.startSound(SND_CLICK, now);
        beginRadarPing(now);
      }
    }

    updateBeltLifetimes(now);
    updateRadar(now);
    updateFullRadar();
    updateMagnet(now);
    tryCollectPollen(now);
    tryStoreAtHive(now);

    // Wasp (predator) updates
    checkWaspSpawning(now);
    updateWasps(now, dt);
    if (checkWaspCollision(now)) {
      // Wasp hit! Apply penalties, stun bee, and play sound
      // Extra time penalty if carrying no pollen
      float timePenalty = (survival.pollenCount == 0)
        ? WaspCfg::TIME_PENALTY_NO_POLLEN
        : WaspCfg::TIME_PENALTY;
      applySurvivalPenalty(timePenalty);
      applyPollenPenalty(WaspCfg::POLLEN_PENALTY);
      stunBee(now);
      buzzer.startSound(SND_WASP_HIT, now);
      triggerCameraShake(now, 8.0f, 250);
    }

    // Ambient wing buzz
    float speed = getBeeSpeed();
    buzzer.updateAmbient(now, dt, bee.wingSpeed, bee.vx, bee.vy, speed);
    buzzer.updateSound(now);
  }

  // Render at adaptive cadence
  static uint32_t lastRenderMs = 0;
  uint32_t renderInterval = Timing::RENDER_INTERVAL_ACTIVE_MS;
  bool boosting = isBoosting(now);
  bool idle = !survival.isGameOver && !hive.isUnloading && !radar.active && !radar.fullActive && !boosting
              && (bee.wingSpeed < 0.05f) && !anyTrailAlive() && !anyBeltAlive() && !anyScorePopupAlive();
  if (idle) renderInterval = Timing::RENDER_INTERVAL_IDLE_MS;

  if ((uint32_t)(now - lastRenderMs) >= renderInterval) {
    lastRenderMs = now;
    renderFrame(now);
  }

  delay(Timing::LOOP_DELAY_MS);
}
