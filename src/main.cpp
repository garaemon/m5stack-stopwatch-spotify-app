#include <M5Unified.h>
#include <WiFi.h>

#include <optional>
#include <vector>

#include "app_controller.h"
#include "player_view.h"
#include "secrets.h"
#include "spotify_client.h"
#include "spotify_parser.h"

namespace {

constexpr uint32_t kPollIntervalMs = 3000;
constexpr uint32_t kWifiTimeoutMs = 20000;
constexpr uint8_t kVibrationLevel = 128;
constexpr uint32_t kVibrationDurationMs = 60;

AppController controller;
PlayerView view;
SpotifyClient spotify(SPOTIFY_CLIENT_ID, SPOTIFY_REFRESH_TOKEN);
uint32_t lastPollMs = 0;

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

void pulseVibration() {
  M5.Power.setVibration(kVibrationLevel);
  delay(kVibrationDurationMs);
  M5.Power.setVibration(0);
}

void applyEffects(const std::vector<Effect>& effects);

void fetchLikeStatus(const std::string& trackUri) {
  const HttpResult result = spotify.fetchLibraryContains(trackUri);
  const std::optional<bool> isLiked = parseLibraryContains(result.body);
  if (result.statusCode == 200 && isLiked.has_value()) {
    applyEffects(controller.handleLikeStatus(trackUri, *isLiked));
  }
}

void applyEffect(const Effect& effect) {
  switch (effect.type) {
    case EffectType::kFetchArtwork:
      view.setArtwork(spotify.downloadImage(effect.argument));
      break;
    case EffectType::kFetchLikeStatus:
      fetchLikeStatus(effect.argument);
      break;
    case EffectType::kSaveTrack:
      spotify.saveToLibrary(effect.argument);
      break;
    case EffectType::kRemoveTrack:
      spotify.removeFromLibrary(effect.argument);
      break;
    case EffectType::kSkipNext:
      spotify.skipToNext();
      lastPollMs = 0;  // Poll right away so that the new track shows up quickly.
      break;
    case EffectType::kSkipPrevious:
      spotify.skipToPrevious();
      lastPollMs = 0;
      break;
    case EffectType::kVibrate:
      pulseVibration();
      break;
    case EffectType::kRender:
      view.render(controller.playback(), controller.likeStatus());
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

void pollPlayback() {
  const HttpResult result = spotify.fetchCurrentlyPlaying();
  Serial.printf("currently-playing status=%d\n", result.statusCode);
  if (result.statusCode != 200 && result.statusCode != 204) {
    return;
  }
  const std::optional<PlaybackState> playback = parseCurrentlyPlaying(result.body);
  if (playback.has_value()) {
    applyEffects(controller.handlePlayback(*playback));
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
  view.showMessage("Waiting for Spotify...");
}

void loop() {
  M5.update();
  const std::optional<UserCommand> command = readUserCommand();
  if (command.has_value()) {
    applyEffects(controller.handleCommand(*command));
  }
  // TODO(garaemon): Move HTTP calls to a separate FreeRTOS task; each
  // request blocks button handling for a few hundred milliseconds.
  if (WiFi.status() == WL_CONNECTED && millis() - lastPollMs >= kPollIntervalMs) {
    lastPollMs = millis();
    pollPlayback();
  }
  delay(10);
}
