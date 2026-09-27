#include "player_view.h"

#include <utility>

#include "display_text.h"
#include "text_fitting.h"

namespace {

constexpr int kRingThicknessPx = 8;
constexpr int kHeartBadgeRadiusPx = 30;
constexpr int kHeartBadgeCenterOffsetPx = 172;
constexpr int kTitleCenterOffsetPx = 88;
constexpr int kArtistCenterOffsetPx = 120;
// Widths stay inside the circle chord at each text line with some margin.
constexpr int kTitleMaxWidthPx = 360;
constexpr int kArtistMaxWidthPx = 320;
constexpr float kRingStartAngleDeg = -90.0f;
const uint16_t kSpotifyGreen = lgfx::color565(30, 215, 96);
const uint16_t kHeartRed = lgfx::color565(233, 30, 99);
const uint16_t kRingTrackColor = lgfx::color565(60, 60, 60);
const uint16_t kBadgeColor = lgfx::color565(200, 60, 40);
constexpr int kBadgeTopPx = 40;
constexpr int kBadgeHeightPx = 32;
constexpr int kBadgePaddingPx = 16;
constexpr int kMessageLineGapPx = 36;

// Returns a short label for a status the user should notice, or nullptr.
const char* describeConnectionProblem(ConnectionStatus status) {
  switch (status) {
    case ConnectionStatus::kWifiDisconnected:
      return "Wi-Fi disconnected";
    case ConnectionStatus::kRateLimited:
      return "Rate limited";
    case ConnectionStatus::kServerError:
      return "Spotify error";
    case ConnectionStatus::kNetworkError:
      return "Network error";
    case ConnectionStatus::kAuthFailed:
      return "Spotify login expired";
    case ConnectionStatus::kConnecting:
    case ConnectionStatus::kOk:
      return nullptr;
  }
  return nullptr;
}

}  // namespace

void PlayerView::begin() {
  const int32_t width = M5.Display.width();
  const int32_t height = M5.Display.height();
  frameCanvas_.setPsram(true);
  frameCanvas_.setColorDepth(16);
  frameCanvas_.createSprite(width, height);
  M5.Display.fillScreen(TFT_BLACK);
}

void PlayerView::setArtwork(ArtworkImage artwork) { artwork_ = std::move(artwork); }

void PlayerView::render(const PlaybackState& playback, LikeStatus likeStatus, ConnectionStatus connectionStatus) {
  const char* problemLabel = describeConnectionProblem(connectionStatus);
  if (connectionStatus == ConnectionStatus::kAuthFailed) {
    showMessage(problemLabel, "Run tools/spotify_auth.py");
    return;
  }
  if (!playback.hasTrack) {
    if (connectionStatus == ConnectionStatus::kConnecting) {
      showMessage("Connecting...");
    } else {
      showMessage(problemLabel != nullptr ? problemLabel : "Nothing playing");
    }
    return;
  }
  if (artwork_ != nullptr) {
    artwork_->pushSprite(&frameCanvas_, 0, 0);
  } else {
    frameCanvas_.fillScreen(TFT_BLACK);
  }
  drawTrackText(playback);
  drawProgressRing(playback);
  drawHeart(likeStatus);
  if (!playback.isPlaying) {
    drawPauseIcon();
  }
  if (problemLabel != nullptr) {
    drawStatusBadge(problemLabel);
  }
  frameCanvas_.pushSprite(&M5.Display, 0, 0);
}

void PlayerView::showMessage(const char* message, const char* detail) {
  const int32_t centerX = frameCanvas_.width() / 2;
  const int32_t centerY = frameCanvas_.height() / 2;
  const int32_t messageY = detail != nullptr ? centerY - kMessageLineGapPx / 2 : centerY;
  frameCanvas_.fillScreen(TFT_BLACK);
  frameCanvas_.setTextDatum(datum_t::middle_center);
  frameCanvas_.setFont(&fonts::efontJA_24);
  frameCanvas_.setTextColor(TFT_WHITE);
  frameCanvas_.drawString(message, centerX, messageY);
  if (detail != nullptr) {
    frameCanvas_.setFont(&fonts::efontJA_16);
    frameCanvas_.setTextColor(TFT_LIGHTGREY);
    frameCanvas_.drawString(detail, centerX, messageY + kMessageLineGapPx);
  }
  frameCanvas_.pushSprite(&M5.Display, 0, 0);
}

void PlayerView::drawStatusBadge(const char* label) {
  frameCanvas_.setFont(&fonts::efontJA_16);
  const int32_t badgeWidth = frameCanvas_.textWidth(label) + kBadgePaddingPx * 2;
  const int32_t badgeLeft = (frameCanvas_.width() - badgeWidth) / 2;
  frameCanvas_.fillRoundRect(badgeLeft, kBadgeTopPx, badgeWidth, kBadgeHeightPx, kBadgeHeightPx / 2, kBadgeColor);
  frameCanvas_.setTextDatum(datum_t::middle_center);
  frameCanvas_.setTextColor(TFT_WHITE);
  frameCanvas_.drawString(label, frameCanvas_.width() / 2, kBadgeTopPx + kBadgeHeightPx / 2);
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
  const auto measureWidth = [this](const std::string& text) { return frameCanvas_.textWidth(text.c_str()); };
  frameCanvas_.setTextDatum(datum_t::middle_center);
  frameCanvas_.setFont(&fonts::efontJA_24);
  frameCanvas_.setTextColor(TFT_WHITE);
  const std::string fittedTitle = fitTextToWidth(normalizeForDisplay(playback.title), kTitleMaxWidthPx, measureWidth);
  frameCanvas_.drawString(fittedTitle.c_str(), centerX, centerY + kTitleCenterOffsetPx);
  frameCanvas_.setFont(&fonts::efontJA_16);
  frameCanvas_.setTextColor(TFT_LIGHTGREY);
  const std::string fittedArtists = fitTextToWidth(normalizeForDisplay(playback.artists), kArtistMaxWidthPx, measureWidth);
  frameCanvas_.drawString(fittedArtists.c_str(), centerX, centerY + kArtistCenterOffsetPx);
}
