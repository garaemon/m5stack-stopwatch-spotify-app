#include "progress_estimator.h"

#include <algorithm>

uint32_t estimateProgressMs(const PlaybackState& playback, uint32_t elapsedMs) {
  if (!playback.isPlaying) {
    return playback.progressMs;
  }
  return std::min(playback.progressMs + elapsedMs, playback.durationMs);
}
