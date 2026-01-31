// BuzzSynth - Dynamic Buzzer Sound Synthesizer
// A reusable audio library for piezo buzzer sound effects
#ifndef BUZZSYNTH_H
#define BUZZSYNTH_H

#include <Arduino.h>

// Sound modes
enum SndMode : uint8_t {
  SND_IDLE = 0,
  SND_CLICK,
  SND_RADAR,
  SND_POLLEN_CHIRP,
  SND_POWERUP,
};

// Sound state structure
struct SoundState {
  SndMode mode;
  uint8_t step;
  uint32_t nextMs;
  float lastEventFreq;
  uint32_t eventTailUntilMs;
  uint32_t eventTailStartMs;
  float eventTailFreq;
  float ambientFreqSmooth;
  float ambientEnv;
  float prevVX;
  float prevVY;
  float heading;
  float turnRateSmooth;
  float accelSmooth;
  float radialAccelSmooth;
  float vibratoPhase;
  uint32_t swishUntilMs;
  uint32_t swishStartMs;
  float swishSign;
  uint32_t accelPulseUntilMs;
  uint32_t accelPulseStartMs;
  float accelPulseStrength;
  float lastUnloadFreq;
};

class BuzzSynth {
public:
  BuzzSynth(int buzzerPin);

  // Initialize the synthesizer
  void begin();

  // Start a sound effect
  void startSound(SndMode mode, uint32_t nowMs);

  // Update sound state machine (call every frame)
  void updateSound(uint32_t nowMs);

  // Check if a sound effect is playing
  bool soundBusy() const;

  // Update ambient wing buzz based on movement
  void updateAmbient(uint32_t nowMs, float dt, float wingSpeed,
                     float vx, float vy, float speed);

  // Stop all sounds
  void stopAll();

  // Play unload chirp tone
  void playUnloadTone(uint16_t freq, uint16_t durationMs);

  // Set event tail for smooth transitions
  void setEventTail(uint32_t nowMs, float freq, uint32_t durationMs);

  // Get sound state for direct manipulation if needed
  SoundState& getState() { return snd; }
  const SoundState& getState() const { return snd; }

private:
  int _pin;
  SoundState snd;

  // Internal clamp helper
  static float clampf(float v, float lo, float hi);
  static int clampi(int v, int lo, int hi);
};

#endif // BUZZSYNTH_H
