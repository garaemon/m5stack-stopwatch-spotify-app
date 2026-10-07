#pragma once

#include <cstdint>
#include <string>

inline constexpr int kHttpStatusOk = 200;

struct HttpResult {
  int statusCode = 0;  // Negative on transport errors.
  std::string body;
  uint32_t retryAfterSeconds = 0;  // From the Retry-After header of a 429.

  bool isOk() const { return statusCode == kHttpStatusOk; }
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
  // Returns the status of the token endpoint: 200 on success, 400 or 401 when
  // Spotify rejected the refresh token, negative on transport errors.
  int refreshAccessToken();
  int ensureAccessToken();
  HttpResult sendApiRequest(const char* method, const std::string& path);
  HttpResult sendAuthorizedRequest(const char* method, const std::string& path);

  std::string clientId_;
  std::string refreshToken_;
  std::string accessToken_;
  uint32_t accessTokenExpiryMs_ = 0;
};
