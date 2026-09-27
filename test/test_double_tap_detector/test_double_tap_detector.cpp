#include <unity.h>

#include "double_tap_detector.h"

void should_not_trigger_on_single_tap() {
  DoubleTapDetector detector;

  TEST_ASSERT_FALSE(detector.registerTap(1000));
}

void should_trigger_on_second_quick_tap() {
  DoubleTapDetector detector;
  detector.registerTap(1000);

  TEST_ASSERT_TRUE(detector.registerTap(1000 + DoubleTapDetector::kMaxIntervalMs));
}

void should_not_trigger_when_second_tap_is_late() {
  DoubleTapDetector detector;
  detector.registerTap(1000);

  TEST_ASSERT_FALSE(detector.registerTap(1000 + DoubleTapDetector::kMaxIntervalMs + 1));
}

void should_trigger_when_late_tap_is_followed_by_quick_tap() {
  DoubleTapDetector detector;
  detector.registerTap(1000);
  detector.registerTap(2000);

  TEST_ASSERT_TRUE(detector.registerTap(2100));
}

void should_not_trigger_on_third_quick_tap() {
  DoubleTapDetector detector;
  detector.registerTap(1000);
  detector.registerTap(1100);

  TEST_ASSERT_FALSE(detector.registerTap(1200));
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_not_trigger_on_single_tap);
  RUN_TEST(should_trigger_on_second_quick_tap);
  RUN_TEST(should_not_trigger_when_second_tap_is_late);
  RUN_TEST(should_trigger_when_late_tap_is_followed_by_quick_tap);
  RUN_TEST(should_not_trigger_on_third_quick_tap);
  return UNITY_END();
}
