#include <M5Unified.h>
#include <WiFi.h>

#include <optional>
#include <vector>

#include "app_controller.h"
#include "network_worker.h"
#include "player_view.h"
#include "progress_estimator.h"
#include "secrets.h"
#include "spotify_client.h"

namespace {

// 250 ms moves the progress ring about 2 px on a 3-minute track.
constexpr uint32_t kProgressRenderIntervalMs = 250;
constexpr uint32_t kWifiTimeoutMs = 20000;
constexpr uint8_t kVibrationLevel = 128;
constexpr uint32_t kVibrationDurationMs = 60;

AppController controller;
PlayerView view;
SpotifyClient spotify(SPOTIFY_CLIENT_ID, SPOTIFY_REFRESH_TOKEN);
NetworkWorker networkWorker(spotify);
uint32_t playbackReceivedMs = 0;
uint32_t lastRenderMs = 0;
std::optional<uint32_t> vibrationStopMs;

bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startMs > kWifiTimeoutMs) {
      return false;
    }
    delay(200);
  }
  return true;
}

void startVibration() {
  M5.Power.setVibration(kVibrationLevel);
  vibrationStopMs = millis() + kVibrationDurationMs;
}

void stopVibrationWhenDue() {
  if (vibrationStopMs.has_value() && static_cast<int32_t>(millis() - *vibrationStopMs) >= 0) {
    M5.Power.setVibration(0);
    vibrationStopMs.reset();
  }
}

void applyEffects(const std::vector<Effect>& effects);

void renderPlayer() {
  PlaybackState displayedPlayback = controller.playback();
  displayedPlayback.progressMs = estimateProgressMs(displayedPlayback, millis() - playbackReceivedMs);
  view.render(displayedPlayback, controller.likeStatus());
  lastRenderMs = millis();
}

void applyEffect(const Effect& effect) {
  switch (effect.type) {
    case EffectType::kVibrate:
      startVibration();
      break;
    case EffectType::kRender:
      renderPlayer();
      break;
    default:
      networkWorker.request(effect);
      break;
  }
}

void applyEffects(const std::vector<Effect>& effects) {
  for (const Effect& effect : effects) {
    applyEffect(effect);
  }
}

std::optional<UserCommand> readUserCommand() {
  if (M5.BtnA.wasClicked()) {
    return UserCommand::kPrevious;
  }
  if (M5.BtnB.wasClicked()) {
    return UserCommand::kNext;
  }
  if (M5.Touch.getDetail().wasClicked()) {
    return UserCommand::kToggleLike;
  }
  return std::nullopt;
}

void applyNetworkResult(const NetworkResult& result) {
  switch (result.type) {
    case NetworkResultType::kPlayback:
      playbackReceivedMs = millis();
      applyEffects(controller.handlePlayback(result.playback));
      break;
    case NetworkResultType::kLikeStatus:
      applyEffects(controller.handleLikeStatus(result.trackUri, result.isLiked));
      break;
    case NetworkResultType::kArtwork:
      view.setArtwork(result.artworkJpeg);
      renderPlayer();
      break;
  }
}

}  // namespace

void setup() {
  M5.begin(M5.config());
  view.begin();
  view.showMessage("Connecting Wi-Fi...");
  if (!connectWifi()) {
    view.showMessage("Wi-Fi failed");
    return;
  }
  spotify.begin();
  networkWorker.start();
  view.showMessage("Waiting for Spotify...");
}

void loop() {
  M5.update();
  stopVibrationWhenDue();
  const std::optional<UserCommand> command = readUserCommand();
  if (command.has_value()) {
    applyEffects(controller.handleCommand(*command));
  }
  while (const std::optional<NetworkResult> result = networkWorker.tryPopResult()) {
    applyNetworkResult(*result);
  }
  const bool isPlaying = controller.playback().hasTrack && controller.playback().isPlaying;
  if (isPlaying && millis() - lastRenderMs >= kProgressRenderIntervalMs) {
    renderPlayer();
  }
  delay(10);
}
