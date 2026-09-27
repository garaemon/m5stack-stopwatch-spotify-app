#include <unity.h>

#include "display_power_policy.h"

namespace {

constexpr uint32_t kDimAfterIdleMs = DisplayPowerPolicy::kDimAfterIdleMs;
constexpr uint32_t kOffAfterIdleMs = DisplayPowerPolicy::kOffAfterIdleMs;

DisplayPowerPolicy makePolicyTurnedOff() {
  DisplayPowerPolicy policy;
  policy.update(kOffAfterIdleMs, false);
  return policy;
}

}  // namespace

void should_stay_on_while_playing() {
  DisplayPowerPolicy policy;

  TEST_ASSERT_TRUE(policy.update(kOffAfterIdleMs * 10, true) == DisplayPower::kOn);
}

void should_stay_on_shortly_after_playback_stops() {
  DisplayPowerPolicy policy;

  TEST_ASSERT_TRUE(policy.update(kDimAfterIdleMs - 1, false) == DisplayPower::kOn);
}

void should_dim_after_idle_timeout() {
  DisplayPowerPolicy policy;

  TEST_ASSERT_TRUE(policy.update(kDimAfterIdleMs, false) == DisplayPower::kDimmed);
}

void should_turn_off_after_long_idle() {
  DisplayPowerPolicy policy;

  TEST_ASSERT_TRUE(policy.update(kOffAfterIdleMs, false) == DisplayPower::kOff);
}

void should_measure_idle_from_when_playback_stopped() {
  DisplayPowerPolicy policy;
  policy.update(100000, true);

  TEST_ASSERT_TRUE(policy.update(100000 + kDimAfterIdleMs - 1, false) == DisplayPower::kOn);
}

void should_turn_on_when_playback_resumes() {
  DisplayPowerPolicy policy = makePolicyTurnedOff();

  TEST_ASSERT_TRUE(policy.update(kOffAfterIdleMs + 1, true) == DisplayPower::kOn);
}

void should_report_wake_up_when_interacting_while_off() {
  DisplayPowerPolicy policy = makePolicyTurnedOff();

  TEST_ASSERT_TRUE(policy.registerInteraction(kOffAfterIdleMs + 1));
}

void should_not_report_wake_up_when_interacting_while_on() {
  DisplayPowerPolicy policy;

  TEST_ASSERT_FALSE(policy.registerInteraction(1000));
}

void should_turn_on_after_interaction_while_off() {
  DisplayPowerPolicy policy = makePolicyTurnedOff();
  policy.registerInteraction(kOffAfterIdleMs + 1);

  TEST_ASSERT_TRUE(policy.update(kOffAfterIdleMs + 2, false) == DisplayPower::kOn);
}

void should_restart_idle_timer_on_interaction() {
  DisplayPowerPolicy policy;
  policy.registerInteraction(kDimAfterIdleMs - 1);

  TEST_ASSERT_TRUE(policy.update(kDimAfterIdleMs, false) == DisplayPower::kOn);
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_stay_on_while_playing);
  RUN_TEST(should_stay_on_shortly_after_playback_stops);
  RUN_TEST(should_dim_after_idle_timeout);
  RUN_TEST(should_turn_off_after_long_idle);
  RUN_TEST(should_measure_idle_from_when_playback_stopped);
  RUN_TEST(should_turn_on_when_playback_resumes);
  RUN_TEST(should_report_wake_up_when_interacting_while_off);
  RUN_TEST(should_not_report_wake_up_when_interacting_while_on);
  RUN_TEST(should_turn_on_after_interaction_while_off);
  RUN_TEST(should_restart_idle_timer_on_interaction);
  return UNITY_END();
}
