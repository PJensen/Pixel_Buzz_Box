// Pixel Buzz Box - Graphics Internal Declarations
// Shared between gfx/*.cpp files only - not part of public API
#pragma once

#include "game.h"
#include <Adafruit_GFX.h>
#include <math.h>
#include <stdio.h>

// -------------------- BACKGROUND (background.cpp) --------------------
// Sky and celestial bodies (drawn behind HUD in top zone)
void gfx_drawSkyZone(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);
void gfx_drawGroundFill(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy);
void gfx_drawBoundaryZone(Adafruit_GFX &g, int ox, int oy);

// Outdoor layers
void gfx_drawCloudLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy,
                        float parallax, int cell, uint32_t salt, uint32_t nowMs);
void gfx_drawGrassLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);
void gfx_drawPollenLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);
void gfx_drawScreenAnchor(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
void gfx_drawNightWarning(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);
void gfx_drawNightRestOverlay(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);
void gfx_drawDayTransitionOverlay(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);

// Legacy stubs (redirect to new outdoor functions)
void gfx_drawWorldGrid(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy);
void gfx_drawStarLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy,
                       float parallax, int cell, uint16_t cA, uint16_t cB, uint32_t salt);
void gfx_drawNebulaLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs);

// -------------------- ENTITIES (entities.cpp) --------------------
void gfx_drawBee(Adafruit_GFX &g, int x, int y);
void gfx_drawBeeShadow(Adafruit_GFX &g, int x, int y);
void gfx_drawBoostAura(Adafruit_GFX &g, int x, int y, uint32_t nowMs);
void gfx_drawPollenSparkles(Adafruit_GFX &g, int x, int y, uint32_t nowMs);
void gfx_drawHive(Adafruit_GFX &g, int x, int y);
void gfx_drawHivePulse(Adafruit_GFX &g, int x, int y, uint32_t nowMs);
void gfx_drawFlower(Adafruit_GFX &g, int x, int y, const Flower &f, uint32_t nowMs, uint32_t bornMs);
void gfx_drawWasp(Adafruit_GFX &g, int x, int y, const Wasp &w, uint32_t nowMs);
void gfx_drawWaspDangerIndicator(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);

// -------------------- PARTICLES (particles.cpp) --------------------
void gfx_drawTrailParticles(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
void gfx_drawScorePopups(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);

// -------------------- OVERLAYS (overlays.cpp) --------------------
void gfx_drawBeeTether(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
void gfx_drawRadarOverlay(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
void gfx_drawMagneticField(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);

// -------------------- UI (ui.cpp) --------------------
void gfx_drawHUDInTile(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy);
void gfx_drawBeltHUD(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
void gfx_drawSurvivalBar(Adafruit_GFX &g, int ox, int oy);
void gfx_drawGameOver(Adafruit_GFX &g, int ox, int oy);
void gfx_drawHighScoreEntry(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
void gfx_drawHighScoreTable(Adafruit_GFX &g, int ox, int oy, int8_t highlightRank);
