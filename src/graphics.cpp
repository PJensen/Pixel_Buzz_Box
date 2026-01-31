// Pixel Buzz Box - Graphics and Rendering
#include "game.h"
#include <math.h>

// -------------------- DRAWING PRIMITIVES --------------------
static void drawBoostAura(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  float t = (float)(nowMs % 900u) / 900.0f;
  int r = 14 + (int)(4.0f * sinf(t * 6.2831853f));
  uint16_t c1 = rgb565(255, 210, 60);
  uint16_t c2 = rgb565(255, 240, 140);
  g.drawCircle(x, y, r, c1);
  g.drawCircle(x, y, r + 2, c2);
  g.drawCircle(x, y, r - 2, c1);
}

static void drawPollenSparkles(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  if (pollenCount == 0) return;
  int sparkles = clampi(4 + (int)pollenCount, 4, 12);
  for (int i = 0; i < sparkles; i++) {
    uint32_t h = hash32(((uint32_t)nowMs >> 4) + (uint32_t)i * 977u);
    int dx = (int)(h & 0x1Fu) - 15;
    int dy = (int)((h >> 5) & 0x1Fu) - 15;
    if ((dx * dx + dy * dy) > 160) continue;
    uint16_t c = (i & 1) ? COL_POLLEN_HI : COL_WHITE;
    if (((h >> 11) & 1u) == 0u) g.drawPixel(x + dx, y + dy, c);
  }
}

static void drawBeeShadow(Adafruit_GFX &g, int x, int y) {
  int sy = y + 14;
  float s = 0.5f + 0.5f * sinf(wingPhase);
  int rx = 10 + (int)(3 * (1.0f - s)) + (int)(2 * wingSpeed);
  int ry = 3  + (int)(2 * (1.0f - s));

  for (int yy = -ry; yy <= ry; yy++) {
    float yf = (float)yy / (float)ry;
    float inside = 1.0f - yf * yf;
    if (inside <= 0.0f) continue;
    int span = (int)(rx * sqrtf(inside));

    int yrow = sy + yy;
    for (int xx = -span; xx <= span; xx++) {
      int xcol = x + xx;
      if (((xcol + yrow) & 1) == 0) g.drawPixel(xcol, yrow, COL_SHADOW);
    }
  }
  g.drawFastHLine(x - rx + 2, sy, rx * 2 - 4, COL_SHADOW_RIM);
}

static void drawPollenOrbit(Adafruit_GFX &g, int x, int y) {
  if (pollenCount == 0) return;
  int count = pollenCount;
  float base = wingPhase * 1.4f;
  int ring = 10 + (count / 3) * 2;
  int ringY = ring - 2;

  for (int i = 0; i < count; i++) {
    float ang = base + (6.2831853f * (float)i) / (float)count;
    int px = x + (int)(cosf(ang) * (float)ring);
    int py = y + 5 + (int)(sinf(ang) * (float)ringY);
    g.fillCircle(px, py, 2, COL_POLLEN);
    g.drawPixel(px + 1, py - 1, COL_POLLEN_HI);
  }

  if (pollenCount >= MAX_POLLEN_CARRY) {
    g.drawCircle(x, y + 2, ring + 4, COL_POLLEN_HI);
  }
}

static void drawBee(Adafruit_GFX &g, int x, int y) {
  float load = clampf((float)pollenCount / (float)MAX_POLLEN_CARRY, 0.0f, 1.0f);
  uint8_t bodyR = 255;
  uint8_t bodyG = (uint8_t)(220 + (int)(25.0f * load));
  uint8_t bodyB = (uint8_t)(40 + (int)(120.0f * load));
  uint16_t body = rgb565(bodyR, bodyG, bodyB);

  float s = sinf(wingPhase);
  int flap = (int)(s * (2 + (int)(3 * wingSpeed)));
  int wH   = 4 + (int)(2 * (0.5f + 0.5f * s));
  int wW   = 7 + (int)(2 * wingSpeed);
  uint8_t wr = clampu8(170 + (int)(55.0f * (0.5f + 0.5f * s)));
  uint8_t wg = clampu8(215 + (int)(35.0f * (0.5f + 0.5f * s)));
  uint8_t wb = 255;
  uint16_t wingCol = rgb565(wr, wg, wb);

  g.fillEllipse(x - 6, y - 9 + flap, wW, wH, wingCol);
  g.fillEllipse(x + 2, y - 10 - flap/2, wW, wH, wingCol);
  g.drawEllipse(x - 6, y - 9 + flap, wW, wH, COL_WHITE);
  g.drawEllipse(x + 2, y - 10 - flap/2, wW, wH, COL_WHITE);

  if (s > 0.35f) {
    g.drawPixel(x - 9, y - 12 + flap, COL_POLLEN_HI);
    g.drawPixel(x + 5, y - 13 - flap/2, COL_POLLEN_HI);
  }

  g.fillEllipse(x, y, 12, 8, body);
  g.fillRect(x - 9, y - 6, 4, 12, COL_BLK);
  g.fillRect(x - 1, y - 6, 4, 12, COL_BLK);
  g.drawEllipse(x, y, 12, 8, COL_WHITE);

  g.fillCircle(x + 11, y - 1, 5, COL_BLK);
  g.drawCircle(x + 11, y - 1, 5, COL_WHITE);

  g.fillTriangle(x - 13, y, x - 18, y - 2, x - 18, y + 2, COL_BLK);

  drawPollenOrbit(g, x, y);
}

static void drawHive(Adafruit_GFX &g, int x, int y) {
  g.drawCircle(x, y, 12, COL_HIVE);
  g.drawCircle(x, y,  7, COL_HIVE);
  g.drawCircle(x, y,  2, COL_HIVE);
}

static void drawHivePulse(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  if ((int32_t)(nowMs - hivePulseUntilMs) >= 0) return;
  float t = 1.0f - (float)(hivePulseUntilMs - nowMs) / (float)HIVE_PULSE_MS;
  t = clampf(t, 0.0f, 1.0f);
  int r = 10 + (int)(t * 26.0f);
  uint16_t c1 = rgb565(140, 220, 150);
  uint16_t c2 = rgb565(220, 255, 230);
  g.drawCircle(x, y, r, c1);
  g.drawCircle(x, y, r + 4, c2);
  if ((nowMs & 0x3u) == 0u) {
    g.drawCircle(x, y, r - 2, COL_WHITE);
  }
}

static void drawFlower(Adafruit_GFX &g, int x, int y, const Flower &f, uint32_t nowMs, uint32_t bornMs) {
  if (!f.alive) return;
  int r = (int)f.r;

  // subtle shadow underlay
  int sx = x + 1;
  int sy = y + 1;
  g.fillCircle(sx - r, sy, r, f.petalLo);
  g.fillCircle(sx + r, sy, r, f.petalLo);
  g.fillCircle(sx, sy - r, r, f.petalLo);
  g.fillCircle(sx, sy + r, r, f.petalLo);
  g.fillCircle(sx, sy, r, f.petalLo);

  // petals
  g.fillCircle(x - r, y, r, f.petal);
  g.fillCircle(x + r, y, r, f.petal);
  g.fillCircle(x, y - r, r, f.petal);
  g.fillCircle(x, y + r, r, f.petal);
  g.fillCircle(x, y, r, f.petal);

  int cr = r / 2 + 2;
  g.fillCircle(x, y, cr, f.center);
  g.drawCircle(x, y, cr, COL_WHITE);

  g.drawPixel(x - 1, y - 1, COL_POLLEN_HI);
  g.drawPixel(x - 2, y - 1, COL_WHITE);

  // quick bloom pop on spawn
  uint32_t age = nowMs - bornMs;
  if (age < 420) {
    float t = (float)age / 420.0f;
    t = clampf(t, 0.0f, 1.0f);
    int growR = 1 + (int)(t * (float)(r + 2));
    uint16_t bloomCore = rgb565(255, 245, 200);
    g.fillCircle(x, y, growR, bloomCore);
    g.drawCircle(x, y, growR + 2, COL_WHITE);

    float ringT = 1.0f - t;
    int br = r + 8 + (int)(ringT * 10.0f);
    uint16_t bc = rgb565(255, 235, 200);
    uint16_t bc2 = rgb565(255, 250, 230);
    g.drawCircle(x, y, br, bc);
    g.drawCircle(x, y, br + 4, bc2);
    if ((age & 0x3u) == 0u) {
      g.drawCircle(x, y, br - 2, COL_WHITE);
      g.drawCircle(x, y, br + 1, COL_POLLEN_HI);
    }
    if ((age & 0x7u) == 0u) {
      int sparkR = br + 6;
      g.drawPixel(x + sparkR, y, bc2);
      g.drawPixel(x - sparkR, y, bc2);
      g.drawPixel(x, y + sparkR, bc2);
      g.drawPixel(x, y - sparkR, bc2);
    }
  }
}

// -------------------- TRAIL PARTICLES --------------------
void drawTrailParticles(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  const uint32_t TRAIL_LIFE_MS = 300;
  for (int i = 0; i < TRAIL_MAX; i++) {
    if (!trail[i].alive) continue;
    uint32_t age = nowMs - trail[i].bornMs;
    if (age > TRAIL_LIFE_MS) continue;

    int sx, sy;
    worldToScreenF(trail[i].wx, trail[i].wy, sx, sy);

    float t = (float)age / (float)TRAIL_LIFE_MS;
    float alpha = 1.0f - t * t;

    float speedT = trail[i].speedN;
    uint8_t baseR = (uint8_t)(255 - (int)(115.0f * speedT));
    uint8_t baseG = (uint8_t)(220 - (int)(120.0f * speedT));
    uint8_t baseB = (uint8_t)(60 + (int)(195.0f * speedT));

    uint8_t r = (uint8_t)(baseR * alpha);
    uint8_t g_val = (uint8_t)(baseG * alpha);
    uint8_t b = (uint8_t)(baseB * alpha);

    if (alpha > 0.6f) {
      uint16_t outerGlow = rgb565(r / 3, g_val / 3, b / 3);
      g.fillCircle(sx + ox, sy + oy, 5, outerGlow);

      uint16_t midGlow = rgb565(r / 2, g_val / 2, b / 2);
      g.fillCircle(sx + ox, sy + oy, 3, midGlow);

      uint16_t core = rgb565(r, g_val, b);
      g.fillCircle(sx + ox, sy + oy, 2, core);

      if (trail[i].variant == 0 && alpha > 0.8f) {
        uint16_t sparkle = rgb565(255, 255, 200);
        g.drawPixel(sx - 3 + ox, sy + oy, sparkle);
        g.drawPixel(sx + 3 + ox, sy + oy, sparkle);
        g.drawPixel(sx + ox, sy - 3 + oy, sparkle);
        g.drawPixel(sx + ox, sy + 3 + oy, sparkle);
      }
    } else if (alpha > 0.3f) {
      uint16_t midGlow = rgb565(r / 2, g_val / 2, b / 2);
      g.fillCircle(sx + ox, sy + oy, 3, midGlow);

      uint16_t core = rgb565(r, g_val, b);
      g.fillCircle(sx + ox, sy + oy, 1, core);
    } else {
      uint16_t dim = rgb565(r, g_val, b);
      g.drawPixel(sx + ox, sy + oy, dim);
    }
  }
}

// -------------------- SCORE POPUPS --------------------
void drawScorePopups(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  char buf[8];
  for (int i = 0; i < SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) continue;
    uint32_t age = nowMs - scorePopups[i].bornMs;
    if (age > SCORE_POPUP_LIFE_MS) continue;

    float t = (float)age / (float)SCORE_POPUP_LIFE_MS;
    t = clampf(t, 0.0f, 1.0f);

    float u = 1.0f - (1.0f - t) * (1.0f - t);
    int floatY = (int)(28.0f * u);
    int sway = (int)(sinf((float)age * 0.018f + (float)scorePopups[i].driftX) * 2.0f);

    int cx = (int)scorePopups[i].baseSX + scorePopups[i].driftX + sway;
    int cy = (int)scorePopups[i].baseSY - 6 - floatY;

    int size;
    if (t < 0.18f) size = 1;
    else if (t < 0.72f) size = 2;
    else size = 3;

    snprintf(buf, sizeof(buf), "+%d", (int)scorePopups[i].value);
    int len = (int)strlen(buf);
    int textW = len * 6 * size;
    int textH = 8 * size;
    int x0 = cx - textW / 2;
    int y0 = cy - textH / 2;
    int x1 = x0 + textW - 1;
    int y1 = y0 + textH - 1;

    int tx0 = -ox;
    int ty0 = -oy;
    int tx1 = tx0 + CANVAS_W - 1;
    int ty1 = ty0 + CANVAS_H - 1;
    if (x1 < tx0 || x0 > tx1 || y1 < ty0 || y0 > ty1) continue;

    g.setTextWrap(false);
    g.setTextSize(size);

    g.setTextColor(COL_SHADOW);
    g.setCursor(x0 + 1 + ox, y0 + 1 + oy);
    g.print(buf);

    uint16_t mainCol = (t > 0.75f) ? COL_POLLEN_HI : COL_YEL;
    g.setTextColor(mainCol);
    g.setCursor(x0 + ox, y0 + oy);
    g.print(buf);

    if (t > 0.72f) {
      g.setTextColor(COL_WHITE);
      g.setCursor(x0 - 1 + ox, y0 + oy);
      g.print(buf);
      g.setCursor(x0 + 1 + ox, y0 - 1 + oy);
      g.print(buf);
    }

    g.setTextWrap(true);
  }
}

// -------------------- BACKGROUND --------------------
static void drawBoundaryZone(Adafruit_GFX &g, int ox, int oy) {
  int hiveX = beeScreenCX() + ox;
  int hiveY = beeScreenCY() + oy;

  float distFromCenter = sqrtf(beeWX * beeWX + beeWY * beeWY);

  if (distFromCenter > BOUNDARY_COMFORTABLE * 0.6f) {
    uint16_t boundaryColor = rgb565(50, 70, 90);
    g.drawCircle(hiveX, hiveY, (int)(BOUNDARY_COMFORTABLE * cameraZoom), boundaryColor);
  }
}

static void drawWorldGrid(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  const int GRID = 160;
  const int GRID2 = 80;

  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + CANVAS_W - 1;
  int sy1 = tileY + CANVAS_H - 1;

  int32_t wx0 = (int32_t)(beeWX + (float)(sx0 - beeScreenCX()) / cameraZoom);
  int32_t wy0 = (int32_t)(beeWY + (float)(sy0 - beeScreenCY()) / cameraZoom);
  int32_t wx1 = (int32_t)(beeWX + (float)(sx1 - beeScreenCX()) / cameraZoom);
  int32_t wy1 = (int32_t)(beeWY + (float)(sy1 - beeScreenCY()) / cameraZoom);

  int32_t gx0 = (int32_t)floorf((float)wx0 / (float)GRID2) * GRID2;
  for (int32_t gx = gx0; gx <= wx1; gx += GRID2) {
    int sx = beeScreenCX() + (int)(((float)gx - beeWX) * cameraZoom);
    if (sx < sx0 || sx > sx1) continue;
    bool major = ((gx % GRID) == 0);
    uint16_t c = major ? COL_GRID : COL_GRID2;
    g.drawFastVLine(sx + ox, sy0 + oy, CANVAS_H, c);
  }

  int32_t gy0 = (int32_t)floorf((float)wy0 / (float)GRID2) * GRID2;
  for (int32_t gy = gy0; gy <= wy1; gy += GRID2) {
    int sy = beeScreenCY() + (int)(((float)gy - beeWY) * cameraZoom);
    if (sy < sy0 || sy > sy1) continue;
    bool major = ((gy % GRID) == 0);
    uint16_t c = major ? COL_GRID : COL_GRID2;
    g.drawFastHLine(sx0 + ox, sy + oy, CANVAS_W, c);
  }
}

static void drawStarLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy,
                          float parallax, int cell, uint16_t cA, uint16_t cB, uint32_t salt) {
  float camX = beeWX * parallax;
  float camY = beeWY * parallax;

  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + CANVAS_W - 1;
  int sy1 = tileY + CANVAS_H - 1;

  int32_t wx0 = (int32_t)(camX + (float)(sx0 - beeScreenCX()) / cameraZoom);
  int32_t wy0 = (int32_t)(camY + (float)(sy0 - beeScreenCY()) / cameraZoom);
  int32_t wx1 = (int32_t)(camX + (float)(sx1 - beeScreenCX()) / cameraZoom);
  int32_t wy1 = (int32_t)(camY + (float)(sy1 - beeScreenCY()) / cameraZoom);

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)cell);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)cell);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)cell);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)cell);

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, salt);
      if ((h & 0x7u) != 0u) continue;

      int px = (int)(h & 0xFFu) % cell;
      int py = (int)((h >> 8) & 0xFFu) % cell;

      int32_t wx = cx * cell + px;
      int32_t wy = cy * cell + py;

      int sx = beeScreenCX() + (int)(((float)wx - camX) * cameraZoom);
      int sy = beeScreenCY() + (int)(((float)wy - camY) * cameraZoom);

      if (sx < sx0 || sx > sx1 || sy < sy0 || sy > sy1) continue;

      uint16_t c = ((h >> 16) & 1u) ? cA : cB;
      g.drawPixel(sx + ox, sy + oy, c);

      if (((h >> 20) & 0xFu) == 0u) {
        g.drawPixel(sx - 1 + ox, sy + oy, c);
        g.drawPixel(sx + 1 + ox, sy + oy, c);
      }
    }
  }
}

static void drawNebulaLayer(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy, uint32_t nowMs) {
  float driftX = sinf((float)nowMs * 0.00012f) * 22.0f;
  float driftY = cosf((float)nowMs * 0.00010f) * 18.0f;
  float camX = beeWX * 0.35f + driftX;
  float camY = beeWY * 0.35f + driftY;
  const int cell = 64;

  int sx0 = tileX;
  int sy0 = tileY;
  int sx1 = tileX + CANVAS_W - 1;
  int sy1 = tileY + CANVAS_H - 1;

  int32_t wx0 = (int32_t)(camX + (float)(sx0 - beeScreenCX()) / cameraZoom);
  int32_t wy0 = (int32_t)(camY + (float)(sy0 - beeScreenCY()) / cameraZoom);
  int32_t wx1 = (int32_t)(camX + (float)(sx1 - beeScreenCX()) / cameraZoom);
  int32_t wy1 = (int32_t)(camY + (float)(sy1 - beeScreenCY()) / cameraZoom);

  int32_t cx0 = (int32_t)floorf((float)wx0 / (float)cell);
  int32_t cy0 = (int32_t)floorf((float)wy0 / (float)cell);
  int32_t cx1 = (int32_t)floorf((float)wx1 / (float)cell);
  int32_t cy1 = (int32_t)floorf((float)wy1 / (float)cell);

  for (int32_t cy = cy0; cy <= cy1; cy++) {
    for (int32_t cx = cx0; cx <= cx1; cx++) {
      uint32_t h = worldCellSeed(cx, cy, 0xD1B00Bu);
      if ((h & 0x0Fu) != 0u) continue;

      int px = (int)(h & 0x3Fu);
      int py = (int)((h >> 6) & 0x3Fu);
      int32_t wx = cx * cell + px;
      int32_t wy = cy * cell + py;

      int sx = beeScreenCX() + (int)(((float)wx - camX) * cameraZoom);
      int sy = beeScreenCY() + (int)(((float)wy - camY) * cameraZoom);
      if (sx < sx0 || sx > sx1 || sy < sy0 || sy > sy1) continue;

      uint8_t r = clampu8(40 + (int)((h >> 12) & 0x1Fu));
      uint8_t gcol = clampu8(40 + (int)((h >> 17) & 0x1Fu));
      uint8_t b = clampu8(70 + (int)((h >> 22) & 0x3Fu));
      uint16_t c = rgb565(r, gcol, b);

      g.drawPixel(sx + ox, sy + oy, c);
      if ((h & 0x100u) != 0u) {
        g.drawPixel(sx + 1 + ox, sy + oy, c);
        g.drawPixel(sx + ox, sy + 1 + oy, c);
      }
    }
  }
}

static void drawScreenAnchor(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;
  uint16_t c = rgb565(40, 70, 90);

  g.drawFastHLine(cx - 26, cy, 12, c);
  g.drawFastHLine(cx + 15, cy, 12, c);
  g.drawFastVLine(cx, cy - 26, 12, c);
  g.drawFastVLine(cx, cy + 15, 12, c);

  float t = (float)(nowMs % 1200u) / 1200.0f;
  int r = 22 + (int)(6.0f * sinf(t * 6.2831853f));
  g.drawCircle(cx, cy, r, rgb565(35, 55, 70));
}

// -------------------- HUD + BELT --------------------
static void drawHUDInTile(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  int y0 = tileY;
  int y1 = tileY + CANVAS_H - 1;
  if (y0 > (HUD_H - 1) || y1 < 0) return;

  int topClip = 0;
  int botClip = CANVAS_H;

  if (tileY < 0) {
    topClip = -tileY;
  }
  if (tileY + CANVAS_H > HUD_H) {
    botClip = HUD_H - tileY;
  }

  g.fillRect(0, topClip, CANVAS_W, botClip - topClip, COL_HUD_BG);

  if (tileY != 0) return;

  g.setCursor(6 + ox, 6 + oy);
  g.setTextColor(COL_WHITE);
  g.setTextSize(2);
  g.print("POLLEN ");
  g.print(score);

  g.setTextSize(1);
  g.setCursor(6 + ox, 20 + oy);
  g.setTextColor(pollenCount ? COL_YEL : COL_UI_DIM);
  if (pollenCount) {
    g.print("CARRY ");
    g.print((int)pollenCount);
  } else {
    g.print("EMPTY ");
    g.print("0");
  }
  g.print("/");
  g.print((int)MAX_POLLEN_CARRY);

  int rackX = 84 + ox;
  int rackY = 20 + oy;
  int idx = 0;
  for (int ry = 0; ry < 2; ry++) {
    for (int rx = 0; rx < 4; rx++) {
      if (idx >= MAX_POLLEN_CARRY) break;
      int cx = rackX + rx * 6;
      int cy = rackY + ry * 6;
      uint16_t c = (idx < pollenCount) ? COL_POLLEN : COL_UI_DIM;
      g.fillCircle(cx, cy, 2, c);
      if (idx < pollenCount) g.drawPixel(cx + 1, cy - 1, COL_POLLEN_HI);
      idx++;
    }
  }

  int bx = tft.width() - 92;
  g.setCursor(bx + ox, 6 + oy);
  g.setTextColor(boostCharge ? COL_UI_GO : COL_UI_DIM);
  g.print("BOOST ");
  g.print(boostCharge ? "READY" : "--");

  g.setCursor(bx + ox, 16 + oy);
  g.setTextColor(COL_UI_DIM);
  g.print("x3 ");
  g.print((int)depositsTowardBoost);

  uint32_t now = millis();
  bool cd = (int32_t)(boostCooldownUntilMs - now) > 0;
  g.setCursor(bx + ox, 24 + oy);
  g.setTextColor(cd ? COL_UI_WARN : COL_UI_DIM);
  g.print(cd ? "COOLDN" : "");
}

static void drawBeltHUD(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  int x0 = tft.width()  - 122;
  int y0 = tft.height() - 56;
  int x1 = tft.width()  - 6;
  int y1 = tft.height() - 20;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + CANVAS_W - 1;
  int ty1 = ty0 + CANVAS_H - 1;
  if (x1 < tx0 || x0 > tx1 || y1 < ty0 || y0 > ty1) return;

  uint16_t panel = rgb565(6, 10, 16);
  uint16_t edge  = rgb565(40, 70, 40);
  g.fillRoundRect(x0 + ox, y0 + oy, (x1 - x0), (y1 - y0), 6, panel);
  g.drawRoundRect(x0 + ox, y0 + oy, (x1 - x0), (y1 - y0), 6, edge);

  int ty = y0 + 20;
  int txA = x0 + 14;
  int txB = x1 - 14;
  g.drawLine(txA + ox, ty + oy, txB + ox, ty + oy, rgb565(34, 54, 34));
  g.drawLine(txA + ox, ty + 2 + oy, txB + ox, ty + 2 + oy, rgb565(22, 34, 22));

  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    uint32_t age = nowMs - beltItems[i].bornMs;
    if (age > BELT_LIFE_MS) continue;

    float t = (float)age / (float)BELT_LIFE_MS;
    t = clampf(t, 0.0f, 1.0f);
    float u = 1.0f - (1.0f - t) * (1.0f - t);

    int x = (int)(txA + u * (float)(txB - txA));
    int y = ty + 1;

    int r = (age < 220) ? 4 : (t < 0.85f ? 3 : 2);
    g.fillCircle(x + ox, y + oy, r, COL_POLLEN);
    g.drawCircle(x + ox, y + oy, r, (t < 0.75f) ? COL_WHITE : COL_YEL);
    g.drawPixel(x + 1 + ox, y - 1 + oy, COL_POLLEN_HI);
  }

  g.setTextSize(1);
  g.setTextColor(COL_UI_DIM);
  g.setCursor(x0 + 10 + ox, y0 + 6 + oy);
  g.print("DELIVERIES");
}

static void drawSurvivalBar(Adafruit_GFX &g, int ox, int oy) {
  int barW = tft.width() - 12;
  int barH = 6;
  int x0 = 6;
  int y0 = tft.height() - 8;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + CANVAS_W - 1;
  int ty1 = ty0 + CANVAS_H - 1;
  if ((x0 + barW) < tx0 || x0 > tx1 || (y0 + barH) < ty0 || y0 > ty1) return;

  float pct = clampf(survivalTimeLeft / SURVIVAL_TIME_MAX, 0.0f, 1.0f);
  int fillW = (int)(pct * (float)barW);

  uint16_t fillColor;
  if (pct > 0.80f) {
    fillColor = COL_UI_GO;
  } else if (pct > 0.40f) {
    fillColor = COL_YEL;
  } else if (pct > 0.20f) {
    fillColor = rgb565(255, 140, 0);
  } else {
    fillColor = COL_UI_WARN;
  }

  uint16_t bgColor = rgb565(20, 20, 25);
  uint16_t borderColor = rgb565(60, 70, 80);

  g.fillRect(x0 + ox, y0 + oy, barW, barH, bgColor);
  g.drawRect(x0 + ox, y0 + oy, barW, barH, borderColor);

  bool critical = (pct <= 0.20f);
  bool blinkOn = critical && ((millis() % 400) < 200);

  if (fillW > 0) {
    uint16_t liveColor = blinkOn ? COL_UI_WARN : fillColor;
    g.fillRect(x0 + ox, y0 + oy, fillW, barH, liveColor);
  }

  if ((int32_t)(millis() - survivalFlashUntilMs) < 0) {
    int startW = (int)(survivalFlashStartPct * (float)barW);
    int endW = (int)(survivalFlashEndPct * (float)barW);
    if (endW > startW) {
      int fx = x0 + startW;
      int fw = endW - startW;
      g.fillRect(fx + ox, y0 + oy, fw, barH, COL_POLLEN_HI);
    }
  }

  if (blinkOn) {
    if (fillW > 0) {
      g.drawRect(x0 + ox, y0 + oy, fillW, barH, COL_WHITE);
      if (fillW > 2 && barH > 2) {
        g.drawRect(x0 + 1 + ox, y0 + 1 + oy, fillW - 2, barH - 2, COL_WHITE);
      }
    }
  }
}

static void drawGameOver(Adafruit_GFX &g, int ox, int oy) {
  int panelW = 200;
  int panelH = 100;
  int panelX = (tft.width() - panelW) / 2;
  int panelY = (tft.height() - panelH) / 2 - 20;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + CANVAS_W - 1;
  int ty1 = ty0 + CANVAS_H - 1;
  if ((panelX + panelW) < tx0 || panelX > tx1 || (panelY + panelH) < ty0 || panelY > ty1) return;

  uint16_t panelBg = rgb565(30, 40, 60);
  uint16_t panelBorder = rgb565(120, 180, 220);
  g.fillRoundRect(panelX + ox, panelY + oy, panelW, panelH, 8, panelBg);
  g.drawRoundRect(panelX + ox, panelY + oy, panelW, panelH, 8, panelBorder);
  g.drawRoundRect(panelX + 1 + ox, panelY + 1 + oy, panelW - 2, panelH - 2, 7, panelBorder);

  const char* messages[] = {
    "Bee-autiful!",
    "Buzz-tastic!",
    "Sweet Flying!",
    "You're the Bee!",
    "Amazing Work!",
    "Pollen Master!"
  };
  int msgIdx = score % 6;

  g.setTextWrap(false);

  g.setTextSize(2);
  g.setTextColor(COL_YEL);
  int titleW = strlen(messages[msgIdx]) * 12;
  int titleX = panelX + (panelW - titleW) / 2;
  g.setCursor(titleX + ox, panelY + 12 + oy);
  g.print(messages[msgIdx]);

  g.setTextSize(3);
  g.setTextColor(COL_WHITE);
  g.setCursor(panelX + panelW / 2 - 30 + ox, panelY + 38 + oy);
  g.print(score);

  g.setTextSize(1);
  g.setTextColor(COL_UI_DIM);
  g.setCursor(panelX + 28 + ox, panelY + 66 + oy);
  g.print("pollen delivered");

  if ((millis() % 800) < 400) {
    g.setTextSize(1);
    g.setTextColor(COL_UI_GO);
    g.setCursor(panelX + 30 + ox, panelY + 82 + oy);
    g.print("Press to play again");
  }

  g.setTextWrap(true);

  // Full-bar red at 0%
  int barW = tft.width() - 12;
  int barH = 6;
  int x0 = 6;
  int y0 = tft.height() - 8;
  int tx0b = -ox;
  int ty0b = -oy;
  int tx1b = tx0b + CANVAS_W - 1;
  int ty1b = ty0b + CANVAS_H - 1;
  if (!((x0 + barW) < tx0b || x0 > tx1b || (y0 + barH) < ty0b || y0 > ty1b)) {
    g.fillRect(x0 + ox, y0 + oy, barW, barH, COL_UI_WARN);
    if ((millis() % 700) < 350) {
      g.drawRect(x0 + ox, y0 + oy, barW, barH, COL_WHITE);
    }
  }
}

static void drawRadarOverlay(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  if (!radarActive) return;
  if ((int32_t)(nowMs - radarUntilMs) >= 0) { radarActive = false; return; }

  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;

  float t = 1.0f - (float)(radarUntilMs - nowMs) / 320.0f;
  t = clampf(t, 0.0f, 1.0f);

  float dx = (float)radarTargetWX - beeWX;
  float dy = (float)radarTargetWY - beeWY;
  float len = sqrtf(dx*dx + dy*dy);

  if (len < 1.0f) len = 1.0f;
  float ux = dx / len;
  float uy = dy / len;

  int r0 = 14 + (int)(t * 26.0f);
  uint16_t rc = radarToHive ? COL_HIVE : COL_YEL;
  g.drawCircle(cx, cy, r0, rc);
  g.drawCircle(cx, cy, r0 + 4, COL_WHITE);
  if (t > 0.35f) {
    int r1 = 10 + (int)((t - 0.35f) * 30.0f);
    g.drawCircle(cx, cy, r1, COL_UI_DIM);
  }

  int ax = cx + (int)(ux * 36.0f);
  int ay = cy + (int)(uy * 36.0f);
  for (int i = 6; i < 36; i += 6) {
    int sx = cx + (int)(ux * (float)i);
    int sy = cy + (int)(uy * (float)i);
    g.drawPixel(sx, sy, COL_WHITE);
  }
  for (int i = 12; i <= 36; i += 8) {
    int tx = cx + (int)(ux * (float)i);
    int ty = cy + (int)(uy * (float)i);
    int px = (int)(-uy * 2.0f);
    int py = (int)(ux * 2.0f);
    g.drawLine(tx - px, ty - py, tx + px, ty + py, COL_UI_DIM);
  }

  float px = -uy;
  float py = ux;
  int hx1 = ax - (int)(ux * 9.0f) + (int)(px * 5.0f);
  int hy1 = ay - (int)(uy * 9.0f) + (int)(py * 5.0f);
  int hx2 = ax - (int)(ux * 9.0f) - (int)(px * 5.0f);
  int hy2 = ay - (int)(uy * 9.0f) - (int)(py * 5.0f);
  g.fillTriangle(ax, ay, hx1, hy1, hx2, hy2, rc);

  g.setTextSize(1);
  g.setTextColor(COL_UI_DIM);
  g.setCursor(cx + 40, cy - 10);
  g.print((int)len);
}

// -------------------- RENDER FRAME --------------------
void renderFrame(uint32_t nowMs) {
  int hiveSX, hiveSY;
  worldToScreen(0, 0, hiveSX, hiveSY);

  for (int tileY = 0; tileY < tft.height(); tileY += CANVAS_H) {
    for (int tileX = 0; tileX < tft.width(); tileX += CANVAS_W) {
      int ox = -tileX;
      int oy = -tileY;

      canvas.fillScreen(COL_BG0);

      if ((tileX ^ tileY) & 0x80) {
        canvas.fillRect(0, 0, CANVAS_W, CANVAS_H, COL_BG1);
      }

      drawStarLayer(canvas, tileX, tileY, ox, oy, 0.25f, 48,  COL_STAR2, COL_STAR3, 0xA11CEu);
      drawStarLayer(canvas, tileX, tileY, ox, oy, 0.55f, 36,  COL_STAR,  COL_STAR2, 0xBEEFu);
      drawNebulaLayer(canvas, tileX, tileY, ox, oy, nowMs);
      drawWorldGrid(canvas, tileX, tileY, ox, oy);
      drawBoundaryZone(canvas, ox, oy);
      drawScreenAnchor(canvas, ox, oy, nowMs);

      if (hiveSX >= -40 && hiveSX <= tft.width() + 40 && hiveSY >= HUD_H - 40 && hiveSY <= tft.height() + 40) {
        drawHive(canvas, hiveSX + ox, hiveSY + oy);
        drawHivePulse(canvas, hiveSX + ox, hiveSY + oy, nowMs);
      }

      for (int i = 0; i < FLOWER_N; i++) {
        if (!flowers[i].alive) continue;
        int sx, sy;
        worldToScreen(flowers[i].wx, flowers[i].wy, sx, sy);
        if (sx < -30 || sx > tft.width() + 30 || sy < HUD_H - 30 || sy > tft.height() + 30) continue;
        drawFlower(canvas, sx + ox, sy + oy, flowers[i], nowMs, flowerBornMs[i]);
      }

      drawTrailParticles(canvas, ox, oy, nowMs);

      int bcX = beeScreenCX();
      int bcY = beeScreenCY();
      int bob = (int)(sinf((float)nowMs * 0.008f) * 2.0f);
      if ((int32_t)(nowMs - boostActiveUntilMs) < 0) {
        drawBoostAura(canvas, bcX + ox, bcY + oy + bob, nowMs);
      }
      drawBeeShadow(canvas, bcX + ox, bcY + oy + bob);
      drawBee(canvas, bcX + ox, bcY + oy + bob);
      drawPollenSparkles(canvas, bcX + ox, bcY + oy + bob, nowMs);
      drawScorePopups(canvas, ox, oy, nowMs);

      drawRadarOverlay(canvas, ox, oy, nowMs);
      drawBeltHUD(canvas, ox, oy, nowMs);
      drawSurvivalBar(canvas, ox, oy);
      drawHUDInTile(canvas, tileX, tileY, ox, oy);

      if (isGameOver) {
        drawGameOver(canvas, ox, oy);
      }

      tft.drawRGBBitmap(tileX, tileY, canvas.getBuffer(), CANVAS_W, CANVAS_H);
    }
  }
}
