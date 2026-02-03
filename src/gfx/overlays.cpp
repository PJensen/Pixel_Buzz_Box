// Pixel Buzz Box - Overlay Effects Rendering
// Bee tether, radar overlay, magnetic field visualization
#include "gfx_internal.h"

// -------------------- BEE TETHER --------------------
void gfx_drawBeeTether(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  int hiveSX, hiveSY;
  worldToScreen(0, 0, hiveSX, hiveSY);

  int beeSX = beeScreenCX();
  int beeSY = beeScreenCY();

  float dx = (float)(hiveSX - beeSX);
  float dy = (float)(hiveSY - beeSY);
  float dist = sqrtf(dx * dx + dy * dy);

  if (dist < Vfx::TETHER_MIN_DIST) return;

  float ux = dx / dist;
  float uy = dy / dist;

  // Animated dotted line
  float offset = (float)((nowMs / Vfx::TETHER_ANIM_SPEED) % Vfx::TETHER_ANIM_CYCLE);

  for (float i = offset; i < dist; i += Vfx::TETHER_DOT_SPACING) {
    int sx = beeSX + (int)(ux * i);
    int sy = beeSY + (int)(uy * i);

    // Alternate between colors for shimmer effect
    uint16_t c = ((int)(i / Vfx::TETHER_DOT_SPACING) & 1)
      ? Color::TETHER_PRIMARY : Color::TETHER_ACCENT;

    g.fillCircle(sx + ox, sy + oy, 1, c);

    // Add occasional brighter dots
    if ((int)(i / Vfx::TETHER_DOT_SPACING) % 4 == 0) {
      g.drawPixel(sx + ox, sy + oy, Color::WHITE);
    }
  }
}

// -------------------- RADAR OVERLAY --------------------
void gfx_drawRadarOverlay(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  // Handle full radar (persistent when at max pollen)
  if (radar.fullActive && !radar.active) {
    bool blinkOn = ((nowMs / Vfx::RADAR_BLINK_PERIOD) % 2u) == 0u;
    if (!blinkOn) return;

    int cx = beeScreenCX() + ox;
    int cy = beeScreenCY() + oy;

    float dx = (float)radar.targetWX - bee.wx;
    float dy = (float)radar.targetWY - bee.wy;
    float len = sqrtf(dx*dx + dy*dy);

    if (len < 1.0f) len = 1.0f;
    float ux = dx / len;
    float uy = dy / len;

    uint16_t rc = Color::YEL;

    // Draw ring around bee
    g.drawCircle(cx, cy, Vfx::RADAR_RING_RADIUS, rc);

    // Draw arrow pointing to hive
    int ax = cx + (int)(ux * Vfx::RADAR_ARROW_DIST);
    int ay = cy + (int)(uy * Vfx::RADAR_ARROW_DIST);

    // Dotted line
    for (int i = Vfx::RADAR_DOT_START; i < Vfx::RADAR_ARROW_DIST; i += Vfx::RADAR_DOT_SPACING) {
      int sx = cx + (int)(ux * (float)i);
      int sy = cy + (int)(uy * (float)i);
      g.drawPixel(sx, sy, Color::YEL);
    }

    // Arrow head
    float px = -uy;
    float py = ux;
    int hx1 = ax - (int)(ux * Vfx::RADAR_ARROW_BACK) + (int)(px * Vfx::RADAR_ARROW_SPREAD);
    int hy1 = ay - (int)(uy * Vfx::RADAR_ARROW_BACK) + (int)(py * Vfx::RADAR_ARROW_SPREAD);
    int hx2 = ax - (int)(ux * Vfx::RADAR_ARROW_BACK) - (int)(px * Vfx::RADAR_ARROW_SPREAD);
    int hy2 = ay - (int)(uy * Vfx::RADAR_ARROW_BACK) - (int)(py * Vfx::RADAR_ARROW_SPREAD);
    g.fillTriangle(ax, ay, hx1, hy1, hx2, hy2, rc);

    // Distance text
    g.setTextSize(1);
    g.setTextColor(Color::YEL);
    g.setCursor(cx + Vfx::RADAR_TEXT_OFFSET_X, cy + Vfx::RADAR_TEXT_OFFSET_Y);
    g.print((int)len);
    return;
  }

  // Normal timed radar ping
  if (!radar.active) return;
  if ((int32_t)(nowMs - radar.untilMs) >= 0) { radar.active = false; return; }

  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;

  float t = 1.0f - (float)(radar.untilMs - nowMs) / (float)Timing::RADAR_DURATION_MS;
  t = clampf(t, 0.0f, 1.0f);

  float dx = (float)radar.targetWX - bee.wx;
  float dy = (float)radar.targetWY - bee.wy;
  float len = sqrtf(dx*dx + dy*dy);

  if (len < 1.0f) len = 1.0f;
  float ux = dx / len;
  float uy = dy / len;

  int r0 = Vfx::RADAR_PING_R0 + (int)(t * Vfx::RADAR_PING_EXPAND);
  uint16_t rc = radar.toHive ? Color::HIVE : Color::YEL;
  g.drawCircle(cx, cy, r0, rc);
  g.drawCircle(cx, cy, r0 + 4, Color::WHITE);
  if (t > Vfx::RADAR_PING_INNER_T) {
    int r1 = Vfx::RADAR_PING_R1 + (int)((t - Vfx::RADAR_PING_INNER_T) * Vfx::RADAR_PING_INNER_EXPAND);
    g.drawCircle(cx, cy, r1, Color::UI_DIM);
  }

  int arrowDist = Vfx::RADAR_PING_ARROW_DIST;
  int ax = cx + (int)(ux * (float)arrowDist);
  int ay = cy + (int)(uy * (float)arrowDist);
  for (int i = 6; i < arrowDist; i += 6) {
    int sx = cx + (int)(ux * (float)i);
    int sy = cy + (int)(uy * (float)i);
    g.drawPixel(sx, sy, Color::WHITE);
  }
  for (int i = 12; i <= arrowDist; i += 8) {
    int tx = cx + (int)(ux * (float)i);
    int ty = cy + (int)(uy * (float)i);
    int px = (int)(-uy * 2.0f);
    int py = (int)(ux * 2.0f);
    g.drawLine(tx - px, ty - py, tx + px, ty + py, Color::UI_DIM);
  }

  float px = -uy;
  float py = ux;
  int hx1 = ax - (int)(ux * Vfx::RADAR_PING_ARROW_BACK) + (int)(px * Vfx::RADAR_PING_ARROW_SPREAD);
  int hy1 = ay - (int)(uy * Vfx::RADAR_PING_ARROW_BACK) + (int)(py * Vfx::RADAR_PING_ARROW_SPREAD);
  int hx2 = ax - (int)(ux * Vfx::RADAR_PING_ARROW_BACK) - (int)(px * Vfx::RADAR_PING_ARROW_SPREAD);
  int hy2 = ay - (int)(uy * Vfx::RADAR_PING_ARROW_BACK) - (int)(py * Vfx::RADAR_PING_ARROW_SPREAD);
  g.fillTriangle(ax, ay, hx1, hy1, hx2, hy2, rc);

  g.setTextSize(1);
  g.setTextColor(Color::UI_DIM);
  g.setCursor(cx + Vfx::RADAR_PING_TEXT_X, cy + Vfx::RADAR_PING_TEXT_Y);
  g.print((int)len);
}

// -------------------- MAGNETIC FIELD --------------------
void gfx_drawMagneticField(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  if (!isMagnetActive(nowMs)) return;

  int cx = beeScreenCX() + ox;
  int cy = beeScreenCY() + oy;

  // Pulsing animation (faster with more charges)
  float pulseFreq = Vfx::MAGNET_PULSE_BASE + ((float)hive.magnet.chargesConsumed * Vfx::MAGNET_PULSE_PER_CHARGE);
  float pulse = sinf((float)nowMs * 0.001f * pulseFreq);

  // Multiple concentric rings (more rings = more charges)
  int ringCount = clampi(Vfx::MAGNET_RING_BASE + (int)(hive.magnet.chargesConsumed / 3), Vfx::MAGNET_RING_MIN, Vfx::MAGNET_RING_MAX);

  for (int i = 0; i < ringCount; i++) {
    float ringPhase = (float)i / (float)ringCount;
    float animPhase = fmodf(((float)nowMs * Vfx::MAGNET_RING_SPEED) + ringPhase, 1.0f);

    int baseRadius = Vfx::MAGNET_RING_R0 + (i * Vfx::MAGNET_RING_SPACING);
    int r = baseRadius + (int)(animPhase * Vfx::MAGNET_RING_EXPAND);

    // Color: Cyan to purple gradient based on strength
    uint8_t red = clampu8(100 + (int)(hive.magnet.strength * 40.0f));
    uint8_t green = clampu8(180 + (int)(pulse * 40.0f));
    uint8_t blue = 255;

    // Alpha fade based on animation
    float alpha = 1.0f - animPhase;
    red = (uint8_t)(red * alpha);
    green = (uint8_t)(green * alpha);
    blue = (uint8_t)(blue * alpha);

    uint16_t ringColor = rgb565(red, green, blue);

    g.drawCircle(cx, cy, r, ringColor);
    if (hive.magnet.strength > Vfx::MAGNET_THICK_THRESHOLD) {
      g.drawCircle(cx, cy, r + 1, ringColor);
    }

    // Sparkles on outer ring
    if (i == ringCount - 1 && ((nowMs + (i * 100)) % 200) < 100) {
      for (int s = 0; s < Vfx::MAGNET_SPARKLE_COUNT; s++) {
        float angle = (float)s * (MathConst::TAU / Vfx::MAGNET_SPARKLE_COUNT) + (float)nowMs * Vfx::MAGNET_SPARKLE_SPEED;
        int sx = cx + (int)(cosf(angle) * (float)r);
        int sy = cy + (int)(sinf(angle) * (float)r);
        g.drawPixel(sx, sy, Color::WHITE);
      }
    }
  }

  // Central glow
  int coreRadius = Vfx::MAGNET_CORE_R + (int)(pulse * 2.0f);
  g.fillCircle(cx, cy, coreRadius, Color::MAGNET_CORE);
  g.drawCircle(cx, cy, coreRadius + 2, Color::WHITE);

  // Field lines to nearby flowers
  for (int i = 0; i < Pool::FLOWER_N; i++) {
    if (!flowers[i].alive) continue;

    int32_t dx = flowers[i].wx - (int32_t)bee.wx;
    int32_t dy = flowers[i].wy - (int32_t)bee.wy;
    int32_t dist = (int32_t)sqrtf((float)(dx*dx + dy*dy));

    if (dist > Magnet::PULL_RADIUS) continue;

    int fx, fy;
    worldToScreen(flowers[i].wx, flowers[i].wy, fx, fy);

    float offset = (float)((nowMs / Vfx::MAGNET_LINE_ANIM_SPEED) % 6);
    float lineDist = sqrtf((float)((fx-cx)*(fx-cx) + (fy-cy)*(fy-cy)));
    if (lineDist < 1.0f) continue;
    float ux = (float)(fx - cx) / lineDist;
    float uy = (float)(fy - cy) / lineDist;

    float distNorm = (float)dist / (float)Magnet::PULL_RADIUS;
    float brightness = 1.0f - (distNorm * 0.5f);

    for (float d = offset; d < lineDist; d += Vfx::MAGNET_LINE_SPACING) {
      float segmentT = d / lineDist;
      int sx = cx + (int)(ux * d);
      int sy = cy + (int)(uy * d);

      uint8_t r = (uint8_t)(120 + (int)((1.0f - segmentT) * 80.0f * brightness));
      uint8_t green = (uint8_t)(220 + (int)((1.0f - segmentT) * 35.0f * brightness));
      uint8_t b = 255;
      uint16_t lineColor = rgb565(r, green, b);

      g.drawPixel(sx, sy, lineColor);
      g.drawPixel(sx + 1, sy, lineColor);
      g.drawPixel(sx, sy + 1, lineColor);

      if (((int)d % 12) < 3) {
        g.drawPixel(sx - 1, sy, Color::WHITE);
        g.drawPixel(sx, sy - 1, Color::WHITE);
      }
    }

    // Draw arrow at flower end
    int arrowDist = (int)(lineDist * Vfx::MAGNET_ARROW_POS);
    int arrowX = fx - (int)(ux * (float)arrowDist);
    int arrowY = fy - (int)(uy * (float)arrowDist);

    int px = (int)(-uy * 3.0f);
    int py = (int)(ux * 3.0f);
    g.drawLine(arrowX - px, arrowY - py, arrowX, arrowY, Color::MAGNET_ARROW);
    g.drawLine(arrowX + px, arrowY + py, arrowX, arrowY, Color::MAGNET_ARROW);
  }
}
