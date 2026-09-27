#include "spotify_client.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Preferences.h>

#include <memory>
#include <utility>

#include "spotify_parser.h"

namespace {

constexpr const char* kApiBaseUrl = "https://api.spotify.com";
constexpr const char* kTokenUrl = "https://accounts.spotify.com/api/token";
constexpr const char* kPreferencesNamespace = "spotify";
constexpr const char* kSeedTokenKey = "seed";
constexpr const char* kRotatedTokenKey = "refresh";
// Refresh a minute early so that a request never races the expiry.
constexpr uint32_t kTokenExpiryMarginMs = 60 * 1000;
constexpr uint16_t kHttpTimeoutMs = 10000;
constexpr int kHttpUnauthorized = 401;

std::unique_ptr<NetworkClientSecure> createTlsClient() {
  auto tlsClient = std::make_unique<NetworkClientSecure>();
  tlsClient->useBuiltinCACertBundle();
  return tlsClient;
}

std::string readBody(HTTPClient& http) {
  const String body = http.getString();
  return std::string(body.c_str(), body.length());
}

// Returns the refresh token that Spotify rotated most recently. NVS keeps it
// across reboots, and a newly flashed secrets.h token discards it.
std::string loadRefreshToken(const std::string& compiledToken) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  const String seedToken = preferences.getString(kSeedTokenKey, "");
  if (seedToken != compiledToken.c_str()) {
    preferences.putString(kSeedTokenKey, compiledToken.c_str());
    preferences.remove(kRotatedTokenKey);
  }
  const String rotatedToken = preferences.getString(kRotatedTokenKey, "");
  preferences.end();
  return rotatedToken.isEmpty() ? compiledToken : std::string(rotatedToken.c_str());
}

void storeRotatedRefreshToken(const std::string& refreshToken) {
  Preferences preferences;
  preferences.begin(kPreferencesNamespace, false);
  preferences.putString(kRotatedTokenKey, refreshToken.c_str());
  preferences.end();
}

}  // namespace

SpotifyClient::SpotifyClient(std::string clientId, std::string refreshToken)
    : clientId_(std::move(clientId)), refreshToken_(std::move(refreshToken)) {}

void SpotifyClient::begin() { refreshToken_ = loadRefreshToken(refreshToken_); }

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
  const auto tlsClient = createTlsClient();
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  if (!http.begin(*tlsClient, url.c_str())) {
    return {};
  }
  const int statusCode = http.GET();
  const std::string jpegBytes = statusCode == HTTP_CODE_OK ? readBody(http) : std::string();
  http.end();
  log_i("artwork status=%d bytes=%u", statusCode, static_cast<unsigned>(jpegBytes.size()));
  return jpegBytes;
}

bool SpotifyClient::refreshAccessToken() {
  const auto tlsClient = createTlsClient();
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  if (!http.begin(*tlsClient, kTokenUrl)) {
    return false;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const String form = String("grant_type=refresh_token&refresh_token=") + refreshToken_.c_str() +
                      "&client_id=" + clientId_.c_str();
  const int statusCode = http.POST(form);
  const std::string body = readBody(http);
  http.end();
  log_i("token refresh status=%d", statusCode);
  const std::optional<AccessToken> token = parseTokenResponse(body);
  if (statusCode != HTTP_CODE_OK || !token.has_value()) {
    return false;
  }
  accessToken_ = token->accessToken;
  accessTokenExpiryMs_ = millis() + token->expiresInSeconds * 1000 - kTokenExpiryMarginMs;
  if (!token->refreshToken.empty() && token->refreshToken != refreshToken_) {
    refreshToken_ = token->refreshToken;
    storeRotatedRefreshToken(refreshToken_);
  }
  return true;
}

bool SpotifyClient::ensureAccessToken() {
  const bool isExpired = static_cast<int32_t>(millis() - accessTokenExpiryMs_) >= 0;
  if (!accessToken_.empty() && !isExpired) {
    return true;
  }
  return refreshAccessToken();
}

HttpResult SpotifyClient::sendApiRequest(const char* method, const std::string& path) {
  if (!ensureAccessToken()) {
    return {kHttpUnauthorized, {}};
  }
  HttpResult result = sendAuthorizedRequest(method, path);
  // Spotify can revoke an access token before its stated expiry.
  if (result.statusCode == kHttpUnauthorized && refreshAccessToken()) {
    result = sendAuthorizedRequest(method, path);
  }
  return result;
}

HttpResult SpotifyClient::sendAuthorizedRequest(const char* method, const std::string& path) {
  const auto tlsClient = createTlsClient();
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  const std::string url = std::string(kApiBaseUrl) + path;
  if (!http.begin(*tlsClient, url.c_str())) {
    return {-1, {}};
  }
  http.addHeader("Authorization", String("Bearer ") + accessToken_.c_str());
  // HTTPClient omits Content-Length for empty bodies, and Spotify rejects
  // body-less PUT/POST/DELETE without it (411 Length Required).
  if (strcmp(method, "GET") != 0) {
    http.addHeader("Content-Length", "0");
  }
  const int statusCode = http.sendRequest(method);
  HttpResult result{statusCode, readBody(http)};
  http.end();
  return result;
}
