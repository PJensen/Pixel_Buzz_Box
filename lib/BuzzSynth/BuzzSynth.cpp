// BuzzSynth - Dynamic Buzzer Sound Synthesizer Implementation
#include "BuzzSynth.h"
#include <math.h>

// Hash function for jitter
static uint32_t hash32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return x;
}

BuzzSynth::BuzzSynth(int buzzerPin) : _pin(buzzerPin) {
  memset(&snd, 0, sizeof(snd));
  snd.mode = SND_IDLE;
}

void BuzzSynth::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

float BuzzSynth::clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

int BuzzSynth::clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void BuzzSynth::startSound(SndMode mode, uint32_t nowMs) {
  snd.mode = mode;
  snd.step = 0;
  snd.nextMs = nowMs;
  snd.eventTailUntilMs = 0;
}

void BuzzSynth::updateSound(uint32_t nowMs) {
  if (snd.mode == SND_IDLE) return;
  if ((int32_t)(nowMs - snd.nextMs) < 0) return;

  switch (snd.mode) {
    case SND_CLICK:
      if (snd.step == 0) {
        snd.lastEventFreq = 1800.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 18);
        snd.nextMs = nowMs + 22;
        snd.step++;
      } else {
        noTone(_pin);
        snd.eventTailFreq = snd.lastEventFreq;
        snd.eventTailStartMs = nowMs;
        snd.eventTailUntilMs = nowMs + 110;
        snd.mode = SND_IDLE;
      }
      break;

    case SND_RADAR:
      if (snd.step == 0) {
        snd.lastEventFreq = 1500.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 14);
        snd.nextMs = nowMs + 18;
        snd.step++;
      } else if (snd.step == 1) {
        snd.lastEventFreq = 980.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 50);
        snd.nextMs = nowMs + 60;
        snd.step++;
      } else if (snd.step == 2) {
        snd.lastEventFreq = 1220.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 55);
        snd.nextMs = nowMs + 70;
        snd.step++;
      } else {
        noTone(_pin);
        snd.eventTailFreq = snd.lastEventFreq;
        snd.eventTailStartMs = nowMs;
        snd.eventTailUntilMs = nowMs + 140;
        snd.mode = SND_IDLE;
      }
      break;

    case SND_POLLEN_CHIRP:
      if (snd.step == 0) {
        snd.lastEventFreq = 940.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 55);
        snd.nextMs = nowMs + 65;
        snd.step++;
      } else if (snd.step == 1) {
        snd.lastEventFreq = 1160.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 55);
        snd.nextMs = nowMs + 65;
        snd.step++;
      } else if (snd.step == 2) {
        snd.lastEventFreq = 860.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 80);
        snd.nextMs = nowMs + 95;
        snd.step++;
      } else {
        noTone(_pin);
        snd.eventTailFreq = snd.lastEventFreq;
        snd.eventTailStartMs = nowMs;
        snd.eventTailUntilMs = nowMs + 130;
        snd.mode = SND_IDLE;
      }
      break;

    case SND_POWERUP:
      if (snd.step == 0) {
        snd.lastEventFreq = 780.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 70);
        snd.nextMs = nowMs + 78;
        snd.step++;
      } else if (snd.step == 1) {
        snd.lastEventFreq = 1080.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 70);
        snd.nextMs = nowMs + 78;
        snd.step++;
      } else if (snd.step == 2) {
        snd.lastEventFreq = 1420.0f;
        tone(_pin, (uint16_t)snd.lastEventFreq, 90);
        snd.nextMs = nowMs + 105;
        snd.step++;
      } else {
        noTone(_pin);
        snd.eventTailFreq = snd.lastEventFreq;
        snd.eventTailStartMs = nowMs;
        snd.eventTailUntilMs = nowMs + 160;
        snd.mode = SND_IDLE;
      }
      break;

    default:
      snd.mode = SND_IDLE;
      noTone(_pin);
      break;
  }
}

bool BuzzSynth::soundBusy() const {
  return snd.mode != SND_IDLE;
}

void BuzzSynth::updateAmbient(uint32_t nowMs, float dt, float wingSpeed,
                               float vx, float vy, float speed) {
  if (soundBusy()) return;

  // Calculate heading and turn rate
  float heading = snd.heading;
  if (speed > 0.02f) heading = atan2f(vy, vx);
  float dHeading = heading - snd.heading;
  while (dHeading > 3.1415926f) dHeading -= 6.2831853f;
  while (dHeading < -3.1415926f) dHeading += 6.2831853f;
  float turnRate = (dt > 0.0001f) ? (dHeading / dt) : 0.0f;
  snd.heading = heading;

  // Calculate acceleration
  float dvx = vx - snd.prevVX;
  float dvy = vy - snd.prevVY;
  float accelMag = (dt > 0.0001f) ? (sqrtf(dvx * dvx + dvy * dvy) / dt) : 0.0f;
  float accelAlong = 0.0f;
  if (speed > 0.02f && dt > 0.0001f) {
    accelAlong = (dvx * vx + dvy * vy) / (speed * dt);
  }

  // Smooth values
  float smoothRate = clampf(6.0f * dt, 0.0f, 1.0f);
  snd.turnRateSmooth += (turnRate - snd.turnRateSmooth) * smoothRate;
  snd.accelSmooth += (accelMag - snd.accelSmooth) * clampf(4.0f * dt, 0.0f, 1.0f);
  snd.radialAccelSmooth += (accelAlong - snd.radialAccelSmooth) * clampf(5.0f * dt, 0.0f, 1.0f);

  // Swish on sharp turns
  if (fabsf(turnRate) > 3.2f && (int32_t)(nowMs - snd.swishUntilMs) > 0) {
    snd.swishStartMs = nowMs;
    snd.swishUntilMs = nowMs + 120;
    snd.swishSign = (turnRate >= 0.0f) ? 1.0f : -1.0f;
  }

  // Acceleration pulse
  float accelN = clampf(accelMag / 420.0f, 0.0f, 1.0f);
  if (accelN > 0.35f && (int32_t)(nowMs - snd.accelPulseUntilMs) > 0) {
    snd.accelPulseStartMs = nowMs;
    snd.accelPulseUntilMs = nowMs + 140;
    snd.accelPulseStrength = accelN;
  }

  snd.prevVX = vx;
  snd.prevVY = vy;

  // Calculate ambient envelope
  bool tailActive = (int32_t)(nowMs - snd.eventTailUntilMs) < 0;
  float envTarget = (wingSpeed > 0.05f || tailActive) ? 1.0f : 0.0f;
  float envRate = (envTarget > snd.ambientEnv) ? 8.0f : 4.0f;
  snd.ambientEnv += (envTarget - snd.ambientEnv) * clampf(envRate * dt, 0.0f, 1.0f);

  // Calculate frequency
  float base = 220.0f + wingSpeed * 520.0f;
  uint32_t jitterSeed = hash32((uint32_t)(nowMs >> 2) + 0x5f3759dfu);
  float jitter = ((int)(jitterSeed & 0x7u) - 3) * 2.2f;

  float turnSkew = clampf(snd.turnRateSmooth * 0.75f, -22.0f, 22.0f);
  float doppler = clampf(snd.radialAccelSmooth * 0.06f, -22.0f, 22.0f);

  float vibRate = 7.5f + clampf(fabsf(snd.turnRateSmooth) * 0.14f, 0.0f, 6.5f);
  float vibDepth = 3.5f + wingSpeed * 8.0f
    + clampf(snd.accelSmooth * 0.045f, 0.0f, 10.0f)
    + clampf(fabsf(snd.turnRateSmooth) * 0.22f, 0.0f, 7.0f);
  snd.vibratoPhase += 6.2831853f * vibRate * dt;
  float vib = sinf(snd.vibratoPhase) * vibDepth;

  float swish = 0.0f;
  if ((int32_t)(nowMs - snd.swishUntilMs) < 0) {
    float t = (float)(nowMs - snd.swishStartMs) / (float)(snd.swishUntilMs - snd.swishStartMs);
    t = clampf(t, 0.0f, 1.0f);
    float env = 1.0f - fabsf(1.0f - 2.0f * t);
    swish = snd.swishSign * 18.0f * env;
  }

  float accelPulse = 0.0f;
  if ((int32_t)(nowMs - snd.accelPulseUntilMs) < 0) {
    float t = (float)(nowMs - snd.accelPulseStartMs) / (float)(snd.accelPulseUntilMs - snd.accelPulseStartMs);
    t = clampf(t, 0.0f, 1.0f);
    float env = 1.0f - t;
    accelPulse = snd.accelPulseStrength * 20.0f * env;
  }

  float target = base + jitter + turnSkew + doppler + vib + swish + accelPulse;

  // Blend with event tail
  if (tailActive && snd.eventTailUntilMs > snd.eventTailStartMs) {
    float t = (float)(nowMs - snd.eventTailStartMs) / (float)(snd.eventTailUntilMs - snd.eventTailStartMs);
    t = clampf(t, 0.0f, 1.0f);
    float blend = t * t;
    target = snd.eventTailFreq * (1.0f - blend) + target * blend;
  }

  snd.ambientFreqSmooth += (target - snd.ambientFreqSmooth) * clampf(10.0f * dt, 0.0f, 1.0f);

  // Output
  if (snd.ambientEnv > 0.05f) {
    int freq = clampi((int)(snd.ambientFreqSmooth), 180, 980);
    tone(_pin, (uint16_t)freq);
  } else {
    noTone(_pin);
  }
}

void BuzzSynth::stopAll() {
  noTone(_pin);
  snd.mode = SND_IDLE;
  snd.eventTailUntilMs = 0;
  snd.ambientEnv = 0.0f;
  snd.ambientFreqSmooth = 0.0f;
  snd.lastUnloadFreq = 0.0f;
}

void BuzzSynth::playUnloadTone(uint16_t freq, uint16_t durationMs) {
  snd.lastUnloadFreq = (float)freq;
  tone(_pin, freq, durationMs);
}

void BuzzSynth::setEventTail(uint32_t nowMs, float freq, uint32_t durationMs) {
  snd.eventTailFreq = freq;
  snd.eventTailStartMs = nowMs;
  snd.eventTailUntilMs = nowMs + durationMs;
}
