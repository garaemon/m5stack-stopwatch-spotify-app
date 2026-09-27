#pragma once

#include <optional>
#include <string>

#include "playback_state.h"

struct AccessToken {
  std::string accessToken;
  // Spotify may rotate the refresh token; empty when it did not.
  std::string refreshToken;
  uint32_t expiresInSeconds = 0;
};

// Returns the artwork edge length that the round 466x466 display needs.
constexpr int kTargetArtworkSizePx = 466;

// Parses the body of GET /me/player/currently-playing.
// Returns std::nullopt when the body is not valid JSON.
// Returns a state with hasTrack == false for podcasts, ads, or an empty body.
std::optional<PlaybackState> parseCurrentlyPlaying(const std::string& json);

// Parses the body of GET /me/library/contains for a single URI.
std::optional<bool> parseLibraryContains(const std::string& json);

// Parses the body of POST https://accounts.spotify.com/api/token.
std::optional<AccessToken> parseTokenResponse(const std::string& json);
