// Pixel Buzz Box - Entity Rendering
// Bee, flowers, hive, wasps
#include "gfx_internal.h"

// -------------------- BEE HELPERS --------------------
static void drawPollenOrbit(Adafruit_GFX &g, int x, int y) {
  if (survival.pollenCount == 0) return;
  int count = survival.pollenCount;
  float base = bee.wingPhase * 1.4f;
  int ring = 10 + (count / 3) * 2;
  int ringY = ring - 2;

  for (int i = 0; i < count; i++) {
    float ang = base + (6.2831853f * (float)i) / (float)count;
    int px = x + (int)(cosf(ang) * (float)ring);
    int py = y + 5 + (int)(sinf(ang) * (float)ringY);
    g.fillCircle(px, py, 2, Color::POLLEN);
    g.drawPixel(px + 1, py - 1, Color::POLLEN_HI);
  }

  if (survival.pollenCount >= Pool::MAX_POLLEN_CARRY) {
    g.drawCircle(x, y + 2, ring + 4, Color::POLLEN_HI);
  }
}

// -------------------- BEE --------------------
void gfx_drawBoostAura(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  float t = (float)(nowMs % 900u) / 900.0f;
  int r = 14 + (int)(4.0f * sinf(t * 6.2831853f));
  uint16_t c1 = rgb565(255, 210, 60);
  uint16_t c2 = rgb565(255, 240, 140);
  g.drawCircle(x, y, r, c1);
  g.drawCircle(x, y, r + 2, c2);
  g.drawCircle(x, y, r - 2, c1);
}

void gfx_drawPollenSparkles(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  if (survival.pollenCount == 0) return;
  int sparkles = clampi(4 + (int)survival.pollenCount, 4, 12);
  for (int i = 0; i < sparkles; i++) {
    uint32_t h = hash32(((uint32_t)nowMs >> 4) + (uint32_t)i * 977u);
    int dx = (int)(h & 0x1Fu) - 15;
    int dy = (int)((h >> 5) & 0x1Fu) - 15;
    if ((dx * dx + dy * dy) > 160) continue;
    uint16_t c = (i & 1) ? Color::POLLEN_HI : Color::WHITE;
    if (((h >> 11) & 1u) == 0u) g.drawPixel(x + dx, y + dy, c);
  }
}

void gfx_drawBeeShadow(Adafruit_GFX &g, int x, int y) {
  int sy = y + 14;
  float s = 0.5f + 0.5f * sinf(bee.wingPhase);
  int rx = 10 + (int)(3 * (1.0f - s)) + (int)(2 * bee.wingSpeed);
  int ry = 3  + (int)(2 * (1.0f - s));

  for (int yy = -ry; yy <= ry; yy++) {
    float yf = (float)yy / (float)ry;
    float inside = 1.0f - yf * yf;
    if (inside <= 0.0f) continue;
    int span = (int)(rx * sqrtf(inside));

    int yrow = sy + yy;
    for (int xx = -span; xx <= span; xx++) {
      int xcol = x + xx;
      if (((xcol + yrow) & 1) == 0) g.drawPixel(xcol, yrow, Color::SHADOW);
    }
  }
  g.drawFastHLine(x - rx + 2, sy, rx * 2 - 4, Color::SHADOW_RIM);
}

void gfx_drawBee(Adafruit_GFX &g, int x, int y) {
  float load = clampf((float)survival.pollenCount / (float)Pool::MAX_POLLEN_CARRY, 0.0f, 1.0f);
  uint8_t bodyR = 255;
  uint8_t bodyG = (uint8_t)(220 + (int)(25.0f * load));
  uint8_t bodyB = (uint8_t)(40 + (int)(120.0f * load));
  uint16_t body = rgb565(bodyR, bodyG, bodyB);

  float s = sinf(bee.wingPhase);
  int flap = (int)(s * (2 + (int)(3 * bee.wingSpeed)));
  int wH   = 4 + (int)(2 * (0.5f + 0.5f * s));
  int wW   = 7 + (int)(2 * bee.wingSpeed);
  uint8_t wr = clampu8(170 + (int)(55.0f * (0.5f + 0.5f * s)));
  uint8_t wg = clampu8(215 + (int)(35.0f * (0.5f + 0.5f * s)));
  uint8_t wb = 255;
  uint16_t wingCol = rgb565(wr, wg, wb);

  g.fillEllipse(x - 6, y - 9 + flap, wW, wH, wingCol);
  g.fillEllipse(x + 2, y - 10 - flap/2, wW, wH, wingCol);
  g.drawEllipse(x - 6, y - 9 + flap, wW, wH, Color::WHITE);
  g.drawEllipse(x + 2, y - 10 - flap/2, wW, wH, Color::WHITE);

  if (s > 0.35f) {
    g.drawPixel(x - 9, y - 12 + flap, Color::POLLEN_HI);
    g.drawPixel(x + 5, y - 13 - flap/2, Color::POLLEN_HI);
  }

  g.fillEllipse(x, y, 12, 8, body);
  g.fillRect(x - 9, y - 6, 4, 12, Color::BLK);
  g.fillRect(x - 1, y - 6, 4, 12, Color::BLK);
  g.drawEllipse(x, y, 12, 8, Color::WHITE);

  g.fillCircle(x + 11, y - 1, 5, Color::BLK);
  g.drawCircle(x + 11, y - 1, 5, Color::WHITE);

  g.fillTriangle(x - 13, y, x - 18, y - 2, x - 18, y + 2, Color::BLK);

  drawPollenOrbit(g, x, y);
}

// -------------------- HIVE --------------------
// Draw a hexagon outline centered at (x, y) with given radius
static void drawHexagon(Adafruit_GFX &g, int x, int y, int r, uint16_t color) {
  // Pointy-top hexagon: angles at 30, 90, 150, 210, 270, 330 degrees
  int px[6], py[6];
  for (int i = 0; i < 6; i++) {
    float angle = (MathConst::PI_F / 6.0f) + (MathConst::PI_F / 3.0f) * (float)i;
    px[i] = x + (int)(cosf(angle) * (float)r);
    py[i] = y + (int)(sinf(angle) * (float)r);
  }
  for (int i = 0; i < 6; i++) {
    int j = (i + 1) % 6;
    g.drawLine(px[i], py[i], px[j], py[j], color);
  }
}

void gfx_drawHive(Adafruit_GFX &g, int x, int y) {
  // Honey colors
  uint16_t honeyDark = rgb565(180, 120, 40);   // Dark amber
  uint16_t honeyMid  = rgb565(220, 160, 60);   // Golden honey
  uint16_t honeyLite = rgb565(255, 200, 80);   // Light honey highlight

  // Larger hexagons for a prominent hive
  drawHexagon(g, x, y, 22, honeyDark);
  drawHexagon(g, x, y, 16, honeyMid);
  drawHexagon(g, x, y, 10, honeyLite);
  drawHexagon(g, x, y,  4, honeyMid);
}

void gfx_drawHivePulse(Adafruit_GFX &g, int x, int y, uint32_t nowMs) {
  if ((int32_t)(nowMs - hivePulseUntilMs) >= 0) return;
  float t = 1.0f - (float)(hivePulseUntilMs - nowMs) / (float)Timing::HIVE_PULSE_MS;
  t = clampf(t, 0.0f, 1.0f);
  int r = 18 + (int)(t * 28.0f);
  uint16_t c1 = rgb565(255, 200, 80);   // Golden pulse
  uint16_t c2 = rgb565(255, 230, 150);  // Light honey glow
  drawHexagon(g, x, y, r, c1);
  drawHexagon(g, x, y, r + 5, c2);
  if ((nowMs & 0x3u) == 0u) {
    drawHexagon(g, x, y, r - 3, Color::WHITE);
  }
}

// -------------------- FLOWER --------------------
void gfx_drawFlower(Adafruit_GFX &g, int x, int y, const Flower &f, uint32_t nowMs, uint32_t bornMs) {
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
  g.drawCircle(x, y, cr, Color::WHITE);

  g.drawPixel(x - 1, y - 1, Color::POLLEN_HI);
  g.drawPixel(x - 2, y - 1, Color::WHITE);

  // Bloom animation on spawn (enhanced for rare flowers)
  uint32_t age = nowMs - bornMs;
  uint32_t bloomDuration = (f.type == FLOWER_RARE) ? 600 : 420;
  if (age < bloomDuration) {
    float t = (float)age / (float)bloomDuration;
    t = clampf(t, 0.0f, 1.0f);
    int growR = 1 + (int)(t * (float)(r + 2));

    // Rare flowers have gold bloom
    uint16_t bloomCore = (f.type == FLOWER_RARE) ? rgb565(255, 220, 120) : rgb565(255, 245, 200);
    g.fillCircle(x, y, growR, bloomCore);
    g.drawCircle(x, y, growR + 2, Color::WHITE);

    float ringT = 1.0f - t;
    int br = r + 8 + (int)(ringT * (f.type == FLOWER_RARE ? 14.0f : 10.0f));
    uint16_t bc = (f.type == FLOWER_RARE) ? rgb565(255, 200, 100) : rgb565(255, 235, 200);
    uint16_t bc2 = (f.type == FLOWER_RARE) ? rgb565(255, 235, 150) : rgb565(255, 250, 230);
    g.drawCircle(x, y, br, bc);
    g.drawCircle(x, y, br + 4, bc2);
    if ((age & 0x3u) == 0u) {
      g.drawCircle(x, y, br - 2, Color::WHITE);
      g.drawCircle(x, y, br + 1, Color::POLLEN_HI);
    }
    if ((age & 0x7u) == 0u) {
      int sparkR = br + 6;
      g.drawPixel(x + sparkR, y, bc2);
      g.drawPixel(x - sparkR, y, bc2);
      g.drawPixel(x, y + sparkR, bc2);
      g.drawPixel(x, y - sparkR, bc2);
    }
  }

  // Rare flowers have continuous shimmer/sparkle effect
  if (f.type == FLOWER_RARE && age > 600) {
    // Animated sparkles that rotate around the flower
    uint32_t sparkPhase = (nowMs / 80) % 8;  // 8-step rotation
    int sparkDist = r + 4;
    uint16_t sparkCol = Color::POLLEN_HI;

    // Primary sparkle
    if (sparkPhase == 0 || sparkPhase == 4) {
      g.drawPixel(x + sparkDist, y, sparkCol);
    } else if (sparkPhase == 1 || sparkPhase == 5) {
      g.drawPixel(x + sparkDist - 2, y - sparkDist + 2, sparkCol);
    } else if (sparkPhase == 2 || sparkPhase == 6) {
      g.drawPixel(x, y - sparkDist, sparkCol);
    } else if (sparkPhase == 3 || sparkPhase == 7) {
      g.drawPixel(x - sparkDist + 2, y - sparkDist + 2, sparkCol);
    }

    // Secondary sparkle (opposite side)
    uint32_t sparkPhase2 = (sparkPhase + 4) % 8;
    if (sparkPhase2 == 0 || sparkPhase2 == 4) {
      g.drawPixel(x - sparkDist, y, Color::WHITE);
    } else if (sparkPhase2 == 1 || sparkPhase2 == 5) {
      g.drawPixel(x - sparkDist + 2, y + sparkDist - 2, Color::WHITE);
    } else if (sparkPhase2 == 2 || sparkPhase2 == 6) {
      g.drawPixel(x, y + sparkDist, Color::WHITE);
    } else if (sparkPhase2 == 3 || sparkPhase2 == 7) {
      g.drawPixel(x + sparkDist - 2, y + sparkDist - 2, Color::WHITE);
    }
  }

  // Magnetic pull effect - pulsing cyan glow when being pulled
  if (isMagnetActive(nowMs)) {
    // Check if flower is within pull radius
    int32_t dx = f.wx - (int32_t)bee.wx;
    int32_t dy = f.wy - (int32_t)bee.wy;
    int32_t distSq = dx*dx + dy*dy;

    if (distSq <= (Magnet::PULL_RADIUS * Magnet::PULL_RADIUS)) {
      // Pulsing effect
      float pulse = sinf((float)nowMs * 0.012f);
      int glowRadius = r + 3 + (int)(pulse * 2.0f);

      // Cyan magnetic glow
      uint16_t magnetGlow = rgb565(100, 200, 255);
      g.drawCircle(x, y, glowRadius, magnetGlow);

      // Extra ring when pulse is strong
      if (pulse > 0.5f) {
        g.drawCircle(x, y, glowRadius + 2, rgb565(150, 220, 255));
      }
    }
  }
}

// -------------------- WASP (PREDATOR) --------------------
void gfx_drawWasp(Adafruit_GFX &g, int x, int y, const Wasp &w, uint32_t nowMs) {
  if (!w.alive) return;

  // Wing animation (faster than bee)
  float s = sinf(w.wingPhase);
  int flap = (int)(s * 3.5f);

  // Color varies by state - classic red and yellow wasp colors
  uint16_t bodyColor, stripeColor;
  if (w.state == WASP_STUNNED) {
    // Stunned: desaturated
    bodyColor = rgb565(120, 80, 80);
    stripeColor = rgb565(180, 160, 100);
  } else if (w.state == WASP_HUNTING) {
    // Hunting: intense red with bright yellow
    bodyColor = rgb565(220, 40, 40);
    stripeColor = rgb565(255, 240, 60);
  } else {
    // Patrol: standard red and yellow
    bodyColor = rgb565(180, 50, 50);
    stripeColor = rgb565(255, 220, 40);
  }

  // Wings (thin, fast-moving)
  uint16_t wingCol = rgb565(200, 200, 220);
  g.drawEllipse(x - 4, y - 8 + flap, 5, 3, wingCol);
  g.drawEllipse(x + 3, y - 9 - flap/2, 5, 3, wingCol);
  if (w.state == WASP_HUNTING && ((nowMs / 50) % 2)) {
    g.drawPixel(x - 6, y - 10 + flap, Color::WHITE);
    g.drawPixel(x + 5, y - 11 - flap/2, Color::WHITE);
  }

  // Body (elongated, segmented)
  g.fillEllipse(x, y, 10, 6, bodyColor);

  // Stripes (classic wasp pattern)
  g.fillRect(x - 7, y - 1, 2, 3, stripeColor);
  g.fillRect(x - 3, y - 1, 2, 3, stripeColor);
  g.fillRect(x + 2, y - 1, 2, 3, stripeColor);
  g.fillRect(x + 6, y - 1, 2, 3, stripeColor);

  // Outline
  g.drawEllipse(x, y, 10, 6, Color::WHITE);

  // Head (darker red)
  g.fillCircle(x + 9, y - 1, 4, rgb565(120, 30, 30));
  g.drawCircle(x + 9, y - 1, 4, Color::WHITE);

  // Eyes (red, menacing)
  uint16_t eyeColor = (w.state == WASP_HUNTING) ? rgb565(255, 60, 60) : rgb565(200, 80, 80);
  g.drawPixel(x + 10, y - 3, eyeColor);
  g.drawPixel(x + 10, y + 1, eyeColor);
  if (w.state == WASP_HUNTING) {
    g.drawPixel(x + 11, y - 2, eyeColor);
    g.drawPixel(x + 11, y, eyeColor);
  }

  // Stinger (at back)
  g.fillTriangle(x - 11, y, x - 15, y - 1, x - 15, y + 1, rgb565(40, 30, 20));

  // Danger indicator when hunting
  if (w.state == WASP_HUNTING && ((nowMs / 150) % 2)) {
    uint16_t dangerColor = rgb565(255, 100, 100);
    g.drawCircle(x, y, 14, dangerColor);
  }

  // Stunned indicator
  if (w.state == WASP_STUNNED) {
    // Stars spinning around head
    float starPhase = (float)(nowMs % 600) / 600.0f * 6.2831853f;
    for (int i = 0; i < 3; i++) {
      float angle = starPhase + (float)i * 2.094395f;  // 120 degrees apart
      int sx = x + (int)(cosf(angle) * 12.0f);
      int sy = y - 6 + (int)(sinf(angle) * 6.0f);
      g.drawPixel(sx, sy, Color::YEL);
    }
  }
}

void gfx_drawWaspDangerIndicator(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  // Show warning when any wasp is hunting (edge-of-screen indicator if offscreen)
  for (int i = 0; i < Pool::WASP_MAX; i++) {
    if (!wasps[i].alive) continue;
    if (wasps[i].state != WASP_HUNTING) continue;

    int sx, sy;
    worldToScreenF(wasps[i].wx, wasps[i].wy, sx, sy);

    // If wasp is offscreen, show edge indicator
    bool offscreen = (sx < -20 || sx > tft.width() + 20 ||
                      sy < Display::HUD_H - 20 || sy > tft.height() + 20);

    if (offscreen) {
      // Clamp to screen edge
      int edgeX = clampi(sx, 10, tft.width() - 10);
      int edgeY = clampi(sy, Display::HUD_H + 10, tft.height() - 10);

      // Blinking warning triangle
      if ((nowMs / 200) % 2) {
        uint16_t warnColor = rgb565(255, 80, 80);
        g.fillTriangle(edgeX + ox, edgeY - 6 + oy,
                       edgeX - 5 + ox, edgeY + 4 + oy,
                       edgeX + 5 + ox, edgeY + 4 + oy, warnColor);
        g.drawTriangle(edgeX + ox, edgeY - 6 + oy,
                       edgeX - 5 + ox, edgeY + 4 + oy,
                       edgeX + 5 + ox, edgeY + 4 + oy, Color::WHITE);
      }
    }
  }
}
