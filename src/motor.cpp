// Pixel Buzz Box - Haptic Motor Driver
// Non-blocking PWM vibration on GP13 via motor driver circuit

#include "game.h"

static uint32_t motorOffMs = 0;
static uint32_t rampStartMs = 0;
static uint32_t rampEndMs = 0;

void motorBegin() {
  pinMode(PIN_MOTOR, OUTPUT);
  analogWriteFreq(Motor::PWM_FREQ);
  analogWrite(PIN_MOTOR, 0);
}

void motorBuzz(uint32_t durationMs, uint32_t nowMs) {
  rampStartMs = 0;
  rampEndMs = 0;
  analogWrite(PIN_MOTOR, Motor::PWM_DUTY);
  motorOffMs = nowMs + durationMs;
}

void motorRamp(uint32_t nowMs) {
  motorOffMs = 0;
  rampStartMs = nowMs;
  rampEndMs = nowMs + Motor::STARTUP_RAMP_MS;
}

void motorUpdate(uint32_t nowMs) {
  // Ramp mode: linearly increase duty then cut sharp
  if (rampEndMs) {
    if ((int32_t)(nowMs - rampEndMs) >= 0) {
      // Ramp finished — hard stop
      analogWrite(PIN_MOTOR, 0);
      rampStartMs = 0;
      rampEndMs = 0;
    } else {
      float t = (float)(nowMs - rampStartMs) / (float)Motor::STARTUP_RAMP_MS;
      if (t > 1.0f) t = 1.0f;
      uint8_t duty = (uint8_t)(t * Motor::PWM_DUTY);
      analogWrite(PIN_MOTOR, duty);
    }
    return;
  }

  // Fixed-duration buzz mode
  if (motorOffMs && (int32_t)(nowMs - motorOffMs) >= 0) {
    analogWrite(PIN_MOTOR, 0);
    motorOffMs = 0;
  }
}

void motorStop() {
  analogWrite(PIN_MOTOR, 0);
  motorOffMs = 0;
  rampStartMs = 0;
  rampEndMs = 0;
}
