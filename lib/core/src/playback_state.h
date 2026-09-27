#pragma once

#include <cstdint>
#include <string>

// Snapshot of what Spotify reports as currently playing.
struct PlaybackState {
  bool hasTrack = false;
  bool isPlaying = false;
  std::string trackUri;  // "spotify:track:<id>"
  std::string title;
  std::string artists;  // Comma-separated artist names.
  std::string artworkUrl;
  uint32_t progressMs = 0;
  uint32_t durationMs = 0;
};
