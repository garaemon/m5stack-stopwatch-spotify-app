// Arduino entry point. The loop task owns input, rendering, and the
// AppController; every Spotify request runs on the NetworkWorker task.
// Data flows in one cycle: input or a NetworkResult goes into AppController,
// which returns Effects; applyEffect() runs kRender and kVibrate here and
// forwards the rest to NetworkWorker, whose results come back through
// tryPopResult() on the next loop iteration.
#include <M5Unified.h>
#include <WiFi.h>

#include <optional>
#include <vector>

#include "app_controller.h"
#include "display_power_policy.h"
#include "double_tap_detector.h"
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
// Buttons wait this long after a release before deciding single vs double
// click; M5Unified's 500 ms default made play/pause feel sluggish.
constexpr uint32_t kButtonClickDecisionMs = 350;
constexpr uint32_t kVibrationDurationMs = 60;
constexpr uint32_t kRestartDelayAfterFatalErrorMs = 10000;

AppController controller;
PlayerView view;
SpotifyClient spotify(SPOTIFY_CLIENT_ID, SPOTIFY_REFRESH_TOKEN);
NetworkWorker networkWorker(spotify);
uint32_t playbackReceivedMs = 0;
uint32_t lastRenderMs = 0;
std::optional<uint32_t> vibrationStopMs;
DisplayPowerPolicy powerPolicy;
DoubleTapDetector doubleTapDetector;
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

std::optional<UserCommand> readUserCommand(bool isTapped) {
  if (M5.BtnA.wasSingleClicked()) {
    return UserCommand::kRestartTrack;
  }
  if (M5.BtnA.wasDoubleClicked()) {
    return UserCommand::kPrevious;
  }
  if (M5.BtnB.wasSingleClicked()) {
    return UserCommand::kTogglePlayback;
  }
  if (M5.BtnB.wasDoubleClicked()) {
    return UserCommand::kNext;
  }
  if (isTapped && doubleTapDetector.registerTap(millis())) {
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
      // The worker may finish a request for the previous track after a poll
      // already reported the next one.
      // TODO(garaemon): Retry a failed download; the artwork stays black until
      // the track changes.
      if (result.artworkUrl == controller.playback().artworkUrl) {
        view.setArtwork(result.artwork);
        renderPlayer();
      }
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
  const bool isWakingUp = previousPower == DisplayPower::kOff;
  if (isWakingUp) {
    M5.Display.wakeup();
    networkWorker.setPollIntervalMs(kActivePollIntervalMs);
    // The state kept while the display was off may be 10 s old.
    networkWorker.requestImmediatePoll();
  }
  M5.Display.setBrightness(power == DisplayPower::kDimmed ? kDimmedBrightness : normalBrightness);
  // Render after the brightness command: the panel's QSPI bus may cut off a
  // frame that is still being transferred when another command follows it.
  if (isWakingUp) {
    renderPlayer();
  }
}

void handleUserInput() {
  const bool isTapped = M5.Touch.getDetail().wasClicked();
  const bool hasInput = isTapped || M5.BtnA.wasDecideClickCount() || M5.BtnB.wasDecideClickCount();
  if (!hasInput) {
    return;
  }
  // Any input on a dark display only wakes it, even a lone tap that would
  // otherwise do nothing.
  if (powerPolicy.registerInteraction(millis())) {
    applyDisplayPower(DisplayPower::kOn);
    return;
  }
  const std::optional<UserCommand> command = readUserCommand(isTapped);
  if (command.has_value()) {
    applyEffects(controller.handleCommand(*command));
  }
}

}  // namespace

void setup() {
  M5.begin(M5.config());
  normalBrightness = M5.Display.getBrightness();
  M5.BtnA.setHoldThresh(kButtonClickDecisionMs);
  M5.BtnB.setHoldThresh(kButtonClickDecisionMs);
  view.begin();
  startWifi();
  spotify.begin();
  if (!networkWorker.start()) {
    view.showMessage("Out of memory", "Could not start networking");
    // Restarting is the only recovery; staying up would let the idle display
    // power policy overwrite the message.
    delay(kRestartDelayAfterFatalErrorMs);
    ESP.restart();
  }
  renderPlayer();
}

void loop() {
  M5.update();
  stopVibrationWhenDue();
  handleUserInput();
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
