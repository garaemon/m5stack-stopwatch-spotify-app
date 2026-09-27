#include "spotify_client.h"

#include <utility>

SpotifyClient::SpotifyClient(std::string clientId, std::string refreshToken)
    : clientId_(std::move(clientId)), refreshToken_(std::move(refreshToken)) {}

HttpResult SpotifyClient::fetchCurrentlyPlaying() { return sendApiRequest("GET", "/v1/me/player/currently-playing"); }

HttpResult SpotifyClient::fetchLibraryContains(const std::string& trackUri) {
  return sendApiRequest("GET", "/v1/me/library/contains?uris=" + trackUri);
}

HttpResult SpotifyClient::saveToLibrary(const std::string& trackUri) {
  return sendApiRequest("PUT", "/v1/me/library?uris=" + trackUri);
}

HttpResult SpotifyClient::removeFromLibrary(const std::string& trackUri) {
  return sendApiRequest("DELETE", "/v1/me/library?uris=" + trackUri);
}

HttpResult SpotifyClient::skipToNext() { return sendApiRequest("POST", "/v1/me/player/next"); }

HttpResult SpotifyClient::skipToPrevious() { return sendApiRequest("POST", "/v1/me/player/previous"); }

std::string SpotifyClient::downloadImage(const std::string& url) {
  (void)url;
  return {};
}

bool SpotifyClient::refreshAccessToken() { return false; }

HttpResult SpotifyClient::sendApiRequest(const char* method, const std::string& path) {
  (void)method;
  (void)path;
  return {-1, {}};
}
