#pragma once

#include <M5Unified.h>

#include <string>

#include "app_controller.h"
#include "artwork_decoder.h"
#include "connection_status.h"

// Draws the now-playing screen on the round AMOLED display.
// Composes each frame off-screen in PSRAM so that updates do not flicker.
class PlayerView {
 public:
  void begin();
  // Shows the artwork behind every later render; nullptr shows black.
  void setArtwork(ArtworkImage artwork);
  void render(const PlaybackState& playback, LikeStatus likeStatus, ConnectionStatus connectionStatus);
  // Draws a centered message with an optional smaller second line.
  void showMessage(const char* message, const char* detail = nullptr);

 private:
  void drawProgressRing(const PlaybackState& playback);
  void drawHeart(LikeStatus likeStatus);
  void drawPauseIcon();
  void drawTrackText(const PlaybackState& playback);
  void updateFittedText(const PlaybackState& playback);
  void drawStatusBadge(const char* label);

  ArtworkImage artwork_;
  // Fitted text is recomputed only when the track changes, because render()
  // runs four times a second for the progress ring.
  std::string fittedTextTrackUri_;
  std::string fittedTitle_;
  std::string fittedArtists_;
  M5Canvas frameCanvas_{&M5.Display};
};
