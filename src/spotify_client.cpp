#include "spotify_client.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Preferences.h>

#include <cstring>
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
constexpr int kHttpBadRequest = 400;
constexpr int kTransportError = -1;

std::unique_ptr<NetworkClientSecure> createTlsClient() {
  auto tlsClient = std::make_unique<NetworkClientSecure>();
  tlsClient->useBuiltinCACertBundle();
  return tlsClient;
}

// Each request opens a fresh TLS client, so ask the server to close the
// connection; otherwise a body without Content-Length never ends.
void disableConnectionReuse(HTTPClient& http) { http.setReuse(false); }

std::string readBody(HTTPClient& http) {
  const String body = http.getString();
  return std::string(body.c_str(), body.length());
}

// Reads the body straight into a std::string so that the JPEG is held once;
// getString() would build an Arduino String first and copy it afterwards.
std::string readBinaryBody(HTTPClient& http) {
  const int contentLength = http.getSize();
  if (contentLength <= 0) {
    // Chunked responses have no Content-Length; getString() decodes them.
    return readBody(http);
  }
  std::string body(static_cast<size_t>(contentLength), '\0');
  const size_t readBytes = http.getStream().readBytes(&body[0], body.size());
  // A read that timed out leaves a truncated JPEG that must not be decoded.
  return readBytes == body.size() ? body : std::string();
}

// Only a rejection by Spotify means the user must authenticate again; a
// Wi-Fi blip must not show "login expired".
int mapTokenFailureToApiStatus(int tokenStatus) {
  if (tokenStatus == kHttpBadRequest || tokenStatus == kHttpUnauthorized) {
    return kHttpUnauthorized;
  }
  return tokenStatus > 0 ? tokenStatus : kTransportError;
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

HttpResult SpotifyClient::fetchQueue() { return sendApiRequest("GET", "/v1/me/player/queue"); }

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

HttpResult SpotifyClient::seekToStart() { return sendApiRequest("PUT", "/v1/me/player/seek?position_ms=0"); }

HttpResult SpotifyClient::pausePlayback() { return sendApiRequest("PUT", "/v1/me/player/pause"); }

HttpResult SpotifyClient::resumePlayback() { return sendApiRequest("PUT", "/v1/me/player/play"); }

std::string SpotifyClient::downloadImage(const std::string& url) {
  const auto tlsClient = createTlsClient();
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  disableConnectionReuse(http);
  if (!http.begin(*tlsClient, url.c_str())) {
    return {};
  }
  const int statusCode = http.GET();
  const std::string jpegBytes = statusCode == kHttpStatusOk ? readBinaryBody(http) : std::string();
  http.end();
  log_i("artwork status=%d bytes=%u", statusCode, static_cast<unsigned>(jpegBytes.size()));
  return jpegBytes;
}

int SpotifyClient::refreshAccessToken() {
  const auto tlsClient = createTlsClient();
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  disableConnectionReuse(http);
  if (!http.begin(*tlsClient, kTokenUrl)) {
    return kTransportError;
  }
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const String form =
      String("grant_type=refresh_token&refresh_token=") + refreshToken_.c_str() + "&client_id=" + clientId_.c_str();
  const int statusCode = http.POST(form);
  const std::string body = readBody(http);
  http.end();
  log_i("token refresh status=%d", statusCode);
  const std::optional<AccessToken> token = parseTokenResponse(body);
  if (statusCode != kHttpStatusOk) {
    return statusCode;
  }
  if (!token.has_value()) {
    return kTransportError;
  }
  accessToken_ = token->accessToken;
  accessTokenExpiryMs_ = millis() + token->expiresInSeconds * 1000 - kTokenExpiryMarginMs;
  if (!token->refreshToken.empty() && token->refreshToken != refreshToken_) {
    refreshToken_ = token->refreshToken;
    storeRotatedRefreshToken(refreshToken_);
  }
  return kHttpStatusOk;
}

int SpotifyClient::ensureAccessToken() {
  const bool isExpired = static_cast<int32_t>(millis() - accessTokenExpiryMs_) >= 0;
  if (!accessToken_.empty() && !isExpired) {
    return kHttpStatusOk;
  }
  return refreshAccessToken();
}

HttpResult SpotifyClient::sendApiRequest(const char* method, const std::string& path) {
  const int tokenStatus = ensureAccessToken();
  if (tokenStatus != kHttpStatusOk) {
    return {mapTokenFailureToApiStatus(tokenStatus), {}};
  }
  HttpResult result = sendAuthorizedRequest(method, path);
  // Spotify can revoke an access token before its stated expiry.
  if (result.statusCode == kHttpUnauthorized) {
    const int refreshStatus = refreshAccessToken();
    if (refreshStatus != kHttpStatusOk) {
      return {mapTokenFailureToApiStatus(refreshStatus), {}};
    }
    result = sendAuthorizedRequest(method, path);
  }
  return result;
}

HttpResult SpotifyClient::sendAuthorizedRequest(const char* method, const std::string& path) {
  const auto tlsClient = createTlsClient();
  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  disableConnectionReuse(http);
  const std::string url = std::string(kApiBaseUrl) + path;
  if (!http.begin(*tlsClient, url.c_str())) {
    return {kTransportError, {}};
  }
  http.addHeader("Authorization", String("Bearer ") + accessToken_.c_str());
  // HTTPClient omits Content-Length for empty bodies, and Spotify rejects
  // body-less PUT/POST/DELETE without it (411 Length Required).
  if (strcmp(method, "GET") != 0) {
    http.addHeader("Content-Length", "0");
  }
  const char* collectedHeaderKeys[] = {"Retry-After"};
  http.collectHeaders(collectedHeaderKeys, 1);
  const int statusCode = http.sendRequest(method);
  // A 204 carries no body; reading one anyway blocks until the server
  // drops the connection, which froze polling while nothing played.
  const bool hasBody = statusCode > 0 && statusCode != HTTP_CODE_NO_CONTENT;
  HttpResult result{statusCode, hasBody ? readBody(http) : std::string(),
                    static_cast<uint32_t>(http.header("Retry-After").toInt())};
  http.end();
  return result;
}
