#include "player_view.h"

#include <M5Unified.h>

#include <utility>

void PlayerView::begin() {
  M5.Display.setTextSize(2);
  M5.Display.fillScreen(TFT_BLACK);
}

void PlayerView::setArtwork(std::string jpegBytes) { artworkJpeg_ = std::move(jpegBytes); }

void PlayerView::render(const PlaybackState& playback, LikeStatus likeStatus) {
  (void)likeStatus;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.drawCenterString(playback.title.c_str(), M5.Display.width() / 2, M5.Display.height() / 2);
}

void PlayerView::showMessage(const char* message) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.drawCenterString(message, M5.Display.width() / 2, M5.Display.height() / 2);
}
