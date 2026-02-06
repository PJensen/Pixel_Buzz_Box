// Pixel Buzz Box - Background Rendering
// Outdoor environment: sky, sun/moon, clouds, grass, pollen
#include "gfx_internal.h"

// -------------------- COLOR INTERPOLATION --------------------
static uint16_t lerpColor565(uint16_t c1, uint16_t c2, float t) {
  if (t <= 0.0f) return c1;
  if (t >= 1.0f) return c2;

  uint8_t r1 = (c1 >> 11) & 0x1F;
  uint8_t g1 = (c1 >> 5) & 0x3F;
  uint8_t b1 = c1 & 0x1F;

  uint8_t r2 = (c2 >> 11) & 0x1F;
  uint8_t g2 = (c2 >> 5) & 0x3F;
  uint8_t b2 = c2 & 0x1F;

  uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
  uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
  uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);

  return (r << 11) | (g << 5) | b;
}

// Get sky colors based on day phase
static void getSkyColors(float dayPhase, uint16_t &topColor, uint16_t &botColor) {
  // dayPhase: 0.0 = midnight, 0.25 = dawn, 0.5 = noon, 0.75 = dusk

  if (dayPhase < DayNight::DAWN_START) {
    // Night
    topColor = Color::SKY_NIGHT_TOP;
    botColor = Color::SKY_NIGHT_BOT;
  } else if (dayPhase < DayNight::DAWN_END) {
    // Dawn transition
    float t = (dayPhase - DayNight::DAWN_START) / (DayNight::DAWN_END - DayNight::DAWN_START);
    topColor = lerpColor565(Color::SKY_NIGHT_TOP, Color::SKY_DAWN_TOP, t);
    botColor = lerpColor565(Color::SKY_NIGHT_BOT, Color::SKY_DAWN_BOT, t);
    // Transition from dawn to day
    if (t > 0.5f) {
      float t2 = (t - 0.5f) * 2.0f;
      topColor = lerpColor565(Color::SKY_DAWN_TOP, Color::SKY_DAY_TOP, t2);
      botColor = lerpColor565(Color::SKY_DAWN_BOT, Color::SKY_DAY_BOT, t2);
    }
  } else if (dayPhase < DayNight::DUSK_START) {
    // Day
    topColor = Color::SKY_DAY_TOP;
    botColor = Color::SKY_DAY_BOT;
  } else if (dayPhase < DayNight::DUSK_END) {
    // Dusk transition
    float t = (dayPhase - DayNight::DUSK_START) / (DayNight::DUSK_END - DayNight::DUSK_START);
    if (t < 0.5f) {
      float t2 = t * 2.0f;
      topColor = lerpColor565(Color::SKY_DAY_TOP, Color::SKY_DUSK_TOP, t2);
      botColor = lerpColor565(Color::SKY_DAY_BOT, Color::SKY_DUSK_BOT, t2);
    } else {
      float t2 = (t - 0.5f) * 2.0f;
      topColor = lerpColor565(Color::SKY_DUSK_TOP, Color::SKY_NIGHT_TOP, t2);
      botColor = lerpColor565(Color::SKY_DUSK_BOT, Color::SKY_NIGHT_BOT, t2);
    }
  } else {
    // Night
    topColor = Color::SKY_NIGHT_TOP;
    botColor = Color::SKY_NIGHT_BOT;
  }
}

// Check if it's nighttime
static bool isNightTime(float dayPhase) {
  return dayPhase < DayNight::DAWN_START || dayPhase >= DayNight::DUSK_END;
}

// -------------------- SKY ZONE (top HUD area) --------------------
void gfx_drawSkyZone(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  // Only draw in the sky zone (top HUD_H pixels)
  int y0 = tileY;
  int y1 = tileY + Display::CANVAS_H - 1;
  if (y0 > Display::HUD_H || y1 < 0) return;

  float dayPhase = survival.dayPhase;
  uint16_t skyTop, skyBot;
  getSkyColors(dayPhase, skyTop, skyBot);

  // Draw gradient sky in HUD zone
  int drawStart = (tileY < 0) ? -tileY : 0;
  int drawEnd = (tileY + Display::CANVAS_H > Display::HUD_H) ? (Display::HUD_H - tileY) : Display::CANVAS_H;

  for (int y = drawStart; y < drawEnd; y++) {
    int screenY = tileY + y;
    float t = (float)screenY / (float)Display::HUD_H;
    uint16_t lineColor = lerpColor565(skyTop, skyBot, t);
    g.drawFastHLine(0, y, Display::CANVAS_W, lineColor);
  }

  // Draw sun or moon
  bool night = isNightTime(dayPhase);

  // Calculate celestial body position based on dayPhase
  // Sun rises at dawn (0.25), peaks at noon (0.5), sets at dusk (0.75)
  // Moon rises at dusk (0.75), peaks at midnight (0.0), sets at dawn (0.25)
  float celestialPhase;
  if (night) {
    // Moon: convert night phase to 0-1 arc
    if (dayPhase >= DayNight::DUSK_END) {
      celestialPhase = (dayPhase - DayNight::DUSK_END) / (1.0f - DayNight::DUSK_END + DayNight::DAWN_START) * 0.5f;
    } else {
      celestialPhase = 0.5f + dayPhase / DayNight::DAWN_START * 0.5f;
    }
  } else {
    // Sun: convert day phase to 0-1 arc
    celestialPhase = (dayPhase - DayNight::DAWN_START) / (DayNight::DUSK_END - DayNight::DAWN_START);
  }

  // Arc across the sky zone
  float arcX = celestialPhase;  // 0 = left edge, 1 = right edge
  float arcY = sinf(celestialPhase * MathConst::PI_F);  // Parabolic arc, highest at center

  int celestialX = (int)(arcX * Display::SCREEN_W);
  int celestialY = (int)(Display::HUD_H - 4 - arcY * (Display::HUD_H - 8));

  // Convert to tile coordinates
  int localX = celestialX - tileX + ox;
  int localY = celestialY - tileY + oy;

  // Only draw if celestial body is visible in this tile
  if (localX >= -12 && localX < Display::CANVAS_W + 12 && localY >= -12 && localY < Display::CANVAS_H + 12) {
    if (night) {
      // Moon
      g.fillCircle(localX, localY, 6, Color::MOON_GLOW);
      g.fillCircle(localX, localY, 4, Color::MOON_CORE);
      // Crescent effect
      g.fillCircle(localX + 2, localY - 1, 3, lerpColor565(Color::MOON_CORE, skyTop, 0.8f));
    } else {
      // Sun
      g.fillCircle(localX, localY, 7, Color::SUN_GLOW);
      g.fillCircle(localX, localY, 5, Color::SUN_CORE);
      // Sun rays (subtle)
      uint16_t rayColor = lerpColor565(Color::SUN_GLOW, skyBot, 0.5f);
      for (int i = 0; i < 8; i++) {
        float angle = (float)i * MathConst::PI_F / 4.0f + (float)nowMs * 0.0005f;
        int rx = localX + (int)(cosf(angle) * 10);
        int ry = localY + (int)(sinf(angle) * 10);
        g.drawLine(localX, localY, rx, ry, rayColor);
      }
    }
  }

  // Draw a few stars at night in the sky zone
  if (night) {
    uint32_t starSeed = 0x57A25u;
    for (int i = 0; i < 8; i++) {
      uint32_t h = hash32(starSeed + i);
      int sx = (h % Display::SCREEN_W);
      int sy = (int)((h >> 8) % Display::HUD_H);
      int localSX = sx - tileX + ox;
      int localSY = sy - tileY + oy;
      if (localSX >= 0 && localSX < Display::CANVAS_W && localSY >= 0 && localSY < drawEnd) {
        // Twinkle effect
        float twinkle = sinf((float)nowMs * 0.003f + (float)i) * 0.5f + 0.5f;
        if (twinkle > 0.3f) {
          g.drawPixel(localSX, localSY, Color::STAR);
        }
      }
    }
  }
}

// -------------------- GROUND FILL (grass base) --------------------
void gfx_drawGroundFill(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  // Fill area below HUD with ground color
  int y0 = tileY;
  int y1 = tileY + Display::CANVAS_H - 1;
  if (y1 < Display::HUD_H) return;  // Entirely in sky zone

  int drawStart = (tileY < Display::HUD_H) ? (Display::HUD_H - tileY) : 0;

  // Determine ground color based on time of day
  float dayPhase = survival.dayPhase;
  bool night = isNightTime(dayPhase);

  uint16_t groundColor = night ? Color::GRASS_NIGHT : Color::GROUND_BASE;

  g.fillRect(0, drawStart, Display::CANVAS_W, Display::CANVAS_H - drawStart, groundColor);
}

// -------------------- BOUNDARY ZONE --------------------
void gfx_drawBoundaryZone(Adafruit_GFX &g, int ox, int oy) {
  int hiveX = beeScreenCX() + ox;
  int hiveY = beeScreenCY() + oy;

  float distFromCenter = sqrtf(bee.wx * bee.wx + bee.wy * bee.wy);
  float worldBoundary = getWorldBoundary();

  if (distFromCenter > worldBoundary * 0.6f) {
    // Natural boundary - hedge/brush color instead of tech circle
    bool night = isNightTime(survival.dayPhase);
    uint16_t boundaryColor = night ? rgb565(30, 50, 35) : rgb565(60, 100, 50);
    g.drawCircle(hiveX, hiveY, (int)(worldBoundary * camera.zoom), boundaryColor);
  }
}

// -------------------- GRASS TUFTS (replaces grid) --------------------
void gfx_drawGrassLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  // Skip if entirely in sky zone
  if (tileY + Display::CANVAS_H <= Display::HUD_H) return;

  const int GRASS_CELL = 24;  // Spacing between grass tufts
  float camX = bee.wx * 0.95f;  // Slight parallax
  float camY = bee.wy * 0.95f;

  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + Display::CANVAS_W - 1;
  int sy1 = tileY + Display::CANVAS_H - 1;

  int32_t wx0 = (int32_t)(camX + (float)(sx0 - beeScreenCX()) / camera.zoom);
  int32_t wy0 = (int32_t)(camY + (float)(sy0 - beeScreenCY()) / camera.zoom);
  int32_t wx1 = (int32_t)(camX + (float)(sx1 - beeScreenCX()) / camera.zoom);
  int32_t wy1 = (int32_t)(camY + (float)(sy1 - beeScreenCY()) / camera.zoom);

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)GRASS_CELL);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)GRASS_CELL);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)GRASS_CELL);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)GRASS_CELL);

  bool night = isNightTime(survival.dayPhase);
  uint16_t grassDark = night ? Color::GRASS_NIGHT : Color::GRASS_DARK;
  uint16_t grassMid = night ? rgb565(30, 55, 30) : Color::GRASS_MID;
  uint16_t grassLight = night ? rgb565(40, 70, 40) : Color::GRASS_LIGHT;

  // Wind effect
  float windPhase = (float)nowMs * 0.002f;
  float windStrength = sinf((float)nowMs * 0.0008f) * 0.5f + 0.5f;

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, 0x62A55u);

      // Position within cell
      int px = (int)(h & 0x1Fu) % GRASS_CELL;
      int py = (int)((h >> 5) & 0x1Fu) % GRASS_CELL;

      int32_t wx = cx * GRASS_CELL + px;
      int32_t wy = cy * GRASS_CELL + py;

      int sx = beeScreenCX() + (int)(((float)wx - camX) * camera.zoom + camera.shakeX);
      int sy = beeScreenCY() + (int)(((float)wy - camY) * camera.zoom + camera.shakeY);

      // Skip if in sky zone
      if (sy < Display::HUD_H) continue;
      if (sx < sx0 - 4 || sx > sx1 + 4 || sy < sy0 - 6 || sy > sy1) continue;

      // Grass tuft height varies
      int height = 3 + (int)((h >> 10) & 0x3u);

      // Wind sway
      float localWind = sinf(windPhase + (float)wx * 0.1f) * windStrength * 2.0f;
      int sway = (int)localWind;

      // Draw grass blades (2-3 per tuft)
      int numBlades = 2 + (int)((h >> 12) & 0x1u);
      for (int b = 0; b < numBlades; b++) {
        int bx = sx + (b - 1) + ox;
        int by = sy + oy;
        int topX = bx + sway;

        // Draw blade from bottom to top
        g.drawLine(bx, by, topX, by - height, grassDark);
        if (height > 3) {
          g.drawPixel(topX, by - height, grassLight);
        }
        if ((h >> (14 + b)) & 1) {
          g.drawPixel(topX - 1, by - height + 1, grassMid);
        }
      }
    }
  }
}

// -------------------- CLOUD LAYER --------------------
void gfx_drawCloudLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy,
                        float parallax, int cell, uint32_t salt, uint32_t nowMs) {
  float driftX = (float)nowMs * 0.008f * parallax;
  float camX = bee.wx * parallax + driftX;
  float camY = bee.wy * parallax;

  int beeSX = beeScreenCX();
  int beeSY = beeScreenCY();

  int32_t wx0 = (int32_t)(camX + (float)(tileX - beeSX) / camera.zoom);
  int32_t wy0 = (int32_t)(camY + (float)(tileY - beeSY) / camera.zoom);
  int32_t wx1 = (int32_t)(camX + (float)(tileX + Display::CANVAS_W - 1 - beeSX) / camera.zoom);
  int32_t wy1 = (int32_t)(camY + (float)(tileY + Display::CANVAS_H - 1 - beeSY) / camera.zoom);

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)cell);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)cell);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)cell);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)cell);

  bool night = isNightTime(survival.dayPhase);
  uint16_t baseMain = night ? Color::CLOUD_NIGHT : Color::CLOUD_WHITE;
  uint16_t baseShadow = night ? rgb565(40, 50, 70) : Color::CLOUD_GRAY;

  // Base RGB values for fade calculations
  uint8_t mainR = night ? 60 : 245, mainG = night ? 70 : 248, mainB = night ? 90 : 255;
  uint8_t shadR = night ? 40 : 200, shadG = night ? 50 : 210, shadB = night ? 70 : 225;

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, salt);
      if ((h & 0xFu) > 2u) continue;  // Sparse: ~3/16 cells have clouds

      int32_t wx = cx * cell + (int)(h & 0xFFu) % cell;
      int32_t wy = cy * cell + (int)((h >> 8) & 0xFFu) % cell;

      int sx = beeSX + (int)(((float)wx - camX) * camera.zoom + camera.shakeX);
      int sy = beeSY + (int)(((float)wy - camY) * camera.zoom + camera.shakeY);

      // Skip if outside tile with margin for cloud size (clouds can be up to 49px wide with offsets)
      if (sx < tileX - 60 || sx > tileX + Display::CANVAS_W + 60 ||
          sy < tileY - 35 || sy > tileY + Display::CANVAS_H + 35) continue;

      int cloudW = 18 + (int)((h >> 16) & 0x1Fu);  // 18-49
      int cloudH = 10 + (int)((h >> 20) & 0xFu);   // 10-25

      // Fade cloud when bee flies through it (use logical distance without shake)
      uint16_t cloudMain = baseMain;
      uint16_t cloudShadow = baseShadow;

      // Calculate distance without shake for consistent fade behavior
      float logicalDx = ((float)wx - camX) * camera.zoom;
      float logicalDy = ((float)wy - camY) * camera.zoom;
      float distSq = logicalDx * logicalDx + logicalDy * logicalDy;
      float fadeRadius = (float)(cloudW + cloudH);
      float fadeRadiusSq = fadeRadius * fadeRadius;

      if (distSq < fadeRadiusSq) {
        float dist = sqrtf(distSq);
        // Smooth fade from fully visible at edge to nearly invisible at center
        float fade = dist / fadeRadius;
        // Apply easing for smoother transition (ease-in-out)
        fade = fade * fade * (3.0f - 2.0f * fade);
        // Minimum visibility of 15% so cloud doesn't completely vanish
        fade = 0.15f + fade * 0.85f;
        cloudMain = rgb565((uint8_t)(mainR * fade), (uint8_t)(mainG * fade), (uint8_t)(mainB * fade));
        cloudShadow = rgb565((uint8_t)(shadR * fade), (uint8_t)(shadG * fade), (uint8_t)(shadB * fade));
      }

      // Draw puffy cloud: shadow layer, then main puffs
      int cx_ofs = sx + ox, cy_ofs = sy + oy;
      g.fillCircle(cx_ofs, cy_ofs + 2, cloudH, cloudShadow);
      g.fillCircle(cx_ofs - cloudW / 3, cy_ofs, cloudH - 2, cloudMain);
      g.fillCircle(cx_ofs + cloudW / 3, cy_ofs, cloudH - 2, cloudMain);
      g.fillCircle(cx_ofs, cy_ofs - 2, cloudH, cloudMain);
      g.fillCircle(cx_ofs - cloudW / 5, cy_ofs - cloudH / 3, cloudH - 3, cloudMain);
      g.fillCircle(cx_ofs + cloudW / 5, cy_ofs - cloudH / 3, cloudH - 3, cloudMain);
    }
  }
}

// -------------------- FLOATING POLLEN/SEEDS (replaces nebula) --------------------
void gfx_drawPollenLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  // Skip if entirely in sky zone
  if (tileY + Display::CANVAS_H <= Display::HUD_H) return;

  float driftX = sinf((float)nowMs * 0.00015f) * 30.0f;
  float driftY = cosf((float)nowMs * 0.00012f) * 20.0f - (float)nowMs * 0.005f;  // Slowly rise
  float camX = bee.wx * 0.4f + driftX;
  float camY = bee.wy * 0.4f + driftY;
  const int cell = 48;

  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + Display::CANVAS_W - 1;
  int sy1 = tileY + Display::CANVAS_H - 1;

  int32_t wx0 = (int32_t)(camX + (float)(sx0 - beeScreenCX()) / camera.zoom);
  int32_t wy0 = (int32_t)(camY + (float)(sy0 - beeScreenCY()) / camera.zoom);
  int32_t wx1 = (int32_t)(camX + (float)(sx1 - beeScreenCX()) / camera.zoom);
  int32_t wy1 = (int32_t)(camY + (float)(sy1 - beeScreenCY()) / camera.zoom);

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)cell);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)cell);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)cell);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)cell);

  bool night = isNightTime(survival.dayPhase);
  // Pollen is dimmer at night
  uint16_t seedColor = night ? rgb565(120, 120, 100) : Color::SEED_WHITE;
  uint16_t pollenColor = night ? rgb565(150, 140, 80) : Color::SEED_YELLOW;

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, 0xB011E4u);
      if ((h & 0x0Fu) != 0u) continue;  // Sparse

      int px = (int)(h & 0x3Fu);
      int py = (int)((h >> 6) & 0x3Fu);
      int32_t wx = cx * cell + px;
      int32_t wy = cy * cell + py;

      int sx = beeScreenCX() + (int)(((float)wx - camX) * camera.zoom + camera.shakeX);
      int sy = beeScreenCY() + (int)(((float)wy - camY) * camera.zoom + camera.shakeY);

      // Skip if in sky zone or outside tile
      if (sy < Display::HUD_H) continue;
      if (sx < sx0 || sx > sx1 || sy < sy0 || sy > sy1) continue;

      // Determine if seed (white, larger) or pollen (yellow, smaller)
      bool isSeed = ((h >> 12) & 0x3u) == 0u;
      uint16_t c = isSeed ? seedColor : pollenColor;

      g.drawPixel(sx + ox, sy + oy, c);

      // Seeds have tiny "fluff" around them
      if (isSeed && !night) {
        float flutter = sinf((float)nowMs * 0.005f + (float)wx * 0.1f);
        int fluffX = (int)(flutter * 2.0f);
        g.drawPixel(sx + fluffX + ox, sy - 1 + oy, Color::SEED_WHITE);
      }
    }
  }
}

// -------------------- SCREEN ANCHOR (subtle, nature-themed) --------------------
void gfx_drawScreenAnchor(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;

  // More subtle, organic anchor - just corner hints
  bool night = isNightTime(survival.dayPhase);
  uint16_t c = night ? rgb565(30, 50, 45) : rgb565(60, 90, 70);

  // Small corner brackets instead of full crosshair
  g.drawFastHLine(cx - 24, cy - 20, 8, c);
  g.drawFastVLine(cx - 24, cy - 20, 8, c);

  g.drawFastHLine(cx + 17, cy - 20, 8, c);
  g.drawFastVLine(cx + 24, cy - 20, 8, c);

  g.drawFastHLine(cx - 24, cy + 20, 8, c);
  g.drawFastVLine(cx - 24, cy + 13, 8, c);

  g.drawFastHLine(cx + 17, cy + 20, 8, c);
  g.drawFastVLine(cx + 24, cy + 13, 8, c);
}

// -------------------- NIGHT WARNING OVERLAY --------------------
// Helper: Draw a portion of a screen-space rectangle that falls within this tile
static void drawScreenBorderInTile(Adafruit_GFX &g, int tileX, int tileY, int thickness, uint16_t color) {
  // Screen edges in tile-local coordinates
  int leftEdge = -tileX;
  int rightEdge = Display::SCREEN_W - 1 - tileX;
  int topEdge = -tileY;
  int bottomEdge = Display::SCREEN_H - 1 - tileY;

  // Draw only the portions of screen border that fall within this tile
  for (int t = 0; t < thickness; t++) {
    // Top edge
    if (topEdge + t >= 0 && topEdge + t < Display::CANVAS_H) {
      int x0 = (leftEdge + t < 0) ? 0 : leftEdge + t;
      int x1 = (rightEdge - t >= Display::CANVAS_W) ? Display::CANVAS_W - 1 : rightEdge - t;
      if (x0 <= x1) g.drawFastHLine(x0, topEdge + t, x1 - x0 + 1, color);
    }
    // Bottom edge
    if (bottomEdge - t >= 0 && bottomEdge - t < Display::CANVAS_H) {
      int x0 = (leftEdge + t < 0) ? 0 : leftEdge + t;
      int x1 = (rightEdge - t >= Display::CANVAS_W) ? Display::CANVAS_W - 1 : rightEdge - t;
      if (x0 <= x1) g.drawFastHLine(x0, bottomEdge - t, x1 - x0 + 1, color);
    }
    // Left edge
    if (leftEdge + t >= 0 && leftEdge + t < Display::CANVAS_W) {
      int y0 = (topEdge + t < 0) ? 0 : topEdge + t;
      int y1 = (bottomEdge - t >= Display::CANVAS_H) ? Display::CANVAS_H - 1 : bottomEdge - t;
      if (y0 <= y1) g.drawFastVLine(leftEdge + t, y0, y1 - y0 + 1, color);
    }
    // Right edge
    if (rightEdge - t >= 0 && rightEdge - t < Display::CANVAS_W) {
      int y0 = (topEdge + t < 0) ? 0 : topEdge + t;
      int y1 = (bottomEdge - t >= Display::CANVAS_H) ? Display::CANVAS_H - 1 : bottomEdge - t;
      if (y0 <= y1) g.drawFastVLine(rightEdge - t, y0, y1 - y0 + 1, color);
    }
  }
}

void gfx_drawNightWarning(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  float dayPhase = survival.dayPhase;

  // Warning zone: approaching dusk
  if (dayPhase >= DayNight::NIGHT_WARN_PHASE && dayPhase < DayNight::DUSK_END) {
    // Pulsing warning tint at edges
    float urgency = (dayPhase - DayNight::NIGHT_WARN_PHASE) / (DayNight::DUSK_END - DayNight::NIGHT_WARN_PHASE);
    float pulse = sinf((float)nowMs * 0.006f) * 0.5f + 0.5f;
    int alpha = (int)(urgency * pulse * 60.0f);

    if (alpha > 8) {
      uint16_t warnColor = rgb565(alpha, alpha / 3, 0);
      // Draw warning border around full screen
      drawScreenBorderInTile(g, tileX, tileY, 2, warnColor);
    }
  }

  // Active night damage indicator
  if (isNightTime(dayPhase)) {
    float pulse = sinf((float)nowMs * 0.01f) * 0.5f + 0.5f;
    int alpha = (int)(40 + pulse * 30);
    uint16_t dangerColor = rgb565(alpha, 0, alpha / 2);

    // Vignette effect around full screen
    drawScreenBorderInTile(g, tileX, tileY, 3, dangerColor);
  }
}

// -------------------- NIGHT REST OVERLAY (safe at hive) --------------------
void gfx_drawNightRestOverlay(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  if (!isNightResting()) return;

  float progress = getNightRestProgress(nowMs);

  // Dark overlay that fades in then out
  float darkness;
  if (progress < 0.3f) {
    // Fade in
    darkness = progress / 0.3f;
  } else if (progress > 0.7f) {
    // Fade out
    darkness = (1.0f - progress) / 0.3f;
  } else {
    // Full darkness
    darkness = 1.0f;
  }

  // Draw dark overlay (only if significant)
  if (darkness > 0.1f) {
    int alpha = (int)(darkness * 180.0f);
    uint16_t overlayColor = rgb565(alpha / 12, alpha / 15, alpha / 8);

    // Draw semi-transparent darkness effect with dithered pattern
    for (int y = 0; y < Display::CANVAS_H; y += 2) {
      for (int x = (y / 2) % 2; x < Display::CANVAS_W; x += 2) {
        g.drawPixel(x, y, overlayColor);
      }
    }
  }

  // Draw moon and stars in center during peak darkness
  if (progress > 0.2f && progress < 0.8f) {
    float moonAlpha = (progress < 0.5f) ? (progress - 0.2f) / 0.3f : (0.8f - progress) / 0.3f;

    int centerX = Display::CANVAS_W / 2;
    int centerY = Display::CANVAS_H / 2 - 10;

    // Only draw if this tile contains the center area
    int screenCenterX = Display::SCREEN_W / 2;
    int screenCenterY = Display::SCREEN_H / 2 - 10;
    int localX = screenCenterX - tileX;
    int localY = screenCenterY - tileY;

    if (localX >= -20 && localX < Display::CANVAS_W + 20 &&
        localY >= -20 && localY < Display::CANVAS_H + 20) {

      // Moon
      if (moonAlpha > 0.3f) {
        g.fillCircle(localX, localY, 12, Color::MOON_GLOW);
        g.fillCircle(localX, localY, 9, Color::MOON_CORE);
        g.fillCircle(localX + 4, localY - 2, 7, rgb565(10, 15, 35));  // Crescent shadow
      }

      // Twinkling stars around moon
      for (int i = 0; i < 6; i++) {
        float angle = (float)i * MathConst::TAU / 6.0f + (float)nowMs * 0.001f;
        int dist = 30 + (i % 3) * 10;
        int starX = localX + (int)(cosf(angle) * dist);
        int starY = localY + (int)(sinf(angle) * dist);

        float twinkle = sinf((float)nowMs * 0.005f + (float)i * 1.5f) * 0.5f + 0.5f;
        if (twinkle > 0.4f && starX >= 0 && starX < Display::CANVAS_W &&
            starY >= 0 && starY < Display::CANVAS_H) {
          g.drawPixel(starX, starY, Color::STAR);
          if (twinkle > 0.7f) {
            g.drawPixel(starX + 1, starY, Color::STAR2);
            g.drawPixel(starX, starY + 1, Color::STAR2);
          }
        }
      }

      // "ZZZ" sleep text
      if (progress > 0.35f && progress < 0.65f) {
        float zzAlpha = (progress < 0.5f) ? (progress - 0.35f) / 0.15f : (0.65f - progress) / 0.15f;
        if (zzAlpha > 0.5f) {
          int zzX = localX + 20;
          int zzY = localY - 15;
          float bob = sinf((float)nowMs * 0.004f) * 3.0f;

          g.setTextSize(1);
          g.setTextColor(Color::UI_DIM);
          g.setCursor(zzX, zzY + (int)bob);
          g.print("Z");
          g.setCursor(zzX + 6, zzY - 5 + (int)(bob * 0.7f));
          g.print("z");
          g.setCursor(zzX + 10, zzY - 9 + (int)(bob * 0.5f));
          g.print("z");
        }
      }

      // "Night X" counter
      if (survival.nightsSurvived > 0 && progress > 0.4f && progress < 0.6f) {
        char nightText[16];
        snprintf(nightText, sizeof(nightText), "Night %d", survival.nightsSurvived);
        int textW = strlen(nightText) * 6;
        g.setTextSize(1);
        g.setTextColor(Color::MOON_GLOW);
        g.setCursor(localX - textW / 2, localY + 25);
        g.print(nightText);
      }
    }
  }
}

// -------------------- DAY TRANSITION OVERLAY (DAY X display) --------------------
void gfx_drawDayTransitionOverlay(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  if (!isDayTransition()) return;

  float progress = getDayTransitionProgress(nowMs);

  // Fade in then out
  float alpha;
  if (progress < 0.25f) {
    // Fade in
    alpha = progress / 0.25f;
  } else if (progress > 0.75f) {
    // Fade out
    alpha = (1.0f - progress) / 0.25f;
  } else {
    // Full visibility
    alpha = 1.0f;
  }

  if (alpha < 0.1f) return;

  // Calculate hive position on screen (hive is at world 0,0)
  int hiveSX, hiveSY;
  worldToScreen(0, 0, hiveSX, hiveSY);
  int localX = hiveSX - tileX;
  int localY = hiveSY - tileY;

  // Only draw if this tile contains the center area
  if (localX >= -60 && localX < Display::CANVAS_W + 60 &&
      localY >= -30 && localY < Display::CANVAS_H + 30) {

    // Format day text
    char dayText[12];
    snprintf(dayText, sizeof(dayText), "DAY %d", survival.currentDay);
    int textLen = strlen(dayText);

    // Calculate centered position (size 3 = 18px per char)
    // Position text ABOVE the ray origin point
    int textW = textLen * 18;
    int textX = localX - textW / 2;
    int textY = localY - 35;

    g.setTextWrap(false);  // Prevent text wrapping!

    // Draw orange rays FIRST (behind text)
    if (alpha > 0.2f) {
      float rayAlpha = (alpha - 0.2f) / 0.8f;
      // Orange color for rays
      uint8_t rayR = (uint8_t)(rayAlpha * 255);
      uint8_t rayG = (uint8_t)(rayAlpha * 140);
      uint8_t rayB = (uint8_t)(rayAlpha * 30);
      uint16_t rayColor = rgb565(rayR, rayG, rayB);

      // 8 rays radiating outward from hive
      for (int i = 0; i < 8; i++) {
        float angle = (float)i * MathConst::PI_F / 4.0f + (float)nowMs * 0.001f;
        int rx1 = localX + (int)(cosf(angle) * 20);
        int ry1 = localY + (int)(sinf(angle) * 12);
        int rx2 = localX + (int)(cosf(angle) * 60);
        int ry2 = localY + (int)(sinf(angle) * 36);
        g.drawLine(rx1, ry1, rx2, ry2, rayColor);
      }
    }

    // Draw text ON TOP of rays with dark outline for legibility
    if (alpha > 0.3f) {
      g.setTextSize(3);

      // Dark outline for legibility against orange rays
      uint16_t outlineColor = rgb565(30, 20, 10);
      g.setTextColor(outlineColor);
      g.setCursor(textX - 1, textY - 1);
      g.print(dayText);
      g.setCursor(textX + 1, textY - 1);
      g.print(dayText);
      g.setCursor(textX - 1, textY + 1);
      g.print(dayText);
      g.setCursor(textX + 1, textY + 1);
      g.print(dayText);

      // Bright white/yellow main text
      uint8_t mainBright = (uint8_t)(alpha * 255);
      uint16_t mainColor = rgb565(mainBright, mainBright, (uint8_t)(mainBright * 0.7f));
      g.setTextColor(mainColor);
      g.setCursor(textX, textY);
      g.print(dayText);
    }
  }
}

// -------------------- LEGACY STUBS (keep for compatibility) --------------------
void gfx_drawWorldGrid(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  // Replaced by grass - no longer draws grid
}

void gfx_drawStarLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy,
                       float parallax, int cell, uint16_t cA, uint16_t cB, uint32_t salt) {
  // Replaced by clouds - keeping stub for compatibility
  // Now draws clouds instead
  gfx_drawCloudLayer(g, tileX, tileY, ox, oy, parallax, cell * 2, salt, millis());
}

void gfx_drawNebulaLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  // Replaced by pollen - keeping stub for compatibility
  gfx_drawPollenLayer(g, tileX, tileY, ox, oy, nowMs);
}
