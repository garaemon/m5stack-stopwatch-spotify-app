#pragma once

#include <cstdint>

#include "playback_state.h"

// Returns the playback position after elapsedMs since Spotify reported
// playback, so the display can advance between polls. Never exceeds the
// track duration, and stays put while paused.
uint32_t estimateProgressMs(const PlaybackState& playback, uint32_t elapsedMs);
