#include "spotify_parser.h"

#include <ArduinoJson.h>

namespace {

std::string joinArtistNames(JsonArrayConst artists) {
  std::string joinedNames;
  for (JsonObjectConst artist : artists) {
    if (!joinedNames.empty()) {
      joinedNames += ", ";
    }
    joinedNames += artist["name"] | "";
  }
  return joinedNames;
}

// Prefers the smallest image that still fills the display so that downloads
// stay small; falls back to the largest image when every image is smaller.
std::string selectArtworkUrl(JsonArrayConst images) {
  const char* coveringUrl = nullptr;
  int coveringWidth = 0;
  const char* largestUrl = nullptr;
  int largestWidth = 0;
  for (JsonObjectConst image : images) {
    const int width = image["width"] | 0;
    const char* url = image["url"] | "";
    if (width >= kTargetArtworkSizePx && (coveringUrl == nullptr || width < coveringWidth)) {
      coveringUrl = url;
      coveringWidth = width;
    }
    if (largestUrl == nullptr || width > largestWidth) {
      largestUrl = url;
      largestWidth = width;
    }
  }
  if (coveringUrl != nullptr) {
    return coveringUrl;
  }
  return largestUrl != nullptr ? largestUrl : "";
}

}  // namespace

std::optional<PlaybackState> parseCurrentlyPlaying(const std::string& json) {
  // Spotify answers 204 No Content when no device is playing.
  if (json.empty()) {
    return PlaybackState{};
  }
  JsonDocument document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return std::nullopt;
  }
  PlaybackState playback;
  playback.isPlaying = document["is_playing"] | false;
  playback.progressMs = document["progress_ms"] | 0u;
  JsonObjectConst item = document["item"];
  const std::string playingType = document["currently_playing_type"] | "";
  if (playingType != "track" || item.isNull()) {
    return playback;
  }
  playback.hasTrack = true;
  playback.trackUri = item["uri"] | "";
  playback.title = item["name"] | "";
  playback.durationMs = item["duration_ms"] | 0u;
  playback.artists = joinArtistNames(item["artists"]);
  playback.artworkUrl = selectArtworkUrl(item["album"]["images"]);
  return playback;
}

std::optional<std::string> parseNextQueuedArtworkUrl(const std::string& json) {
  // The queue lists up to 20 full track objects; keep only what we read.
  JsonDocument filter;
  filter["queue"][0]["album"]["images"] = true;
  JsonDocument document;
  if (deserializeJson(document, json, DeserializationOption::Filter(filter)) != DeserializationError::Ok) {
    return std::nullopt;
  }
  JsonArrayConst images = document["queue"][0]["album"]["images"];
  if (images.isNull() || images.size() == 0) {
    return std::nullopt;
  }
  return selectArtworkUrl(images);
}

std::optional<bool> parseLibraryContains(const std::string& json) {
  JsonDocument document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return std::nullopt;
  }
  JsonArrayConst containsFlags = document.as<JsonArrayConst>();
  if (containsFlags.size() == 0) {
    return std::nullopt;
  }
  return containsFlags[0].as<bool>();
}

std::optional<AccessToken> parseTokenResponse(const std::string& json) {
  JsonDocument document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return std::nullopt;
  }
  const char* accessToken = document["access_token"];
  if (accessToken == nullptr) {
    return std::nullopt;
  }
  AccessToken token;
  token.accessToken = accessToken;
  token.refreshToken = document["refresh_token"] | "";
  token.expiresInSeconds = document["expires_in"] | 0u;
  return token;
}
