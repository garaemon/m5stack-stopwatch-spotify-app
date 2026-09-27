#include "network_worker.h"

#include <Arduino.h>
#include <WiFi.h>

#include "spotify_parser.h"

namespace {

constexpr uint32_t kPollIntervalMs = 3000;
// Spotify keeps reporting the old track for a moment after a skip.
constexpr uint32_t kPollDelayAfterSkipMs = 300;
constexpr uint32_t kIdleDelayMs = 20;
// TLS handshakes in mbedTLS need a deep stack.
constexpr uint32_t kTaskStackBytes = 16384;
constexpr UBaseType_t kTaskPriority = 1;
// Core 0 also runs the Wi-Fi stack; the Arduino loop owns core 1.
constexpr BaseType_t kTaskCore = 0;

}  // namespace

void NetworkWorker::start() {
  xTaskCreatePinnedToCore(runTask, "network", kTaskStackBytes, this, kTaskPriority, nullptr, kTaskCore);
}

void NetworkWorker::runTask(void* worker) { static_cast<NetworkWorker*>(worker)->runLoop(); }

void NetworkWorker::runLoop() {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      while (const std::optional<Effect> effect = requests_.tryPop()) {
        handleRequest(*effect);
      }
      if (static_cast<int32_t>(millis() - nextPollMs_) >= 0) {
        nextPollMs_ = millis() + kPollIntervalMs;
        pollPlayback();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
  }
}

void NetworkWorker::handleRequest(const Effect& effect) {
  switch (effect.type) {
    case EffectType::kFetchArtwork: {
      NetworkResult result{NetworkResultType::kArtwork};
      result.artworkJpeg = spotify_.downloadImage(effect.argument);
      results_.push(std::move(result));
      break;
    }
    case EffectType::kFetchLikeStatus:
      fetchLikeStatus(effect.argument);
      break;
    case EffectType::kSaveTrack:
      spotify_.saveToLibrary(effect.argument);
      break;
    case EffectType::kRemoveTrack:
      spotify_.removeFromLibrary(effect.argument);
      break;
    case EffectType::kSkipNext:
      spotify_.skipToNext();
      nextPollMs_ = millis() + kPollDelayAfterSkipMs;
      break;
    case EffectType::kSkipPrevious:
      spotify_.skipToPrevious();
      nextPollMs_ = millis() + kPollDelayAfterSkipMs;
      break;
    case EffectType::kVibrate:
    case EffectType::kRender:
      break;
  }
}

void NetworkWorker::pollPlayback() {
  const HttpResult response = spotify_.fetchCurrentlyPlaying();
  log_i("currently-playing status=%d", response.statusCode);
  if (response.statusCode != 200 && response.statusCode != 204) {
    return;
  }
  const std::optional<PlaybackState> playback = parseCurrentlyPlaying(response.body);
  if (playback.has_value()) {
    NetworkResult result{NetworkResultType::kPlayback};
    result.playback = *playback;
    results_.push(std::move(result));
  }
}

void NetworkWorker::fetchLikeStatus(const std::string& trackUri) {
  const HttpResult response = spotify_.fetchLibraryContains(trackUri);
  const std::optional<bool> isLiked = parseLibraryContains(response.body);
  if (response.statusCode != 200 || !isLiked.has_value()) {
    return;
  }
  NetworkResult result{NetworkResultType::kLikeStatus};
  result.trackUri = trackUri;
  result.isLiked = *isLiked;
  results_.push(std::move(result));
}
