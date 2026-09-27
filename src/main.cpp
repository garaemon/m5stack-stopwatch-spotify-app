#include <M5Unified.h>
#include <WiFi.h>

#include <optional>
#include <vector>

#include "app_controller.h"
#include "display_power_policy.h"
#include "network_worker.h"
#include "player_view.h"
#include "progress_estimator.h"
#include "secrets.h"
#include "spotify_client.h"

namespace {

// 250 ms moves the progress ring about 2 px on a 3-minute track.
constexpr uint32_t kProgressRenderIntervalMs = 250;
constexpr uint32_t kActivePollIntervalMs = 3000;
// Slow enough to save power, fast enough to wake soon after playback starts.
constexpr uint32_t kDisplayOffPollIntervalMs = 10000;
constexpr uint8_t kDimmedBrightness = 30;
constexpr uint8_t kVibrationLevel = 128;
constexpr uint32_t kVibrationDurationMs = 60;

AppController controller;
PlayerView view;
SpotifyClient spotify(SPOTIFY_CLIENT_ID, SPOTIFY_REFRESH_TOKEN);
NetworkWorker networkWorker(spotify);
uint32_t playbackReceivedMs = 0;
uint32_t lastRenderMs = 0;
std::optional<uint32_t> vibrationStopMs;
DisplayPowerPolicy powerPolicy;
DisplayPower appliedDisplayPower = DisplayPower::kOn;
uint8_t normalBrightness = 0;

void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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
  if (appliedDisplayPower == DisplayPower::kOff) {
    return;
  }
  PlaybackState displayedPlayback = controller.playback();
  displayedPlayback.progressMs = estimateProgressMs(displayedPlayback, millis() - playbackReceivedMs);
  view.render(displayedPlayback, controller.likeStatus(), controller.connectionStatus());
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
    case NetworkResultType::kConnectionStatus:
      applyEffects(controller.handleConnectionStatus(result.connectionStatus));
      break;
  }
}

void applyDisplayPower(DisplayPower power) {
  if (power == appliedDisplayPower) {
    return;
  }
  const DisplayPower previousPower = appliedDisplayPower;
  appliedDisplayPower = power;
  if (power == DisplayPower::kOff) {
    M5.Display.sleep();
    networkWorker.setPollIntervalMs(kDisplayOffPollIntervalMs);
    return;
  }
  if (previousPower == DisplayPower::kOff) {
    M5.Display.wakeup();
    networkWorker.setPollIntervalMs(kActivePollIntervalMs);
    renderPlayer();
  }
  M5.Display.setBrightness(power == DisplayPower::kDimmed ? kDimmedBrightness : normalBrightness);
}

void handleUserCommand(UserCommand command) {
  const bool wasDisplayOff = powerPolicy.registerInteraction(millis());
  if (wasDisplayOff) {
    applyDisplayPower(DisplayPower::kOn);
    return;
  }
  applyEffects(controller.handleCommand(command));
}

}  // namespace

void setup() {
  M5.begin(M5.config());
  normalBrightness = M5.Display.getBrightness();
  view.begin();
  startWifi();
  spotify.begin();
  networkWorker.start();
  renderPlayer();
}

void loop() {
  M5.update();
  stopVibrationWhenDue();
  const std::optional<UserCommand> command = readUserCommand();
  if (command.has_value()) {
    handleUserCommand(*command);
  }
  while (const std::optional<NetworkResult> result = networkWorker.tryPopResult()) {
    applyNetworkResult(*result);
  }
  const bool isPlaying = controller.playback().hasTrack && controller.playback().isPlaying;
  applyDisplayPower(powerPolicy.update(millis(), isPlaying));
  if (isPlaying && millis() - lastRenderMs >= kProgressRenderIntervalMs) {
    renderPlayer();
  }
  delay(10);
}
