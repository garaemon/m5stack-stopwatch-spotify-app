#include <unity.h>

#include "progress_estimator.h"

namespace {

PlaybackState makePlayback(bool isPlaying, uint32_t progressMs, uint32_t durationMs) {
  PlaybackState playback;
  playback.hasTrack = true;
  playback.isPlaying = isPlaying;
  playback.progressMs = progressMs;
  playback.durationMs = durationMs;
  return playback;
}

}  // namespace

void should_advance_progress_while_playing() {
  TEST_ASSERT_EQUAL_UINT32(11500, estimateProgressMs(makePlayback(true, 10000, 200000), 1500));
}

void should_keep_progress_while_paused() {
  TEST_ASSERT_EQUAL_UINT32(10000, estimateProgressMs(makePlayback(false, 10000, 200000), 1500));
}

void should_clamp_progress_to_duration() {
  TEST_ASSERT_EQUAL_UINT32(200000, estimateProgressMs(makePlayback(true, 199000, 200000), 5000));
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_advance_progress_while_playing);
  RUN_TEST(should_keep_progress_while_paused);
  RUN_TEST(should_clamp_progress_to_duration);
  return UNITY_END();
}
