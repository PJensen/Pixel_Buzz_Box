// Pixel Buzz Box - Particle Effects Rendering
// Trail particles and score popups
#include "gfx_internal.h"

// -------------------- TRAIL PARTICLES --------------------
void gfx_drawTrailParticles(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  for (int i = 0; i < Pool::TRAIL_MAX; i++) {
    if (!trail[i].alive) continue;
    uint32_t age = nowMs - trail[i].bornMs;
    if (age > Vfx::TRAIL_LIFE_MS) continue;

    int sx, sy;
    worldToScreenF(trail[i].wx, trail[i].wy, sx, sy);

    float t = (float)age / (float)Vfx::TRAIL_LIFE_MS;
    float alpha = 1.0f - t * t;

    uint8_t baseR, baseG, baseB;

    // Variant 3 = gold particles for rare flowers
    if (trail[i].variant == Vfx::VARIANT_GOLD) {
      baseR = 255;
      baseG = (uint8_t)(220 - (int)(30.0f * t));
      baseB = (uint8_t)(100 - (int)(50.0f * t));
    } else if (trail[i].variant == Vfx::VARIANT_CYAN) {
      // Variant 4 = cyan particles for magnet
      baseR = (uint8_t)(100 + (int)(80.0f * (1.0f - t)));
      baseG = (uint8_t)(200 + (int)(55.0f * (1.0f - t)));
      baseB = 255;
    } else if (trail[i].variant == Vfx::VARIANT_RED) {
      // Variant 5 = red particles for wasp death
      baseR = 255;
      baseG = (uint8_t)(80 - (int)(60.0f * t));
      baseB = (uint8_t)(80 - (int)(60.0f * t));
    } else {
      float speedT = trail[i].speedN;
      baseR = (uint8_t)(255 - (int)(115.0f * speedT));
      baseG = (uint8_t)(220 - (int)(120.0f * speedT));
      baseB = (uint8_t)(60 + (int)(195.0f * speedT));
    }

    uint8_t r = (uint8_t)(baseR * alpha);
    uint8_t g_val = (uint8_t)(baseG * alpha);
    uint8_t b = (uint8_t)(baseB * alpha);

    if (alpha > Vfx::ALPHA_HIGH) {
      uint16_t outerGlow = rgb565(r / 3, g_val / 3, b / 3);
      g.fillCircle(sx + ox, sy + oy, Vfx::GLOW_RADIUS_OUTER, outerGlow);

      uint16_t midGlow = rgb565(r / 2, g_val / 2, b / 2);
      g.fillCircle(sx + ox, sy + oy, Vfx::GLOW_RADIUS_MID, midGlow);

      uint16_t core = rgb565(r, g_val, b);
      g.fillCircle(sx + ox, sy + oy, Vfx::GLOW_RADIUS_CORE, core);

      if (trail[i].variant == 0 && alpha > Vfx::ALPHA_SPARKLE) {
        uint16_t sparkle = rgb565(255, 255, 200);
        g.drawPixel(sx - Vfx::SPARKLE_OFFSET + ox, sy + oy, sparkle);
        g.drawPixel(sx + Vfx::SPARKLE_OFFSET + ox, sy + oy, sparkle);
        g.drawPixel(sx + ox, sy - Vfx::SPARKLE_OFFSET + oy, sparkle);
        g.drawPixel(sx + ox, sy + Vfx::SPARKLE_OFFSET + oy, sparkle);
      }
    } else if (alpha > Vfx::ALPHA_MID) {
      uint16_t midGlow = rgb565(r / 2, g_val / 2, b / 2);
      g.fillCircle(sx + ox, sy + oy, Vfx::GLOW_RADIUS_MID, midGlow);

      uint16_t core = rgb565(r, g_val, b);
      g.fillCircle(sx + ox, sy + oy, 1, core);
    } else {
      uint16_t dim = rgb565(r, g_val, b);
      g.drawPixel(sx + ox, sy + oy, dim);
    }
  }
}

// -------------------- SCORE POPUPS --------------------
void gfx_drawScorePopups(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  char buf[8];
  char multBuf[8];
  for (int i = 0; i < Pool::SCORE_POPUP_N; i++) {
    if (!scorePopups[i].alive) continue;
    uint32_t age = nowMs - scorePopups[i].bornMs;
    if (age > Timing::SCORE_POPUP_LIFE_MS) continue;

    float t = (float)age / (float)Timing::SCORE_POPUP_LIFE_MS;
    t = clampf(t, 0.0f, 1.0f);

    float u = 1.0f - (1.0f - t) * (1.0f - t);
    int floatY = (int)(Vfx::POPUP_FLOAT_HEIGHT * u);
    int sway = (int)(sinf((float)age * Vfx::POPUP_SWAY_FREQ + (float)scorePopups[i].driftX) * 2.0f);

    int cx = (int)scorePopups[i].baseSX + scorePopups[i].driftX + sway;
    int cy = (int)scorePopups[i].baseSY - Vfx::POPUP_BASE_OFFSET - floatY;

    int size;
    if (t < Vfx::POPUP_SIZE_T1) size = 1;
    else if (t < Vfx::POPUP_SIZE_T2) size = 2;
    else size = 3;

    // Draw pollen count (large)
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
    int tx1 = tx0 + Display::CANVAS_W - 1;
    int ty1 = ty0 + Display::CANVAS_H - 1;
    if (x1 < tx0 || x0 > tx1 || y1 < ty0 || y0 > ty1) continue;

    g.setTextWrap(false);
    g.setTextSize(size);

    g.setTextColor(Color::SHADOW);
    g.setCursor(x0 + 1 + ox, y0 + 1 + oy);
    g.print(buf);

    uint16_t mainCol = (t > Vfx::POPUP_HIGHLIGHT_T) ? Color::POLLEN_HI : Color::YEL;
    g.setTextColor(mainCol);
    g.setCursor(x0 + ox, y0 + oy);
    g.print(buf);

    if (t > Vfx::POPUP_SIZE_T2) {
      g.setTextColor(Color::WHITE);
      g.setCursor(x0 - 1 + ox, y0 + oy);
      g.print(buf);
      g.setCursor(x0 + 1 + ox, y0 - 1 + oy);
      g.print(buf);
    }

    // Draw multiplier below (small, baby blue)
    if (scorePopups[i].multiplier > 1.0f) {
      snprintf(multBuf, sizeof(multBuf), "x%.1f", scorePopups[i].multiplier);
      int multLen = (int)strlen(multBuf);
      int multW = multLen * 6;  // Size 1
      int multX = cx - multW / 2;
      int multY = y0 + textH + 2;

      g.setTextSize(1);
      g.setTextColor(Color::WING);
      g.setCursor(multX + ox, multY + oy);
      g.print(multBuf);
    }

    g.setTextWrap(true);
  }
}
