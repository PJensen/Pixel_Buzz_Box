// Pixel Buzz Box - Hardware Pin Definitions
// GPIO mappings for Raspberry Pi Pico
#pragma once

#include <Arduino.h>

// -------------------- LCD (SPI) --------------------
static const int PIN_BL   = 16;   // Backlight
static const int PIN_CS   = 17;   // Chip Select
static const int PIN_SCK  = 18;   // SPI Clock
static const int PIN_MOSI = 19;   // SPI Data (MOSI)
static const int PIN_DC   = 20;   // Data/Command
static const int PIN_RST  = 21;   // Reset

// -------------------- JOYSTICK --------------------
static const int PIN_JOY_SW  = 22;  // GP22 (phys 29) - Button
static const int PIN_JOY_VRX = 26;  // GP26/ADC0 (phys 31) - X Axis
static const int PIN_JOY_VRY = 27;  // GP27/ADC1 (phys 32) - Y Axis

// -------------------- BUZZER --------------------
static const int PIN_BUZZ = 15;     // GP15 (phys 20)
