#pragma once

// Health of the link to Spotify, derived from the latest playback poll.
enum class ConnectionStatus {
  kConnecting,
  kOk,
  kWifiDisconnected,
  kAuthFailed,
  kRateLimited,
  kServerError,
  kNetworkError,
};

// Maps a playback poll status code to a connection status.
// Negative codes are transport errors from HTTPClient.
ConnectionStatus classifyPollStatusCode(int statusCode);
