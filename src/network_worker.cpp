#include "network_worker.h"

#include <Arduino.h>
#include <WiFi.h>

#include "spotify_parser.h"

namespace {

// Spotify keeps reporting the old state for a moment after a player command.
constexpr uint32_t kPollDelayAfterPlayerCommandMs = 300;
constexpr uint32_t kIdleDelayMs = 20;
// Used when a 429 response lacks a Retry-After header.
constexpr uint32_t kDefaultRetryAfterSeconds = 30;
// Leaves time for the loop task to request the current artwork first, so
// that prefetching the next one never delays it.
constexpr uint32_t kPrefetchDelayMs = 500;

// TLS handshakes in mbedTLS and JPEG decoding need a deep stack.
constexpr uint32_t kTaskStackBytes = 20480;
constexpr UBaseType_t kTaskPriority = 1;
// Core 0 also runs the Wi-Fi stack; the Arduino loop owns core 1.
constexpr BaseType_t kTaskCore = 0;

}  // namespace

bool NetworkWorker::start() {
  const BaseType_t created =
      xTaskCreatePinnedToCore(runTask, "network", kTaskStackBytes, this, kTaskPriority, nullptr, kTaskCore);
  if (created != pdPASS) {
    log_e("failed to create the network task");
    return false;
  }
  return true;
}

void NetworkWorker::runTask(void* worker) { static_cast<NetworkWorker*>(worker)->runLoop(); }

void NetworkWorker::runLoop() {
  while (true) {
    const bool isWifiConnected = WiFi.status() == WL_CONNECTED;
    // Deferred effects go first so that they run in the order they were issued.
    if (isWifiConnected) {
      std::vector<Effect> deferredEffects;
      deferredEffects.swap(deferredEffects_);
      for (const Effect& deferredEffect : deferredEffects) {
        handleRequest(deferredEffect);
      }
    }
    // Player commands made while offline are dropped, because replaying stale
    // skips after reconnecting would surprise the user. Fetches and like edits
    // wait instead.
    while (const std::optional<Effect> effect = requests_.tryPop()) {
      if (isWifiConnected) {
        handleRequest(*effect);
      } else if (shouldSurviveOfflineGap(effect->type)) {
        deferredEffects_.push_back(*effect);
      }
    }
    if (isImmediatePollRequested_.exchange(false)) {
      nextPollMs_ = millis();
    }
    if (rateLimitedUntilMs_.has_value() && static_cast<int32_t>(millis() - *rateLimitedUntilMs_) >= 0) {
      rateLimitedUntilMs_.reset();
    }
    // A 429 deadline outranks the player-command and wake-up shortcuts that
    // move nextPollMs_ earlier.
    const bool isRateLimited = rateLimitedUntilMs_.has_value();
    if (!isRateLimited && static_cast<int32_t>(millis() - nextPollMs_) >= 0) {
      nextPollMs_ = millis() + pollIntervalMs_;
      hasEverConnectedWifi_ = hasEverConnectedWifi_ || isWifiConnected;
      if (isWifiConnected) {
        pollPlayback();
      } else {
        // The first association after boot takes a few seconds; that is
        // not a disconnection worth alarming the user about.
        reportConnectionStatus(hasEverConnectedWifi_ ? ConnectionStatus::kWifiDisconnected
                                                     : ConnectionStatus::kConnecting);
      }
    }
    const bool isPrefetchDue = prefetchDueMs_.has_value() && static_cast<int32_t>(millis() - *prefetchDueMs_) >= 0;
    if (isWifiConnected && isPrefetchDue) {
      prefetchDueMs_.reset();
      prefetchNextArtwork();
    }
    vTaskDelay(pdMS_TO_TICKS(kIdleDelayMs));
  }
}

void NetworkWorker::handleRequest(const Effect& effect) {
  switch (effect.type) {
    case EffectType::kFetchArtwork: {
      NetworkResult result{NetworkResultType::kArtwork};
      result.artworkUrl = effect.argument;
      result.artwork = loadArtwork(effect.argument);
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
      nextPollMs_ = millis() + kPollDelayAfterPlayerCommandMs;
      break;
    case EffectType::kSkipPrevious:
      spotify_.skipToPrevious();
      nextPollMs_ = millis() + kPollDelayAfterPlayerCommandMs;
      break;
    case EffectType::kSeekToStart:
      spotify_.seekToStart();
      nextPollMs_ = millis() + kPollDelayAfterPlayerCommandMs;
      break;
    case EffectType::kPause:
      spotify_.pausePlayback();
      nextPollMs_ = millis() + kPollDelayAfterPlayerCommandMs;
      break;
    case EffectType::kResume:
      spotify_.resumePlayback();
      nextPollMs_ = millis() + kPollDelayAfterPlayerCommandMs;
      break;
    case EffectType::kVibrate:
    case EffectType::kRender:
      break;
  }
}

void NetworkWorker::pollPlayback() {
  const HttpResult response = spotify_.fetchCurrentlyPlaying();
  log_i("currently-playing status=%d", response.statusCode);
  const ConnectionStatus status = classifyPollStatusCode(response.statusCode);
  reportConnectionStatus(status);
  if (status == ConnectionStatus::kRateLimited) {
    const uint32_t retryAfterSeconds =
        response.retryAfterSeconds > 0 ? response.retryAfterSeconds : kDefaultRetryAfterSeconds;
    rateLimitedUntilMs_ = millis() + retryAfterSeconds * 1000;
    nextPollMs_ = *rateLimitedUntilMs_;
  }
  if (status != ConnectionStatus::kOk) {
    return;
  }
  const std::optional<PlaybackState> playback = parseCurrentlyPlaying(response.body);
  if (!playback.has_value()) {
    return;
  }
  if (playback->hasTrack && playback->trackUri != lastPolledTrackUri_) {
    lastPolledTrackUri_ = playback->trackUri;
    prefetchDueMs_ = millis() + kPrefetchDelayMs;
  }
  NetworkResult result{NetworkResultType::kPlayback};
  result.playback = *playback;
  results_.push(std::move(result));
}

ArtworkImage NetworkWorker::loadArtwork(const std::string& url) {
  if (const std::optional<ArtworkImage> cachedArtwork = artworkCache_.get(url)) {
    return *cachedArtwork;
  }
  const uint32_t startMs = millis();
  ArtworkImage artwork = decodeArtwork(spotify_.downloadImage(url));
  log_i("artwork loaded in %lu ms", static_cast<unsigned long>(millis() - startMs));
  if (artwork != nullptr) {
    artworkCache_.put(url, artwork);
  }
  return artwork;
}

void NetworkWorker::prefetchNextArtwork() {
  const HttpResult response = spotify_.fetchQueue();
  if (!response.isOk()) {
    return;
  }
  const std::optional<std::string> nextArtworkUrl = parseNextQueuedArtworkUrl(response.body);
  if (nextArtworkUrl.has_value() && !artworkCache_.contains(*nextArtworkUrl)) {
    log_i("prefetching next artwork");
    loadArtwork(*nextArtworkUrl);
  }
}

void NetworkWorker::fetchLikeStatus(const std::string& trackUri) {
  const HttpResult response = spotify_.fetchLibraryContains(trackUri);
  const std::optional<bool> isLiked = parseLibraryContains(response.body);
  // TODO(garaemon): Retry a failed lookup; the heart stays hidden until the
  // track changes.
  if (!response.isOk() || !isLiked.has_value()) {
    return;
  }
  NetworkResult result{NetworkResultType::kLikeStatus};
  result.trackUri = trackUri;
  result.isLiked = *isLiked;
  results_.push(std::move(result));
}

void NetworkWorker::reportConnectionStatus(ConnectionStatus status) {
  NetworkResult result{NetworkResultType::kConnectionStatus};
  result.connectionStatus = status;
  results_.push(std::move(result));
}
