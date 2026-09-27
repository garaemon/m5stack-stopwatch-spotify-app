#pragma once

#include <M5Unified.h>

#include <string>

#include "app_controller.h"

// Draws the now-playing screen on the round AMOLED display.
// Composes each frame off-screen in PSRAM so that updates do not flicker.
class PlayerView {
 public:
  void begin();
  // Decodes the JPEG once; later renders reuse the decoded pixels.
  void setArtwork(const std::string& jpegBytes);
  void render(const PlaybackState& playback, LikeStatus likeStatus);
  void showMessage(const char* message);

 private:
  void drawProgressRing(const PlaybackState& playback);
  void drawHeart(LikeStatus likeStatus);
  void drawPauseIcon();
  void drawTrackText(const PlaybackState& playback);

  M5Canvas artworkCanvas_{&M5.Display};
  M5Canvas frameCanvas_{&M5.Display};
  bool hasArtwork_ = false;
};
