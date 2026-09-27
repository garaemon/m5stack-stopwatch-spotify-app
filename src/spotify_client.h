#pragma once

#include <optional>
#include <string>

struct HttpResult {
  int statusCode = 0;  // Negative on transport errors.
  std::string body;
};

// Calls the Spotify Web API over HTTPS and refreshes the access token on demand.
// Blocks the caller for the duration of each request.
class SpotifyClient {
 public:
  SpotifyClient(std::string clientId, std::string refreshToken);

  HttpResult fetchCurrentlyPlaying();
  HttpResult fetchLibraryContains(const std::string& trackUri);
  HttpResult saveToLibrary(const std::string& trackUri);
  HttpResult removeFromLibrary(const std::string& trackUri);
  HttpResult skipToNext();
  HttpResult skipToPrevious();

  // Returns the JPEG bytes at an i.scdn.co URL, or an empty string on failure.
  std::string downloadImage(const std::string& url);

 private:
  bool refreshAccessToken();
  HttpResult sendApiRequest(const char* method, const std::string& path);

  std::string clientId_;
  std::string refreshToken_;
  std::string accessToken_;
  uint32_t accessTokenExpiryMs_ = 0;
};
