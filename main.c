#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

// LCD (your existing wiring)
static const int PIN_BL   = 16;
static const int PIN_CS   = 17;
static const int PIN_SCK  = 18;
static const int PIN_MOSI = 19;
static const int PIN_DC   = 20;
static const int PIN_RST  = 21;

// Joystick (per mapping we locked in)
static const int PIN_JOY_SW  = 22; // GP22 (phys 29)
static const int PIN_JOY_VRX = 26; // GP26/ADC0 (phys 31)
static const int PIN_JOY_VRY = 27; // GP27/ADC1 (phys 32)

// ---------- BUZZER ----------
// Use a FREE GPIO (NOT 18 or 20 — those are SPI SCK + LCD DC).
// GP15 is a great choice; on Pico (40-pin) it's PHYSICAL PIN 20 (bottom-left when USB is at top).
static const int PIN_BUZZ = 15;   // GP15 (phys 20)

Adafruit_ST7789 tft(&SPI, PIN_CS, PIN_DC, PIN_RST);

// ---------- forward decls (Arduino auto-prototype fix) ----------
struct Flower;
static void drawFlower(Adafruit_GFX &g, const Flower &f);
static void drawHive(Adafruit_GFX &g, int cx, int cy);
static void drawBee(Adafruit_GFX &g, int x, int y);
static void drawBeeShadow(Adafruit_GFX &g, int x, int y);
static void drawBuzzRing(Adafruit_GFX &g, int x, int y);

// FX
static void drawDepositBeltFX(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
static void drawSparklesFX(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs);
static void updateFXLifetimes(uint32_t nowMs);
static bool anyFXActive();
static uint16_t fxFrameTag(uint32_t nowMs);

static void renderDirtyRect(int x0, int y0, int w, int h, int beeX, int beeY, bool ring, uint32_t nowMs);
static void tryCollectPollen(int bx, int by);
static void tryStoreAtHive(int bx, int by, uint32_t nowMs);
static void initFlowers();
static void fullRepaint(uint32_t nowMs);

// ---------- joystick helpers ----------
static int readJoyX() { return analogRead(PIN_JOY_VRX); } // 0..1023
static int readJoyY() { return analogRead(PIN_JOY_VRY); } // 0..1023
static bool joyPressedRaw() { return digitalRead(PIN_JOY_SW) == LOW; }

static int applyDeadzone(int v, int center, int dz) {
  int d = v - center;
  if (d > -dz && d < dz) return 0;
  return d;
}
static int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------- colors ----------
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static const uint16_t COL_BG        = rgb565(5, 10, 18);
static const uint16_t COL_BG_NOISE  = rgb565(8, 14, 26);
static const uint16_t COL_WHITE     = rgb565(240, 240, 240);
static const uint16_t COL_YEL       = rgb565(255, 220,  40);
static const uint16_t COL_BLK       = rgb565( 20,  20,  20);
static const uint16_t COL_WING      = rgb565(180, 220, 255);
static const uint16_t COL_HIVE      = rgb565(90, 140, 90);
static const uint16_t COL_HUD_BG    = rgb565(0, 0, 0);
static const uint16_t COL_POLLEN    = rgb565(255, 235, 110);
static const uint16_t COL_POLLEN_HI = rgb565(255, 255, 210);
static const uint16_t COL_SHADOW    = rgb565(0, 0, 0);
static const uint16_t COL_SHADOW_RIM= rgb565(20, 20, 20);

static const int HUD_H = 28;

// ---------- micro double-buffer (dirty rect) ----------
static const int CANVAS_W = 96;
static const int CANVAS_H = 72;
static GFXcanvas16 canvas(CANVAS_W, CANVAS_H);

// ---------- game state ----------
static float beeX = 0, beeY = 0;
static float beeVX = 0, beeVY = 0;

static int joyCenterX = 512;
static int joyCenterY = 512;

// observed extremes (auto-cal)
static int joyMinY = 1023;
static int joyMaxY = 0;
static const float DOWN_BOOST = 1.20f;

static bool hasPollen = false;  // only 1 at a time
static uint16_t score = 0;

// button edge
static bool btnPrev = false;
static bool ringPulse = false;
static uint32_t ringUntilMs = 0;

// ---- wing anim ----
static float wingPhase = 0.0f; // radians, advances with speed
static float wingSpeed = 0.0f; // 0..1

// ---- pollen chirp ----
static bool pollenChirpActive = false;
static bool pollenChirpPending = false;
static uint8_t pollenChirpStep = 0;
static uint32_t pollenChirpNextMs = 0;

// ---- flower respawn + ding ----
static bool flowerRespawnActive = false;
static uint8_t flowerRespawnRemaining = 0;
static uint32_t flowerRespawnNextMs = 0;
static uint8_t flowerDingQueue = 0;
static bool flowerDingActive = false;
static uint32_t flowerDingUntilMs = 0;

// RNG (xorshift32)
static uint32_t rngState = 0xA5A5F00Du;
static inline uint32_t xrnd() {
  uint32_t x = rngState;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  return (rngState = x);
}
static inline int irand(int lo, int hi) { // inclusive
  uint32_t r = xrnd();
  return lo + (int)(r % (uint32_t)(hi - lo + 1));
}

// ---------- flowers ----------
struct Flower {
  int x, y;
  uint8_t alive;
  uint8_t r;
  uint16_t petal;
  uint16_t petalLo;   // NEW: shadow tint
  uint16_t center;
};

static const int FLOWER_N = 6;
static Flower flowers[FLOWER_N];

// ---------- FX: persistent conveyor items + sparkles ----------
// Each drop-off spawns a BeltItem that travels right.
struct BeltItem {
  uint32_t bornMs;
  uint8_t alive;
};
static const int BELT_ITEM_N = 8;
static BeltItem beltItems[BELT_ITEM_N];
static const uint32_t BELT_LIFE_MS = 15000; // how long a delivered pollen remains visible on the belt

static void spawnBeltItem(uint32_t nowMs) {
  int freeIdx = -1;
  uint32_t oldest = 0xFFFFFFFFu;
  int oldestIdx = 0;

  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) { freeIdx = i; break; }
    if (beltItems[i].bornMs < oldest) { oldest = beltItems[i].bornMs; oldestIdx = i; }
  }

  int idx = (freeIdx >= 0) ? freeIdx : oldestIdx;
  beltItems[idx].alive = 1;
  beltItems[idx].bornMs = nowMs;
}

// Sparkles: small starbursts with per-spark lifetime.
struct Sparkle {
  int x, y;
  uint32_t bornMs;
  uint16_t c1, c2;
  uint8_t alive;
};
static const int SPARKLE_N = 10;
static Sparkle sparkles[SPARKLE_N];

static void spawnSparkles(int cx, int cy, uint32_t nowMs) {
  int made = 0;
  for (int i = 0; i < SPARKLE_N && made < 6; i++) {
    if (sparkles[i].alive) continue;
    Sparkle &s = sparkles[i];
    s.alive = 1;
    s.bornMs = nowMs;
    s.x = cx + irand(-12, 12);
    s.y = cy + irand(-12, 12);
    s.c1 = COL_POLLEN_HI;
    s.c2 = COL_YEL;
    made++;
  }
}

static void updateFXLifetimes(uint32_t nowMs) {
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;
    if ((uint32_t)(nowMs - beltItems[i].bornMs) > BELT_LIFE_MS) beltItems[i].alive = 0;
  }

  for (int i = 0; i < SPARKLE_N; i++) {
    if (!sparkles[i].alive) continue;
    if ((uint32_t)(nowMs - sparkles[i].bornMs) > 320) sparkles[i].alive = 0;
  }
}

static bool anyFXActive() {
  for (int i = 0; i < BELT_ITEM_N; i++) if (beltItems[i].alive) return true;
  for (int i = 0; i < SPARKLE_N; i++) if (sparkles[i].alive) return true;
  return false;
}

// IMPORTANT FIX:
// Dirty-rect redraw is event-driven, so time-based FX won't animate unless we force redraws.
// We compute a small frame tag that advances at ~20fps while FX is active.
static uint16_t fxFrameTag(uint32_t nowMs) {
  // 50ms = 20 fps. Lower = smoother but more SPI traffic.
  return (uint16_t)(nowMs / 50u);
}

// ---------- drawing ----------
static void drawHUD() {
  tft.fillRect(0, 0, tft.width(), HUD_H, COL_HUD_BG);
  tft.setCursor(6, 6);
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(2);
  tft.print("POLLEN ");
  tft.print(score);

  int left = 0;
  for (int i = 0; i < FLOWER_N; i++) if (flowers[i].alive) left++;

  tft.setTextSize(1);
  tft.setCursor(tft.width() - 106, 10);
  tft.print(hasPollen ? "CARRY" : "EMPTY");
  tft.setCursor(tft.width() - 106, 18);
  tft.print("FLR ");
  tft.print(left);
}

static void drawHive(Adafruit_GFX &g, int cx, int cy) {
  g.drawCircle(cx, cy, 12, COL_HIVE);
  g.drawCircle(cx, cy,  7, COL_HIVE);
  g.drawCircle(cx, cy,  2, COL_HIVE);
}

// NEW: stippled shadow ellipse (fake alpha)
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
      if (((xcol + yrow) & 1) == 0) {
        g.drawPixel(xcol, yrow, COL_SHADOW);
      }
    }
  }

  // rim hint
  g.drawFastHLine(x - rx + 2, sy, rx * 2 - 4, COL_SHADOW_RIM);
}

static void drawBee(Adafruit_GFX &g, int x, int y) {
  uint16_t body = hasPollen ? rgb565(255, 245, 140) : COL_YEL;

  // wing flap (velocity-driven)
  float s = sinf(wingPhase);
  int flap = (int)(s * (2 + (int)(3 * wingSpeed)));   // -5..+5-ish
  int wH   = 4 + (int)(2 * (0.5f + 0.5f * s));        // 4..6
  int wW   = 7 + (int)(2 * wingSpeed);                // 7..9

  // wings
  g.fillEllipse(x - 6, y - 9 + flap, wW, wH, COL_WING);
  g.fillEllipse(x + 2, y - 10 - flap/2, wW, wH, COL_WING);
  g.drawEllipse(x - 6, y - 9 + flap, wW, wH, COL_WHITE);
  g.drawEllipse(x + 2, y - 10 - flap/2, wW, wH, COL_WHITE);

  // tiny specular tick
  if (s > 0.35f) {
    g.drawPixel(x - 9, y - 12 + flap, COL_POLLEN_HI);
    g.drawPixel(x + 5, y - 13 - flap/2, COL_POLLEN_HI);
  }

  // body
  g.fillEllipse(x, y, 12, 8, body);
  g.fillRect(x - 9, y - 6, 4, 12, COL_BLK);
  g.fillRect(x - 1, y - 6, 4, 12, COL_BLK);
  g.drawEllipse(x, y, 12, 8, COL_WHITE);

  // head
  g.fillCircle(x + 11, y - 1, 5, COL_BLK);
  g.drawCircle(x + 11, y - 1, 5, COL_WHITE);

  // stinger
  g.fillTriangle(x - 13, y, x - 18, y - 2, x - 18, y + 2, COL_BLK);

  // pollen dot
  if (hasPollen) {
    g.fillCircle(x + 3, y + 8, 3, COL_YEL);
    g.drawCircle(x + 3, y + 8, 3, COL_WHITE);
  }
}

static void drawBuzzRing(Adafruit_GFX &g, int x, int y) {
  g.drawCircle(x, y, 18, COL_WHITE);
  g.drawCircle(x, y, 22, COL_YEL);

  // NEW: stippled outer halo (pseudo glow)
  int r = 26;
  for (int a = 0; a < 360; a += 10) {
    float ang = (float)a * 0.0174532925f;
    int px = x + (int)(cosf(ang) * r);
    int py = y + (int)(sinf(ang) * r);
    if (((px + py) & 1) == 0) g.drawPixel(px, py, COL_POLLEN_HI);
  }
}

static void drawFlower(Adafruit_GFX &g, const Flower &f) {
  if (!f.alive) return;
  int x = f.x, y = f.y, r = f.r;

  // NEW: subtle shadow underlay
  int sx = x + 1;
  int sy = y + 1;
  g.fillCircle(sx - r, sy, r, f.petalLo);
  g.fillCircle(sx + r, sy, r, f.petalLo);
  g.fillCircle(sx, sy - r, r, f.petalLo);
  g.fillCircle(sx, sy + r, r, f.petalLo);
  g.fillCircle(sx, sy, r, f.petalLo);

  // main petals
  g.fillCircle(x - r, y, r, f.petal);
  g.fillCircle(x + r, y, r, f.petal);
  g.fillCircle(x, y - r, r, f.petal);
  g.fillCircle(x, y + r, r, f.petal);
  g.fillCircle(x, y, r, f.petal);

  int cr = r / 2 + 2;
  g.fillCircle(x, y, cr, f.center);
  g.drawCircle(x, y, cr, COL_WHITE);

  // NEW: tiny highlight cluster
  g.drawPixel(x - 1, y - 1, COL_POLLEN_HI);
  g.drawPixel(x - 2, y - 1, COL_WHITE);
}

static void drawDepositBeltFX(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  // Track sits just below hive; items travel right along it.
  int hx = tft.width() / 2;
  int hy = (tft.height() + HUD_H) / 2;

  int trackY  = hy + 14;
  int trackX0 = hx + 8;
  int trackX1 = clampi(trackX0 + 120, 0, tft.width() - 1);

  // subtle track lines
  uint16_t trk = rgb565(40, 65, 40);
  g.drawLine(trackX0 + ox, trackY + oy, trackX1 + ox, trackY + oy, trk);
  g.drawLine(trackX0 + ox, trackY + 2 + oy, trackX1 + ox, trackY + 2 + oy, rgb565(28, 48, 28));

  // render each active item as a single pollen parcel moving right
  for (int i = 0; i < BELT_ITEM_N; i++) {
    if (!beltItems[i].alive) continue;

    uint32_t age = nowMs - beltItems[i].bornMs;
    if (age > BELT_LIFE_MS) continue;

    float t = (float)age / (float)BELT_LIFE_MS; // 0..1
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    // Ease-out motion
    float u = 1.0f - (1.0f - t) * (1.0f - t);

    int x = (int)(trackX0 + u * (float)(trackX1 - trackX0));
    int y = trackY + 1;

    // fake fade: change outline + size late
    int r = (age < 220) ? 4 : (t < 0.85f ? 3 : 2);

    g.fillCircle(x + ox, y + oy, r, COL_POLLEN);
    g.drawCircle(x + ox, y + oy, r, (t < 0.75f) ? COL_WHITE : COL_YEL);
    g.drawPixel(x + 1 + ox, y - 1 + oy, COL_POLLEN_HI);
  }
}

static void drawSparklesFX(Adafruit_GFX &g, int ox, int oy, uint32_t nowMs) {
  for (int i = 0; i < SPARKLE_N; i++) {
    Sparkle &s = sparkles[i];
    if (!s.alive) continue;

    uint32_t age = nowMs - s.bornMs;
    float t = (float)age / 320.0f;
    if (t < 0) t = 0;
    if (t > 1) t = 1;

    uint16_t c = (t < 0.55f) ? s.c1 : s.c2;

    int x = s.x + ox;
    int y = s.y + oy;

    g.drawPixel(x, y, c);
    g.drawPixel(x - 1, y, c);
    g.drawPixel(x + 1, y, c);
    g.drawPixel(x, y - 1, c);
    g.drawPixel(x, y + 1, c);

    if (t > 0.25f) {
      g.drawPixel(x - 1, y - 1, COL_WHITE);
      g.drawPixel(x + 1, y - 1, COL_WHITE);
      g.drawPixel(x - 1, y + 1, COL_WHITE);
      g.drawPixel(x + 1, y + 1, COL_WHITE);
    }
  }
}

static int floordiv(int a, int b) {
  if (a >= 0) return a / b;
  return -(((-a + b - 1) / b));
}

// NEW: deterministic background texture per tile (no shimmer)
static void drawTileBGNoise(uint32_t tileSeed) {
  uint32_t s = tileSeed;
  for (int i = 0; i < 28; i++) {
    // xorshift
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    int px = (int)(s % (uint32_t)CANVAS_W);
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    int py = (int)(s % (uint32_t)CANVAS_H);
    canvas.drawPixel(px, py, COL_BG_NOISE);
  }

  // a couple of tiny "stars"
  s ^= s << 13; s ^= s >> 17; s ^= s << 5;
  if ((s & 3u) == 0u) {
    int px = (int)((s >> 8) % (uint32_t)CANVAS_W);
    int py = (int)((s >> 16) % (uint32_t)CANVAS_H);
    canvas.drawPixel(px, py, COL_WHITE);
  }
}

static void renderDirtyRect(int x0, int y0, int w, int h, int beeXi, int beeYi, bool ring, uint32_t nowMs) {
  // clip to playfield (below HUD)
  if (y0 < HUD_H) { int d = HUD_H - y0; y0 = HUD_H; h -= d; }
  if (w <= 0 || h <= 0) return;

  // clip to screen
  int sx0 = x0;
  int sy0 = y0;
  int sx1 = x0 + w - 1;
  int sy1 = y0 + h - 1;

  if (sx0 < 0) sx0 = 0;
  if (sy0 < 0) sy0 = 0;
  if (sx1 >= tft.width())  sx1 = tft.width()  - 1;
  if (sy1 >= tft.height()) sy1 = tft.height() - 1;
  if (sx0 > sx1 || sy0 > sy1) return;

  // full tiles
  int startTx = floordiv(sx0, CANVAS_W) * CANVAS_W;
  int startTy = floordiv(sy0, CANVAS_H) * CANVAS_H;
  int endTx   = floordiv(sx1, CANVAS_W) * CANVAS_W;
  int endTy   = floordiv(sy1, CANVAS_H) * CANVAS_H;

  int maxX0 = tft.width()  - CANVAS_W;
  int maxY0 = tft.height() - CANVAS_H;
  if (maxX0 < 0) maxX0 = 0;
  if (maxY0 < HUD_H) maxY0 = HUD_H;

  for (int py0 = startTy; py0 <= endTy; py0 += CANVAS_H) {
    int ty0 = py0;
    if (ty0 < HUD_H) ty0 = HUD_H;
    if (ty0 > maxY0) ty0 = maxY0;

    for (int px0 = startTx; px0 <= endTx; px0 += CANVAS_W) {
      int tx0 = px0;
      if (tx0 < 0) tx0 = 0;
      if (tx0 > maxX0) tx0 = maxX0;

      canvas.fillScreen(COL_BG);
      uint32_t tileSeed = (uint32_t)(tx0 * 73856093u) ^ (uint32_t)(ty0 * 19349663u);
      drawTileBGNoise(tileSeed);

      int ox = -tx0;
      int oy = -ty0;

      // hive
      int hx = tft.width() / 2;
      int hy = (tft.height() + HUD_H) / 2;
      drawHive(canvas, hx + ox, hy + oy);

      // flowers
      for (int i = 0; i < FLOWER_N; i++) {
        if (!flowers[i].alive) continue;
        Flower f = flowers[i];
        f.x += ox;
        f.y += oy;
        drawFlower(canvas, f);
      }

      // FX under bee
      drawDepositBeltFX(canvas, ox, oy, nowMs);
      drawSparklesFX(canvas, ox, oy, nowMs);

      // shadow under bee
      drawBeeShadow(canvas, beeXi + ox, beeYi + oy);

      // bee + ring
      drawBee(canvas, beeXi + ox, beeYi + oy);
      if (ring) drawBuzzRing(canvas, beeXi + ox, beeYi + oy);

      tft.drawRGBBitmap(tx0, ty0, canvas.getBuffer(), CANVAS_W, CANVAS_H);
    }
  }
}

// ---------- setup helpers ----------
static void calibrateJoystick() {
  long sx = 0, sy = 0;
  const int N = 40;
  delay(30);
  for (int i = 0; i < N; i++) {
    sx += readJoyX();
    sy += readJoyY();
    delay(2);
  }
  joyCenterX = (int)(sx / N);
  joyCenterY = (int)(sy / N);
}

static void spawnFlower(int i) {
  Flower &f = flowers[i];
  f.alive = 1;
  f.r = (uint8_t)irand(6, 11);

  int cx = tft.width() / 2;
  int cy = (tft.height() + HUD_H) / 2;

  int x = 0, y = 0;
  for (int tries = 0; tries < 40; tries++) {
    x = irand(18, tft.width() - 18);
    y = irand(HUD_H + 18, tft.height() - 18);
    int dx = x - cx, dy = y - cy;
    if ((dx*dx + dy*dy) < (55*55)) continue;
    break;
  }

  f.x = x;
  f.y = y;

  // store petals as RGB table so we can derive a darker tint cheaply once
  struct RGB { uint8_t r,g,b; };
  static const RGB petals[] = {
    {255, 120, 180},
    {170, 120, 255},
    {120, 200, 255},
    {255, 170,  80},
    {120, 255, 170},
  };

  int pi = irand(0, (int)(sizeof(petals)/sizeof(petals[0])) - 1);
  RGB p = petals[pi];

  f.petal = rgb565(p.r, p.g, p.b);
  // darker underlay tint
  uint8_t r2 = (p.r > 48) ? (uint8_t)(p.r - 48) : 0;
  uint8_t g2 = (p.g > 48) ? (uint8_t)(p.g - 48) : 0;
  uint8_t b2 = (p.b > 48) ? (uint8_t)(p.b - 48) : 0;
  f.petalLo = rgb565(r2, g2, b2);

  f.center = rgb565(255, 235, 130);
}

static void initFlowers() {
  for (int i = 0; i < FLOWER_N; i++) {
    flowers[i].alive = 0;
    spawnFlower(i);
  }
}

static bool spawnNextDeadFlower() {
  for (int i = 0; i < FLOWER_N; i++) {
    if (!flowers[i].alive) {
      spawnFlower(i);
      return true;
    }
  }
  return false;
}

static bool allFlowersGone() {
  for (int i = 0; i < FLOWER_N; i++) if (flowers[i].alive) return false;
  return true;
}

static void startPollenChirp(uint32_t nowMs) {
  pollenChirpActive = true;
  pollenChirpStep = 0;
  pollenChirpNextMs = nowMs;
}

static void updatePollenChirp(uint32_t nowMs) {
  if (!pollenChirpActive) return;
  if ((int32_t)(nowMs - pollenChirpNextMs) < 0) return;

  switch (pollenChirpStep) {
    case 0:
      tone(PIN_BUZZ, 720, 60);
      pollenChirpNextMs = nowMs + 70;
      pollenChirpStep++;
      break;
    case 1:
      tone(PIN_BUZZ, 880, 60);
      pollenChirpNextMs = nowMs + 70;
      pollenChirpStep++;
      break;
    case 2:
      tone(PIN_BUZZ, 620, 80);
      pollenChirpNextMs = nowMs + 90;
      pollenChirpStep++;
      break;
    default:
      pollenChirpActive = false;
      noTone(PIN_BUZZ);
      break;
  }
}

static void startFlowerDing(uint32_t nowMs) {
  flowerDingActive = true;
  flowerDingUntilMs = nowMs + 120;
  tone(PIN_BUZZ, 980, 100);
}

static void updateFlowerDing(uint32_t nowMs) {
  if (!flowerDingActive) return;
  if ((int32_t)(nowMs - flowerDingUntilMs) >= 0) {
    flowerDingActive = false;
    noTone(PIN_BUZZ);
  }
}

static void updateFlowerRespawn(uint32_t nowMs) {
  if (!flowerRespawnActive) return;
  if ((int32_t)(nowMs - flowerRespawnNextMs) < 0) return;

  if (flowerRespawnRemaining > 0 && spawnNextDeadFlower()) {
    flowerRespawnRemaining--;
    flowerDingQueue++;
    fullRepaint(nowMs);
  }

  if (flowerRespawnRemaining == 0) {
    flowerRespawnActive = false;
  } else {
    flowerRespawnNextMs = nowMs + 260;
  }
}

static void fullRepaint(uint32_t nowMs) {
  int bx = (int)beeX;
  int by = (int)beeY;
  renderDirtyRect(0, HUD_H, tft.width(), tft.height() - HUD_H, bx, by, false, nowMs);
}

// ---------- rules ----------
static void tryCollectPollen(int bx, int by) {
  if (hasPollen) return;
  for (int i = 0; i < FLOWER_N; i++) {
    Flower &f = flowers[i];
    if (!f.alive) continue;
    int dx = bx - f.x;
    int dy = by - f.y;
    int hitR = (int)f.r + 10;
    if (dx*dx + dy*dy <= hitR*hitR) {
      hasPollen = true;
      f.alive = 0;
      pollenChirpPending = true;
      return;
    }
  }
}

static void tryStoreAtHive(int bx, int by, uint32_t nowMs) {
  if (!hasPollen) return;
  int cx = tft.width() / 2;
  int cy = (tft.height() + HUD_H) / 2;
  int dx = bx - cx;
  int dy = by - cy;
  const int hiveR = 18;
  if (dx*dx + dy*dy <= hiveR*hiveR) {
    hasPollen = false;
    score++;

    spawnBeltItem(nowMs);
    spawnSparkles(cx, cy, nowMs);

    drawHUD();

    if (allFlowersGone()) {
      flowerRespawnActive = true;
      flowerRespawnRemaining = FLOWER_N;
      flowerRespawnNextMs = nowMs + 200;
    }
  }
}

// ---------- sketch ----------
void setup() {
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  pinMode(PIN_JOY_SW, INPUT_PULLUP);

  // BUZZER
  pinMode(PIN_BUZZ, OUTPUT);
  digitalWrite(PIN_BUZZ, LOW);

  SPI.setSCK(PIN_SCK);
  SPI.setTX(PIN_MOSI);
  SPI.begin();

  tft.init(240, 320);
  tft.setRotation(1);

  // seed RNG
  rngState ^= (uint32_t)analogRead(PIN_JOY_VRX) << 16;
  rngState ^= (uint32_t)analogRead(PIN_JOY_VRY) << 1;
  rngState ^= (uint32_t)micros();

  tft.fillScreen(COL_BG);

  calibrateJoystick();

  joyMinY = joyCenterY;
  joyMaxY = joyCenterY;

  for (int i = 0; i < BELT_ITEM_N; i++) beltItems[i].alive = 0;
  for (int i = 0; i < SPARKLE_N; i++) sparkles[i].alive = 0;

  initFlowers();
  drawHUD();

  beeX = tft.width() * 0.5f;
  beeY = (tft.height() + HUD_H) * 0.5f;

  flowerRespawnActive = false;
  flowerRespawnRemaining = 0;
  flowerRespawnNextMs = 0;
  flowerDingQueue = 0;
  flowerDingActive = false;
  flowerDingUntilMs = 0;

  fullRepaint(millis());
}

void loop() {
  // dt
  static uint32_t lastMs = millis();
  uint32_t now = millis();
  uint32_t dtMs = now - lastMs;
  if (dtMs > 40) dtMs = 40;
  lastMs = now;
  float dt = (float)dtMs / 1000.0f;

  // FX maintenance
  updateFXLifetimes(now);

  // read joystick
  int x = readJoyX();
  int y = readJoyY();

  // update observed extremes continuously (auto-cal)
  if (y < joyMinY) joyMinY = y;
  if (y > joyMaxY) joyMaxY = y;

  const int dead = 35;

  int dx = applyDeadzone(x, joyCenterX, dead);
  int dy = applyDeadzone(y, joyCenterY, dead);

  // X symmetric
  float nx = -(float)clampi(dx, -512, 512) / 512.0f;

  // Y asymmetric
  int upSpan   = joyCenterY - joyMinY;
  int downSpan = joyMaxY - joyCenterY;
  if (upSpan < 1) upSpan = 1;
  if (downSpan < 1) downSpan = 1;

  float nyRaw;
  if (dy >= 0) {
    nyRaw = (float)dy / (float)downSpan;
    nyRaw *= DOWN_BOOST;
  } else {
    nyRaw = (float)dy / (float)upSpan;
  }

  if (nyRaw < -1.0f) nyRaw = -1.0f;
  if (nyRaw >  1.0f) nyRaw =  1.0f;

  float ny = -nyRaw;

  // diagonal boost (circle->square)
  float ax = fabsf(nx);
  float ay = fabsf(ny);
  float m  = (ax > ay) ? ax : ay;
  if (m > 0.0001f) {
    nx /= m;
    ny /= m;
    if (nx < -1) nx = -1; if (nx > 1) nx = 1;
    if (ny < -1) ny = -1; if (ny > 1) ny = 1;
  }

  float cx = tft.width() * 0.5f;
  float cy = (tft.height() + HUD_H) * 0.5f;

  const float roamX = (tft.width() * 0.45f);
  const float roamY = ((tft.height() - HUD_H) * 0.45f);

  float targetX = cx + nx * roamX;
  float targetY = cy + ny * roamY;

  if (dx == 0) targetX = cx;
  if (dy == 0) targetY = cy;

  // spring motion
  const float k = 40.0f;
  const float d = 10.0f;
  ax = k * (targetX - beeX) - d * beeVX;
  ay = k * (targetY - beeY) - d * beeVY;

  beeVX += ax * dt;
  beeVY += ay * dt;
  beeX  += beeVX * dt;
  beeY  += beeVY * dt;

  beeX = (float)clampi((int)beeX, 16, tft.width() - 16);
  beeY = (float)clampi((int)beeY, HUD_H + 16, tft.height() - 16);

  // ---- wing phase update (velocity-driven) ----
  float sp = fabsf(beeVX) + fabsf(beeVY);
  float spN = sp / 260.0f; // tune divisor
  if (spN > 1.0f) spN = 1.0f;
  wingSpeed = spN;

  // ---------- BUZZ (flap-driven) ----------
  // Cheap update cadence so tone() isn't spammed.
  static uint32_t lastBuzzMs = 0;
  if ((uint32_t)(now - lastBuzzMs) >= 40) { // ~25 Hz
    lastBuzzMs = now;

    if (!pollenChirpActive && !flowerDingActive) {
      if (wingSpeed > 0.08f) {
        int freq = 90 + (int)(wingSpeed * 360.0f); // ~120..450 Hz
        if (freq < 80)  freq = 80;
        if (freq > 520) freq = 520;
        tone(PIN_BUZZ, freq);
      } else {
        noTone(PIN_BUZZ);
      }
    }
  }

  float hz = 3.0f + 14.0f * wingSpeed; // 3..17 Hz
  wingPhase += 2.0f * 3.1415926f * hz * dt;
  if (wingPhase > 1000.0f) wingPhase -= 1000.0f;

  int bx = (int)beeX;
  int by = (int)beeY;

  // button edge => short ring pulse
  bool b = joyPressedRaw();
  if (b && !btnPrev) {
    ringPulse = true;
    ringUntilMs = now + 120;
  }
  btnPrev = b;
  if (ringPulse && now > ringUntilMs) ringPulse = false;

  // interactions
  tryCollectPollen(bx, by);
  if (pollenChirpPending && !pollenChirpActive) {
    pollenChirpPending = false;
    startPollenChirp(now);
  }
  tryStoreAtHive(bx, by, now);
  updatePollenChirp(now);
  updateFlowerDing(now);
  if (!pollenChirpActive && !flowerDingActive && flowerDingQueue > 0) {
    flowerDingQueue--;
    startFlowerDing(now);
  }
  updateFlowerRespawn(now);

  // dirty rect redraw only when needed
  static int lastX = -9999, lastY = -9999;
  static bool lastCarry = false;
  static bool lastRing = false;
  static bool lastFX = false;
  static uint16_t lastFxTag = 0;

  bool fx = anyFXActive();
  uint16_t tag = fx ? fxFrameTag(now) : 0;

  // FIX: include "tag changed" so belt animates even when bee is stationary
  bool fxTick = fx && (tag != lastFxTag);

  if (bx != lastX || by != lastY || hasPollen != lastCarry || ringPulse != lastRing || fx != lastFX || fxTick) {
    const int beeW = 52, beeH = 44;
    const int ringM = ringPulse ? 28 : 0;

    int hx = tft.width() / 2;
    int hy = (tft.height() + HUD_H) / 2;
    const int fxR = (fx ? 110 : 0); // belt extends right; keep generous

    int minX = (lastX < -1000) ? bx : ((bx < lastX) ? bx : lastX);
    int minY = (lastY < -1000) ? by : ((by < lastY) ? by : lastY);
    int maxX = (lastX < -1000) ? bx : ((bx > lastX) ? bx : lastX);
    int maxY = (lastY < -1000) ? by : ((by > lastY) ? by : lastY);

    int x0 = clampi(minX - beeW/2 - ringM, -10000, 10000);
    int y0 = clampi(minY - beeH/2 - ringM, -10000, 10000);
    int x1 = clampi(maxX + beeW/2 + ringM, -10000, 10000);
    int y1 = clampi(maxY + beeH/2 + ringM, -10000, 10000);

    if (fx) {
      int fx0 = hx - fxR;
      int fy0 = hy - fxR;
      int fx1 = hx + fxR;
      int fy1 = hy + fxR;
      if (fx0 < x0) x0 = fx0;
      if (fy0 < y0) y0 = fy0;
      if (fx1 > x1) x1 = fx1;
      if (fy1 > y1) y1 = fy1;
    }

    int w = (x1 - x0 + 1);
    int h = (y1 - y0 + 1);

    renderDirtyRect(x0, y0, w, h, bx, by, ringPulse, now);

    lastX = bx;
    lastY = by;
    lastCarry = hasPollen;
    lastRing = ringPulse;
    lastFX = fx;
    lastFxTag = tag;
  }

  delay(6);
}
