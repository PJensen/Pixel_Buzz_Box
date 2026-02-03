// Pixel Buzz Box - Main Renderer
// Frame composition and render loop
#include "gfx_internal.h"

void renderFrame(uint32_t nowMs) {
  int hiveSX, hiveSY;
  worldToScreen(0, 0, hiveSX, hiveSY);

  for (int tileY = 0; tileY < tft.height(); tileY += Display::CANVAS_H) {
    for (int tileX = 0; tileX < tft.width(); tileX += Display::CANVAS_W) {
      int ox = -tileX;
      int oy = -tileY;

      // Sky zone first (behind everything, including HUD)
      gfx_drawSkyZone(canvas, tileX, tileY, ox, oy, nowMs);

      // Ground fill (below sky zone)
      gfx_drawGroundFill(canvas, tileX, tileY, ox, oy);

      // Grass layer (ground detail, behind clouds)
      gfx_drawGrassLayer(canvas, tileX, tileY, ox, oy, nowMs);

      // Cloud layers (parallax, floating above ground)
      gfx_drawCloudLayer(canvas, tileX, tileY, ox, oy, 0.15f, 96,  0xC10D1u, nowMs);
      gfx_drawCloudLayer(canvas, tileX, tileY, ox, oy, 0.35f, 72,  0xC10D2u, nowMs);

      // Floating pollen/seeds
      gfx_drawPollenLayer(canvas, tileX, tileY, ox, oy, nowMs);

      // Game boundary and anchor
      gfx_drawBoundaryZone(canvas, ox, oy);
      gfx_drawScreenAnchor(canvas, ox, oy, nowMs);

      // Hive
      if (hiveSX >= -40 && hiveSX <= tft.width() + 40 && hiveSY >= Display::HUD_H - 40 && hiveSY <= tft.height() + 40) {
        gfx_drawHive(canvas, hiveSX + ox, hiveSY + oy);
        gfx_drawHivePulse(canvas, hiveSX + ox, hiveSY + oy, nowMs);
      }

      // Tether line from bee to hive
      gfx_drawBeeTether(canvas, ox, oy, nowMs);

      // Flowers
      for (int i = 0; i < Pool::FLOWER_N; i++) {
        if (!flowers[i].alive) continue;
        int sx, sy;
        worldToScreen(flowers[i].wx, flowers[i].wy, sx, sy);
        if (sx < -30 || sx > tft.width() + 30 || sy < Display::HUD_H - 30 || sy > tft.height() + 30) continue;
        gfx_drawFlower(canvas, sx + ox, sy + oy, flowers[i], nowMs, flowerBornMs[i]);
      }

      // Wasps (predators)
      for (int i = 0; i < Pool::WASP_MAX; i++) {
        if (!wasps[i].alive) continue;
        int sx, sy;
        worldToScreenF(wasps[i].wx, wasps[i].wy, sx, sy);
        if (sx < -30 || sx > tft.width() + 30 || sy < Display::HUD_H - 30 || sy > tft.height() + 30) continue;
        gfx_drawWasp(canvas, sx + ox, sy + oy, wasps[i], nowMs);
      }
      gfx_drawWaspDangerIndicator(canvas, ox, oy, nowMs);

      // Effects
      gfx_drawTrailParticles(canvas, ox, oy, nowMs);

      // Bee
      int bcX = beeScreenCX();
      int bcY = beeScreenCY();
      int bob = (int)(sinf((float)nowMs * 0.008f) * 2.0f);
      if ((int32_t)(nowMs - bee.boostActiveUntilMs) < 0) {
        gfx_drawBoostAura(canvas, bcX + ox, bcY + oy + bob, nowMs);
      }
      gfx_drawBeeShadow(canvas, bcX + ox, bcY + oy + bob);
      gfx_drawBee(canvas, bcX + ox, bcY + oy + bob);
      gfx_drawPollenSparkles(canvas, bcX + ox, bcY + oy + bob, nowMs);
      gfx_drawMagneticField(canvas, ox, oy, nowMs);
      gfx_drawScorePopups(canvas, ox, oy, nowMs);

      // Overlays
      gfx_drawRadarOverlay(canvas, ox, oy, nowMs);
      gfx_drawBeltHUD(canvas, ox, oy, nowMs);
      gfx_drawSurvivalBar(canvas, ox, oy);
      gfx_drawHUDInTile(canvas, tileX, tileY, ox, oy);

      // Night warning overlay (before game over)
      gfx_drawNightWarning(canvas, tileX, tileY, ox, oy, nowMs);

      // Night rest overlay (safe at hive during night)
      gfx_drawNightRestOverlay(canvas, tileX, tileY, ox, oy, nowMs);

      // Day transition overlay (DAY X display)
      gfx_drawDayTransitionOverlay(canvas, tileX, tileY, ox, oy, nowMs);

      // Game over screen
      if (survival.isGameOver) {
        gfx_drawGameOver(canvas, ox, oy);
      }

      tft.drawRGBBitmap(tileX, tileY, canvas.getBuffer(), Display::CANVAS_W, Display::CANVAS_H);
    }
  }
}
