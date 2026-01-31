// Pixel Buzz Box - Shared Globals (Display and RNG only)
#include "game.h"
#include <SPI.h>

// -------------------- RNG STATE --------------------
uint32_t rngState = 0xA5A5F00Du;

// -------------------- DISPLAY --------------------
Adafruit_ST7789 tft(&SPI, PIN_CS, PIN_DC, PIN_RST);
GFXcanvas16 canvas(CANVAS_W, CANVAS_H);
