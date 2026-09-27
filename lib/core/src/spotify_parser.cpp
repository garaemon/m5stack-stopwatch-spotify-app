#include "spotify_parser.h"

std::optional<PlaybackState> parseCurrentlyPlaying(const std::string& json) {
  (void)json;
  return std::nullopt;
}

std::optional<bool> parseLibraryContains(const std::string& json) {
  (void)json;
  return std::nullopt;
}

std::optional<AccessToken> parseTokenResponse(const std::string& json) {
  (void)json;
  return std::nullopt;
}
