// Pixel Buzz Box - UI Rendering
// HUD, belt, survival bar, game over
#include "gfx_internal.h"

// -------------------- HUD --------------------
void gfx_drawHUDInTile(Adafruit_GFX &g, int tileX, int tileY, int ox, int oy) {
  int y0 = tileY;
  int y1 = tileY + Display::CANVAS_H - 1;
  if (y0 > (Display::HUD_H - 1) || y1 < 0) return;

  // Sky is drawn behind the HUD - no solid background fill needed!
  // The sun/moon and sky gradient show through

  if (tileY != 0) return;

  g.setTextWrap(false);

  int leftX = 6 + ox;
  int rightX = tft.width() - 6 + ox;
  int line1Y = 6 + oy;
  int line2Y = 16 + oy;

  // === SCORE DISPLAY (top-right, size 2, yellow) ===
  g.setTextSize(2);
  g.setTextColor(Color::YEL);
  char scoreText[8];
  snprintf(scoreText, sizeof(scoreText), "%d", survival.score);
  int scoreWidth = strlen(scoreText) * 12;  // size 2: 12px per char
  int scoreX = rightX - scoreWidth;
  g.setCursor(scoreX, line1Y);
  g.print(scoreText);

  g.setTextSize(1);
  g.setTextColor(survival.pollenCount ? Color::YEL : Color::UI_DIM);
  g.setCursor(leftX, line1Y);
  if (survival.pollenCount) {
    g.print("CARRY ");
    g.print((int)survival.pollenCount);
  } else {
    g.print("EMPTY ");
    g.print("0");
  }
  g.print("/");
  g.print((int)Pool::MAX_POLLEN_CARRY);

  int rackCenterX = tft.width() / 2;
  int rackX = rackCenterX - 9 + ox;
  int rackY = line2Y - 2;
  int idx = 0;
  for (int ry = 0; ry < 2; ry++) {
    for (int rx = 0; rx < 4; rx++) {
      if (idx >= Pool::MAX_POLLEN_CARRY) break;
      int cx = rackX + rx * 6;
      int cy = rackY + ry * 6;
      uint16_t c = (idx < survival.pollenCount) ? Color::POLLEN : Color::UI_DIM;
      g.fillCircle(cx, cy, 2, c);
      if (idx < survival.pollenCount) g.drawPixel(cx + 1, cy - 1, Color::POLLEN_HI);
      idx++;
    }
  }

  // Score multiplier display
  float mult = getScoreMultiplier();
  if (mult > 1.0f) {
    char multText[12];
    snprintf(multText, sizeof(multText), "x%.1f", mult);
    g.setTextColor(Color::YEL);
    g.setCursor(leftX, line2Y);
    g.print(multText);
  }

  // Magnet status display - centered between pollen rack and score
  uint32_t now = millis();
  bool magnetCooldown = isMagnetOnCooldown(now);
  bool magnetReady = canActivateMagnet(now);
  bool bonusFlash = (int32_t)(hive.bonusFlashUntilMs - now) > 0;
  int rackRightEdge = rackCenterX + 15 + ox;  // rack ends ~15px right of center
  int magCenterX = (rackRightEdge + scoreX) / 2;  // center between rack and score

  if (isMagnetActive(now)) {
    // Active: show charges and time remaining
    g.setTextColor(Color::WING);  // Cyan color
    uint32_t timeLeftMs = hive.magnet.activeUntilMs - now;
    float timeLeftSec = (float)timeLeftMs / 1000.0f;
    char magnetText[16];
    snprintf(magnetText, sizeof(magnetText), "MAG[%d] %.1fs", (int)hive.magnet.chargesConsumed, timeLeftSec);
    int magnetW = (int)strlen(magnetText) * 6;
    g.setCursor(magCenterX - magnetW / 2, line2Y);
    g.print(magnetText);

  } else if (magnetCooldown) {
    // Cooldown: show CD and bonus info
    if (bonusFlash) {
      g.setTextColor(Color::POLLEN_HI);
      const char* cdText = "BONUS!";
      int cdW = (int)strlen(cdText) * 6;
      g.setCursor(magCenterX - cdW / 2, line2Y);
      g.print(cdText);
    } else {
      g.setTextColor(Color::UI_WARN);
      char cdText[16];
      snprintf(cdText, sizeof(cdText), "MAG CD [%d]", (int)hive.totalBonusCharges);
      int cdW = (int)strlen(cdText) * 6;
      g.setCursor(magCenterX - cdW / 2, line2Y);
      g.print(cdText);
    }

  } else if (magnetReady) {
    // Ready: show charges and ready indicator
    bool pulse = (now % 600) < 300;
    g.setTextColor(pulse ? Color::POLLEN_HI : Color::UI_GO);
    char readyText[16];
    snprintf(readyText, sizeof(readyText), "MAG[%d] RDY", (int)hive.totalBonusCharges);
    int readyW = (int)strlen(readyText) * 6;
    g.setCursor(magCenterX - readyW / 2, line2Y);
    g.print(readyText);

  } else {
    // Not ready: show progress toward first charge
    if (bonusFlash) {
      g.setTextColor(Color::POLLEN_HI);
      const char* bonusText = "BONUS!";
      int bonusW = (int)strlen(bonusText) * 6;
      g.setCursor(magCenterX - bonusW / 2, line2Y);
      g.print(bonusText);
    } else {
      g.setTextColor(Color::UI_DIM);
      float progress = hive.bonusPoints / Bonus::SECONDS_PER_CHARGE;
      char progressText[16];
      snprintf(progressText, sizeof(progressText), "MAG %.1f/%.0f", progress, Bonus::SECONDS_PER_CHARGE);
      int progressW = (int)strlen(progressText) * 6;
      g.setCursor(magCenterX - progressW / 2, line2Y);
      g.print(progressText);
    }
  }

  g.setTextWrap(true);
}

// -------------------- BELT HUD --------------------
void gfx_drawBeltHUD(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  int x0 = tft.width()  - 122;
  int y0 = tft.height() - 56;
  int x1 = tft.width()  - 6;
  int y1 = tft.height() - 20;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + Display::CANVAS_W - 1;
  int ty1 = ty0 + Display::CANVAS_H - 1;
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

  for (int i = 0; i < Pool::BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    uint32_t age = nowMs - beltItems[i].bornMs;
    if (age > Timing::BELT_LIFE_MS) continue;

    float t = (float)age / (float)Timing::BELT_LIFE_MS;
    t = clampf(t, 0.0f, 1.0f);
    float u = 1.0f - (1.0f - t) * (1.0f - t);

    int x = (int)(txA + u * (float)(txB - txA));
    int y = ty + 1;

    int r = (age < 220) ? 4 : (t < 0.85f ? 3 : 2);
    g.fillCircle(x + ox, y + oy, r, Color::POLLEN);
    g.drawCircle(x + ox, y + oy, r, (t < 0.75f) ? Color::WHITE : Color::YEL);
    g.drawPixel(x + 1 + ox, y - 1 + oy, Color::POLLEN_HI);
  }

  g.setTextSize(1);
  g.setTextColor(Color::UI_DIM);
  g.setCursor(x0 + 10 + ox, y0 + 6 + oy);
  g.print("DELIVERIES");
}

// -------------------- SURVIVAL BAR --------------------
void gfx_drawSurvivalBar(Adafruit_GFX &g, int ox, int oy) {
  int barW = tft.width() - 12;
  int barH = 6;
  int x0 = 6;
  int y0 = tft.height() - 8;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + Display::CANVAS_W - 1;
  int ty1 = ty0 + Display::CANVAS_H - 1;
  if ((x0 + barW) < tx0 || x0 > tx1 || (y0 + barH) < ty0 || y0 > ty1) return;

  float pct = clampf(survival.timeLeft / Survival::TIME_MAX, 0.0f, 1.0f);
  int fillW = (int)(pct * (float)barW);

  uint16_t fillColor;
  if (pct > 0.80f) {
    fillColor = Color::UI_GO;
  } else if (pct > 0.40f) {
    fillColor = Color::YEL;
  } else if (pct > 0.20f) {
    fillColor = rgb565(255, 140, 0);
  } else {
    fillColor = Color::UI_WARN;
  }

  uint16_t bgColor = rgb565(20, 20, 25);
  uint16_t borderColor = rgb565(60, 70, 80);

  g.fillRect(x0 + ox, y0 + oy, barW, barH, bgColor);
  g.drawRect(x0 + ox, y0 + oy, barW, barH, borderColor);

  bool critical = (pct <= 0.20f);
  bool blinkOn = critical && ((millis() % 400) < 200);

  if (fillW > 0) {
    uint16_t liveColor = blinkOn ? Color::UI_WARN : fillColor;
    g.fillRect(x0 + ox, y0 + oy, fillW, barH, liveColor);
  }

  if ((int32_t)(millis() - survival.flashUntilMs) < 0) {
    int startW = (int)(survival.flashStartPct * (float)barW);
    int endW = (int)(survival.flashEndPct * (float)barW);
    if (endW > startW) {
      int fx = x0 + startW;
      int fw = endW - startW;
      g.fillRect(fx + ox, y0 + oy, fw, barH, Color::POLLEN_HI);
    }
  }

  if (blinkOn) {
    if (fillW > 0) {
      g.drawRect(x0 + ox, y0 + oy, fillW, barH, Color::WHITE);
      if (fillW > 2 && barH > 2) {
        g.drawRect(x0 + 1 + ox, y0 + 1 + oy, fillW - 2, barH - 2, Color::WHITE);
      }
    }
  }
}

// -------------------- HIGH SCORE ENTRY --------------------
void gfx_drawHighScoreEntry(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  int panelW = 200;
  int panelH = 110;
  int panelX = (tft.width() - panelW) / 2;
  int panelY = (tft.height() - panelH) / 2 - 15;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + Display::CANVAS_W - 1;
  int ty1 = ty0 + Display::CANVAS_H - 1;
  if ((panelX + panelW) < tx0 || panelX > tx1 || (panelY + panelH) < ty0 || panelY > ty1) return;

  uint16_t panelBg = rgb565(30, 50, 70);
  uint16_t panelBorder = rgb565(255, 220, 80);
  g.fillRoundRect(panelX + ox, panelY + oy, panelW, panelH, 8, panelBg);
  g.drawRoundRect(panelX + ox, panelY + oy, panelW, panelH, 8, panelBorder);
  g.drawRoundRect(panelX + 1 + ox, panelY + 1 + oy, panelW - 2, panelH - 2, 7, panelBorder);

  g.setTextWrap(false);

  // Title
  g.setTextSize(2);
  g.setTextColor(Color::YEL);
  const char* title = "NEW HIGH SCORE!";
  int titleW = (int)strlen(title) * 12;
  int titleX = panelX + (panelW - titleW) / 2;
  g.setCursor(titleX + ox, panelY + 10 + oy);
  g.print(title);

  // Score display
  g.setTextSize(2);
  g.setTextColor(Color::WHITE);
  char scoreText[12];
  snprintf(scoreText, sizeof(scoreText), "%d", hsEntry.score);
  int scoreW = (int)strlen(scoreText) * 12;
  int scoreX = panelX + (panelW - scoreW) / 2;
  g.setCursor(scoreX + ox, panelY + 32 + oy);
  g.print(scoreText);

  // Rank display
  g.setTextSize(1);
  g.setTextColor(Color::UI_DIM);
  char rankText[16];
  snprintf(rankText, sizeof(rankText), "(Rank #%d)", hsEntry.rank + 1);
  int rankW = (int)strlen(rankText) * 6;
  int rankX = panelX + (panelW - rankW) / 2;
  g.setCursor(rankX + ox, panelY + 50 + oy);
  g.print(rankText);

  // Check if we're in saving state
  if (hsEntry.state == HS_SAVING) {
    g.setTextSize(2);
    g.setTextColor(Color::UI_GO);
    const char* savedText = "SAVED!";
    int savedW = (int)strlen(savedText) * 12;
    int savedX = panelX + (panelW - savedW) / 2;
    g.setCursor(savedX + ox, panelY + 70 + oy);
    g.print(savedText);
  } else {
    // Initials entry boxes
    int boxW = 28;
    int boxH = 28;
    int spacing = 8;
    int totalW = boxW * 3 + spacing * 2;
    int startX = panelX + (panelW - totalW) / 2;
    int boxY = panelY + 62;

    for (int i = 0; i < 3; i++) {
      int bx = startX + i * (boxW + spacing);
      bool selected = (i == hsEntry.cursorPos);

      // Box background
      uint16_t boxBg = selected ? rgb565(60, 80, 100) : rgb565(20, 30, 40);
      uint16_t boxBorder = selected ? Color::YEL : rgb565(80, 100, 120);
      g.fillRoundRect(bx + ox, boxY + oy, boxW, boxH, 4, boxBg);
      g.drawRoundRect(bx + ox, boxY + oy, boxW, boxH, 4, boxBorder);
      if (selected) {
        g.drawRoundRect(bx + 1 + ox, boxY + 1 + oy, boxW - 2, boxH - 2, 3, boxBorder);
      }

      // Character
      char ch = HighScore::CHAR_SET[hsEntry.charIndex[i]];
      g.setTextSize(2);
      g.setTextColor(selected ? Color::WHITE : Color::UI_DIM);
      g.setCursor(bx + 8 + ox, boxY + 6 + oy);
      g.print(ch);

      // Cursor indicator (blinking arrow below selected box)
      if (selected && ((nowMs % 400) < 250)) {
        int arrowX = bx + boxW / 2;
        int arrowY = boxY + boxH + 4;
        g.fillTriangle(
          arrowX - 4 + ox, arrowY + oy,
          arrowX + 4 + ox, arrowY + oy,
          arrowX + ox, arrowY + 6 + oy,
          Color::YEL
        );
      }
    }

    // Instructions
    g.setTextSize(1);
    g.setTextColor(Color::UI_DIM);
    const char* instrText = "Move: Joystick  OK: Button";
    int instrW = (int)strlen(instrText) * 6;
    int instrX = panelX + (panelW - instrW) / 2;
    g.setCursor(instrX + ox, panelY + 96 + oy);
    g.print(instrText);
  }

  g.setTextWrap(true);
}

// -------------------- HIGH SCORE TABLE --------------------
void gfx_drawHighScoreTable(Adafruit_GFX &g, int ox, int oy, int8_t highlightRank) {
  int panelW = 180;
  int panelH = 80;
  int panelX = (tft.width() - panelW) / 2;
  int panelY = tft.height() / 2 + 45;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + Display::CANVAS_W - 1;
  int ty1 = ty0 + Display::CANVAS_H - 1;
  if ((panelX + panelW) < tx0 || panelX > tx1 || (panelY + panelH) < ty0 || panelY > ty1) return;

  uint16_t panelBg = rgb565(20, 30, 45);
  uint16_t panelBorder = rgb565(80, 100, 130);
  g.fillRoundRect(panelX + ox, panelY + oy, panelW, panelH, 6, panelBg);
  g.drawRoundRect(panelX + ox, panelY + oy, panelW, panelH, 6, panelBorder);

  g.setTextWrap(false);

  // Title
  g.setTextSize(1);
  g.setTextColor(Color::YEL);
  const char* title = "HIGH SCORES";
  int titleW = (int)strlen(title) * 6;
  int titleX = panelX + (panelW - titleW) / 2;
  g.setCursor(titleX + ox, panelY + 6 + oy);
  g.print(title);

  // Entries (show top 3 to fit in panel)
  int entryY = panelY + 20;
  int showCount = (highScoreTable.count < 3) ? highScoreTable.count : 3;

  for (int i = 0; i < 3; i++) {
    bool hasEntry = (i < (int)highScoreTable.count);
    bool isHighlight = (i == highlightRank);

    int lineY = entryY + i * 18;

    if (isHighlight) {
      // Highlight bar
      g.fillRect(panelX + 4 + ox, lineY - 2 + oy, panelW - 8, 16, rgb565(50, 70, 90));
    }

    g.setTextSize(1);

    // Rank number
    g.setTextColor(isHighlight ? Color::YEL : Color::UI_DIM);
    g.setCursor(panelX + 10 + ox, lineY + oy);
    g.print(i + 1);
    g.print(".");

    if (hasEntry) {
      // Initials
      g.setTextColor(isHighlight ? Color::WHITE : Color::UI_DIM);
      g.setCursor(panelX + 34 + ox, lineY + oy);
      g.print(highScoreTable.entries[i].initials);

      // Score
      g.setTextColor(isHighlight ? Color::WHITE : Color::UI_DIM);
      char scoreText[8];
      snprintf(scoreText, sizeof(scoreText), "%d", highScoreTable.entries[i].score);
      g.setCursor(panelX + 70 + ox, lineY + oy);
      g.print(scoreText);

      // Day
      g.setTextColor(isHighlight ? Color::UI_GO : rgb565(80, 100, 80));
      char dayText[12];
      snprintf(dayText, sizeof(dayText), "Day %d", highScoreTable.entries[i].dayReached);
      g.setCursor(panelX + 120 + ox, lineY + oy);
      g.print(dayText);
    } else {
      // Empty slot
      g.setTextColor(rgb565(60, 60, 70));
      g.setCursor(panelX + 34 + ox, lineY + oy);
      g.print("---  ---");
    }
  }

  g.setTextWrap(true);
}

// -------------------- GAME OVER --------------------
void gfx_drawGameOver(Adafruit_GFX &g, int ox, int oy) {
  // If high score entry is active, show that instead
  if (isHighScoreEntryActive()) {
    gfx_drawHighScoreEntry(g, ox, oy, millis());
    return;
  }

  int panelW = 200;
  int panelH = 100;
  int panelX = (tft.width() - panelW) / 2;
  int panelY = (tft.height() - panelH) / 2 - 20;

  int tx0 = -ox;
  int ty0 = -oy;
  int tx1 = tx0 + Display::CANVAS_W - 1;
  int ty1 = ty0 + Display::CANVAS_H - 1;
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
  const char* nightMessages[] = {
    "Too Dark!",
    "Night Fell!",
    "Get Home!",
    "Past Bedtime!"
  };
  int msgIdx;
  const char* displayMsg;
  if (survival.diedAtNight) {
    msgIdx = survival.score % 4;
    displayMsg = nightMessages[msgIdx];
  } else {
    msgIdx = survival.score % 6;
    displayMsg = messages[msgIdx];
  }

  g.setTextWrap(false);

  g.setTextSize(2);
  g.setTextColor(survival.diedAtNight ? Color::UI_WARN : Color::YEL);
  int titleW = strlen(displayMsg) * 12;
  int titleX = panelX + (panelW - titleW) / 2;
  g.setCursor(titleX + ox, panelY + 12 + oy);
  g.print(displayMsg);

  g.setTextSize(3);
  g.setTextColor(Color::WHITE);
  char scoreText[12];
  snprintf(scoreText, sizeof(scoreText), "%d", survival.score);
  int scoreW = (int)strlen(scoreText) * 18;
  int scoreX = panelX + (panelW - scoreW) / 2;
  g.setCursor(scoreX + ox, panelY + 38 + oy);
  g.print(scoreText);

  g.setTextSize(1);
  g.setTextColor(Color::UI_DIM);
  const char* deliveredText = "pollen delivered";
  int deliveredW = (int)strlen(deliveredText) * 6;
  int deliveredX = panelX + (panelW - deliveredW) / 2;
  g.setCursor(deliveredX + ox, panelY + 62 + oy);
  g.print(deliveredText);

  // Show current high score if one exists
  if (highScoreTable.count > 0) {
    g.setTextSize(1);
    g.setTextColor(Color::YEL);
    char highText[20];
    snprintf(highText, sizeof(highText), "HIGH: %d", highScoreTable.entries[0].score);
    int highW = (int)strlen(highText) * 6;
    int highX = panelX + (panelW - highW) / 2;
    g.setCursor(highX + ox, panelY + 74 + oy);
    g.print(highText);
  }

  if ((millis() % 800) < 400) {
    g.setTextSize(1);
    g.setTextColor(Color::UI_GO);
    const char* playAgainText = "Press to play again";
    int playAgainW = (int)strlen(playAgainText) * 6;
    int playAgainX = panelX + (panelW - playAgainW) / 2;
    g.setCursor(playAgainX + ox, panelY + 86 + oy);
    g.print(playAgainText);
  }

  g.setTextWrap(true);

  // Draw high score table below
  gfx_drawHighScoreTable(g, ox, oy, hsEntry.rank);

  // Full-bar red at 0%
  int barW = tft.width() - 12;
  int barH = 6;
  int x0 = 6;
  int y0 = tft.height() - 8;
  int tx0b = -ox;
  int ty0b = -oy;
  int tx1b = tx0b + Display::CANVAS_W - 1;
  int ty1b = ty0b + Display::CANVAS_H - 1;
  if (!((x0 + barW) < tx0b || x0 > tx1b || (y0 + barH) < ty0b || y0 > ty1b)) {
    g.fillRect(x0 + ox, y0 + oy, barW, barH, Color::UI_WARN);
    if ((millis() % 700) < 350) {
      g.drawRect(x0 + ox, y0 + oy, barW, barH, Color::WHITE);
    }
  }
}
