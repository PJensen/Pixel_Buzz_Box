// Pixel Buzz Box - Unit Tests for Math and Physics
// Run with: pio test -e native
#include <unity.h>
#include <cmath>
#include <cstdint>

// ============================================================================
// Extracted utility functions (same as constants.h, but standalone for testing)
// ============================================================================

namespace MathConst {
  constexpr float DEG2RAD = 0.0174532925f;
  constexpr float TAU = 6.2831853f;
  constexpr float PI_F = 3.1415926f;
}

static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline uint8_t clampu8(int v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)v;
}

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ============================================================================
// Collision detection helper (extracted from game logic)
// ============================================================================

static inline bool circleCollision(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t radius) {
  int32_t dx = x2 - x1;
  int32_t dy = y2 - y1;
  return (dx * dx + dy * dy) <= (radius * radius);
}

static inline int64_t distanceSquared(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
  int64_t dx = (int64_t)(x2 - x1);
  int64_t dy = (int64_t)(y2 - y1);
  return dx * dx + dy * dy;
}

// ============================================================================
// Spring physics helper (extracted from bee physics)
// ============================================================================

static inline float springForce(float target, float current, float velocity, float springK, float damping) {
  return springK * (target - current) - damping * velocity;
}

// ============================================================================
// Tests: Clamp Functions
// ============================================================================

void test_clampi_within_range() {
  TEST_ASSERT_EQUAL_INT(5, clampi(5, 0, 10));
  TEST_ASSERT_EQUAL_INT(0, clampi(0, 0, 10));
  TEST_ASSERT_EQUAL_INT(10, clampi(10, 0, 10));
}

void test_clampi_below_range() {
  TEST_ASSERT_EQUAL_INT(0, clampi(-5, 0, 10));
  TEST_ASSERT_EQUAL_INT(-10, clampi(-100, -10, 10));
}

void test_clampi_above_range() {
  TEST_ASSERT_EQUAL_INT(10, clampi(15, 0, 10));
  TEST_ASSERT_EQUAL_INT(100, clampi(1000, 0, 100));
}

void test_clampf_within_range() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, clampf(0.5f, 0.0f, 1.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, clampf(0.0f, 0.0f, 1.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, clampf(1.0f, 0.0f, 1.0f));
}

void test_clampf_below_range() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, clampf(-0.5f, 0.0f, 1.0f));
}

void test_clampf_above_range() {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, clampf(1.5f, 0.0f, 1.0f));
}

void test_clampu8_within_range() {
  TEST_ASSERT_EQUAL_UINT8(128, clampu8(128));
  TEST_ASSERT_EQUAL_UINT8(0, clampu8(0));
  TEST_ASSERT_EQUAL_UINT8(255, clampu8(255));
}

void test_clampu8_below_range() {
  TEST_ASSERT_EQUAL_UINT8(0, clampu8(-10));
  TEST_ASSERT_EQUAL_UINT8(0, clampu8(-1000));
}

void test_clampu8_above_range() {
  TEST_ASSERT_EQUAL_UINT8(255, clampu8(300));
  TEST_ASSERT_EQUAL_UINT8(255, clampu8(1000));
}

// ============================================================================
// Tests: RGB565 Color Conversion
// ============================================================================

void test_rgb565_black() {
  TEST_ASSERT_EQUAL_HEX16(0x0000, rgb565(0, 0, 0));
}

void test_rgb565_white() {
  // White: R=248 (5 bits max), G=252 (6 bits max), B=248 (5 bits max)
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, rgb565(255, 255, 255));
}

void test_rgb565_red() {
  // Pure red: R=248, G=0, B=0 -> 11111 000000 00000 = 0xF800
  TEST_ASSERT_EQUAL_HEX16(0xF800, rgb565(255, 0, 0));
}

void test_rgb565_green() {
  // Pure green: R=0, G=252, B=0 -> 00000 111111 00000 = 0x07E0
  TEST_ASSERT_EQUAL_HEX16(0x07E0, rgb565(0, 255, 0));
}

void test_rgb565_blue() {
  // Pure blue: R=0, G=0, B=248 -> 00000 000000 11111 = 0x001F
  TEST_ASSERT_EQUAL_HEX16(0x001F, rgb565(0, 0, 255));
}

// ============================================================================
// Tests: Collision Detection
// ============================================================================

void test_circle_collision_touching() {
  // Two circles at (0,0) and (10,0) with combined radius 10 should touch
  TEST_ASSERT_TRUE(circleCollision(0, 0, 10, 0, 10));
}

void test_circle_collision_overlapping() {
  // Circles closer than radius
  TEST_ASSERT_TRUE(circleCollision(0, 0, 5, 0, 10));
}

void test_circle_collision_same_point() {
  // Same point, any positive radius
  TEST_ASSERT_TRUE(circleCollision(100, 100, 100, 100, 1));
}

void test_circle_collision_miss() {
  // Circles too far apart
  TEST_ASSERT_FALSE(circleCollision(0, 0, 20, 0, 10));
}

void test_circle_collision_diagonal() {
  // Diagonal: distance = sqrt(50) ~= 7.07, radius 7 should miss
  TEST_ASSERT_FALSE(circleCollision(0, 0, 5, 5, 7));
  // radius 8 should hit
  TEST_ASSERT_TRUE(circleCollision(0, 0, 5, 5, 8));
}

void test_distance_squared_basic() {
  TEST_ASSERT_EQUAL_INT64(100, distanceSquared(0, 0, 10, 0));
  TEST_ASSERT_EQUAL_INT64(100, distanceSquared(0, 0, 0, 10));
  TEST_ASSERT_EQUAL_INT64(200, distanceSquared(0, 0, 10, 10));
}

void test_distance_squared_negative_coords() {
  TEST_ASSERT_EQUAL_INT64(400, distanceSquared(-10, -10, 10, -10));
}

void test_distance_squared_large_values() {
  // Test with large values to ensure no overflow
  int32_t large = 10000;
  int64_t expected = 2LL * large * large;
  TEST_ASSERT_EQUAL_INT64(expected, distanceSquared(0, 0, large, large));
}

// ============================================================================
// Tests: Spring Physics
// ============================================================================

void test_spring_force_at_rest() {
  // At target with zero velocity: force should be zero
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, springForce(100.0f, 100.0f, 0.0f, 32.0f, 12.0f));
}

void test_spring_force_displaced() {
  // Target 100, current 0, velocity 0: force = 32 * (100 - 0) = 3200
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3200.0f, springForce(100.0f, 0.0f, 0.0f, 32.0f, 12.0f));
}

void test_spring_force_with_velocity() {
  // Target 100, current 50, velocity 10: force = 32 * 50 - 12 * 10 = 1600 - 120 = 1480
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1480.0f, springForce(100.0f, 50.0f, 10.0f, 32.0f, 12.0f));
}

void test_spring_force_overshooting() {
  // Past target: force should pull back
  // Target 100, current 150, velocity 20: force = 32 * -50 - 12 * 20 = -1600 - 240 = -1840
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -1840.0f, springForce(100.0f, 150.0f, 20.0f, 32.0f, 12.0f));
}

void test_spring_damping_only() {
  // At target but moving: damping should slow down
  // Target 100, current 100, velocity 50: force = 0 - 12 * 50 = -600
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -600.0f, springForce(100.0f, 100.0f, 50.0f, 32.0f, 12.0f));
}

// ============================================================================
// Tests: Math Constants
// ============================================================================

void test_math_const_tau() {
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f * MathConst::PI_F, MathConst::TAU);
}

void test_math_const_deg2rad() {
  // 180 degrees should equal PI
  TEST_ASSERT_FLOAT_WITHIN(0.001f, MathConst::PI_F, 180.0f * MathConst::DEG2RAD);
}

// ============================================================================
// Test Runner
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Clamp tests
  RUN_TEST(test_clampi_within_range);
  RUN_TEST(test_clampi_below_range);
  RUN_TEST(test_clampi_above_range);
  RUN_TEST(test_clampf_within_range);
  RUN_TEST(test_clampf_below_range);
  RUN_TEST(test_clampf_above_range);
  RUN_TEST(test_clampu8_within_range);
  RUN_TEST(test_clampu8_below_range);
  RUN_TEST(test_clampu8_above_range);

  // RGB565 tests
  RUN_TEST(test_rgb565_black);
  RUN_TEST(test_rgb565_white);
  RUN_TEST(test_rgb565_red);
  RUN_TEST(test_rgb565_green);
  RUN_TEST(test_rgb565_blue);

  // Collision tests
  RUN_TEST(test_circle_collision_touching);
  RUN_TEST(test_circle_collision_overlapping);
  RUN_TEST(test_circle_collision_same_point);
  RUN_TEST(test_circle_collision_miss);
  RUN_TEST(test_circle_collision_diagonal);
  RUN_TEST(test_distance_squared_basic);
  RUN_TEST(test_distance_squared_negative_coords);
  RUN_TEST(test_distance_squared_large_values);

  // Spring physics tests
  RUN_TEST(test_spring_force_at_rest);
  RUN_TEST(test_spring_force_displaced);
  RUN_TEST(test_spring_force_with_velocity);
  RUN_TEST(test_spring_force_overshooting);
  RUN_TEST(test_spring_damping_only);

  // Math constant tests
  RUN_TEST(test_math_const_tau);
  RUN_TEST(test_math_const_deg2rad);

  return UNITY_END();
}
