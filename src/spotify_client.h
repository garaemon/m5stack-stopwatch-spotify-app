#pragma once

#include <cstdint>
#include <string>

struct HttpResult {
  int statusCode = 0;  // Negative on transport errors.
  std::string body;
  uint32_t retryAfterSeconds = 0;  // From the Retry-After header of a 429.
};

// Calls the Spotify Web API over HTTPS and refreshes the access token on demand.
// Blocks the caller for the duration of each request.
class SpotifyClient {
 public:
  SpotifyClient(std::string clientId, std::string refreshToken);

  // Loads the latest rotated refresh token from NVS. Call after the flash
  // file system is ready and before any request.
  void begin();

  HttpResult fetchCurrentlyPlaying();
  HttpResult fetchQueue();
  HttpResult fetchLibraryContains(const std::string& trackUri);
  HttpResult saveToLibrary(const std::string& trackUri);
  HttpResult removeFromLibrary(const std::string& trackUri);
  HttpResult skipToNext();
  HttpResult skipToPrevious();
  HttpResult seekToStart();
  HttpResult pausePlayback();
  HttpResult resumePlayback();

  // Returns the JPEG bytes at an i.scdn.co URL, or an empty string on failure.
  std::string downloadImage(const std::string& url);

 private:
  bool refreshAccessToken();
  bool ensureAccessToken();
  HttpResult sendApiRequest(const char* method, const std::string& path);
  HttpResult sendAuthorizedRequest(const char* method, const std::string& path);

  std::string clientId_;
  std::string refreshToken_;
  std::string accessToken_;
  uint32_t accessTokenExpiryMs_ = 0;
};
