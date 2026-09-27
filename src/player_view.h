#pragma once

#include <string>

#include "app_controller.h"

// Draws the now-playing screen on the round AMOLED display.
class PlayerView {
 public:
  void begin();
  // Keeps the decoded JPEG so that later renders skip decoding.
  void setArtwork(std::string jpegBytes);
  void render(const PlaybackState& playback, LikeStatus likeStatus);
  void showMessage(const char* message);

 private:
  std::string artworkJpeg_;
};
