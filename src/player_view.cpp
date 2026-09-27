#include "player_view.h"

namespace {

constexpr int kRingThicknessPx = 8;
constexpr int kHeartBadgeRadiusPx = 34;
constexpr int kHeartBadgeCenterOffsetPx = 150;
constexpr float kRingStartAngleDeg = -90.0f;
const uint16_t kSpotifyGreen = lgfx::color565(30, 215, 96);
const uint16_t kHeartRed = lgfx::color565(233, 30, 99);
const uint16_t kRingTrackColor = lgfx::color565(60, 60, 60);

}  // namespace

void PlayerView::begin() {
  const int32_t width = M5.Display.width();
  const int32_t height = M5.Display.height();
  artworkCanvas_.setPsram(true);
  artworkCanvas_.setColorDepth(16);
  artworkCanvas_.createSprite(width, height);
  frameCanvas_.setPsram(true);
  frameCanvas_.setColorDepth(16);
  frameCanvas_.createSprite(width, height);
  frameCanvas_.setFont(&fonts::lgfxJapanGothicP_20);
  M5.Display.fillScreen(TFT_BLACK);
}

void PlayerView::setArtwork(const std::string& jpegBytes) {
  artworkCanvas_.fillScreen(TFT_BLACK);
  hasArtwork_ = !jpegBytes.empty();
  if (!hasArtwork_) {
    return;
  }
  const auto* jpegData = reinterpret_cast<const uint8_t*>(jpegBytes.data());
  // A zero scale makes M5GFX fit the image to the canvas, keeping its aspect.
  hasArtwork_ = artworkCanvas_.drawJpg(jpegData, jpegBytes.size(), 0, 0, artworkCanvas_.width(),
                                       artworkCanvas_.height(), 0, 0, 0.0f, 0.0f, datum_t::middle_center);
}

void PlayerView::render(const PlaybackState& playback, LikeStatus likeStatus) {
  if (!playback.hasTrack) {
    showMessage("Nothing playing");
    return;
  }
  artworkCanvas_.pushSprite(&frameCanvas_, 0, 0);
  if (!hasArtwork_) {
    drawTrackText(playback);
  }
  drawProgressRing(playback);
  drawHeart(likeStatus);
  if (!playback.isPlaying) {
    drawPauseIcon();
  }
  frameCanvas_.pushSprite(&M5.Display, 0, 0);
}

void PlayerView::showMessage(const char* message) {
  frameCanvas_.fillScreen(TFT_BLACK);
  frameCanvas_.setTextColor(TFT_WHITE);
  frameCanvas_.setTextDatum(datum_t::middle_center);
  frameCanvas_.drawString(message, frameCanvas_.width() / 2, frameCanvas_.height() / 2);
  frameCanvas_.pushSprite(&M5.Display, 0, 0);
}

void PlayerView::drawProgressRing(const PlaybackState& playback) {
  if (playback.durationMs == 0) {
    return;
  }
  const int32_t centerX = frameCanvas_.width() / 2;
  const int32_t centerY = frameCanvas_.height() / 2;
  const int32_t outerRadius = std::min(centerX, centerY) - 1;
  const int32_t innerRadius = outerRadius - kRingThicknessPx;
  const float progressRatio = std::min(1.0f, static_cast<float>(playback.progressMs) / playback.durationMs);
  const float progressEndAngle = kRingStartAngleDeg + 360.0f * progressRatio;
  frameCanvas_.fillArc(centerX, centerY, innerRadius, outerRadius, kRingStartAngleDeg, kRingStartAngleDeg + 360.0f,
                       kRingTrackColor);
  if (progressRatio > 0.0f) {
    frameCanvas_.fillArc(centerX, centerY, innerRadius, outerRadius, kRingStartAngleDeg, progressEndAngle,
                         kSpotifyGreen);
  }
}

void PlayerView::drawHeart(LikeStatus likeStatus) {
  if (likeStatus == LikeStatus::kUnknown) {
    return;
  }
  const int32_t centerX = frameCanvas_.width() / 2;
  const int32_t centerY = frameCanvas_.height() / 2 + kHeartBadgeCenterOffsetPx;
  const uint16_t heartColor = likeStatus == LikeStatus::kLiked ? kHeartRed : TFT_WHITE;
  constexpr int32_t lobeRadius = 9;
  constexpr int32_t lobeOffset = 8;
  frameCanvas_.fillCircle(centerX, centerY, kHeartBadgeRadiusPx, TFT_BLACK);
  frameCanvas_.fillCircle(centerX - lobeOffset, centerY - 4, lobeRadius, heartColor);
  frameCanvas_.fillCircle(centerX + lobeOffset, centerY - 4, lobeRadius, heartColor);
  frameCanvas_.fillTriangle(centerX - lobeOffset - lobeRadius, centerY - 1, centerX + lobeOffset + lobeRadius,
                            centerY - 1, centerX, centerY + 18, heartColor);
  if (likeStatus == LikeStatus::kNotLiked) {
    // Hollow out the white heart so that "not liked" reads as an outline.
    frameCanvas_.fillCircle(centerX - lobeOffset, centerY - 4, lobeRadius - 3, TFT_BLACK);
    frameCanvas_.fillCircle(centerX + lobeOffset, centerY - 4, lobeRadius - 3, TFT_BLACK);
    frameCanvas_.fillTriangle(centerX - lobeOffset - lobeRadius + 4, centerY - 1, centerX + lobeOffset + lobeRadius - 4,
                              centerY - 1, centerX, centerY + 13, TFT_BLACK);
  }
}

void PlayerView::drawPauseIcon() {
  const int32_t centerX = frameCanvas_.width() / 2;
  const int32_t centerY = frameCanvas_.height() / 2;
  frameCanvas_.fillCircle(centerX, centerY, 48, TFT_BLACK);
  frameCanvas_.fillRect(centerX - 18, centerY - 22, 12, 44, TFT_WHITE);
  frameCanvas_.fillRect(centerX + 6, centerY - 22, 12, 44, TFT_WHITE);
}

void PlayerView::drawTrackText(const PlaybackState& playback) {
  const int32_t centerX = frameCanvas_.width() / 2;
  const int32_t centerY = frameCanvas_.height() / 2;
  frameCanvas_.setTextDatum(datum_t::middle_center);
  frameCanvas_.setTextColor(TFT_WHITE);
  frameCanvas_.drawString(playback.title.c_str(), centerX, centerY - 16);
  frameCanvas_.setTextColor(TFT_LIGHTGREY);
  frameCanvas_.drawString(playback.artists.c_str(), centerX, centerY + 16);
}
