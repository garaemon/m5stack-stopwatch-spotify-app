#pragma once

#include <atomic>
#include <optional>
#include <string>

#include "app_controller.h"
#include "artwork_decoder.h"
#include "connection_status.h"
#include "lru_cache.h"
#include "playback_state.h"
#include "spotify_client.h"
#include "thread_safe_queue.h"

enum class NetworkResultType { kPlayback, kLikeStatus, kArtwork, kConnectionStatus };

struct NetworkResult {
  NetworkResultType type;
  PlaybackState playback;   // kPlayback
  std::string trackUri;     // kLikeStatus
  bool isLiked = false;     // kLikeStatus
  ArtworkImage artwork;     // kArtwork; nullptr when the download failed.
  ConnectionStatus connectionStatus = ConnectionStatus::kConnecting;  // kConnectionStatus
};

// Runs every Spotify request on a dedicated FreeRTOS task so that HTTPS
// latency never stalls input handling or rendering on the Arduino loop task.
class NetworkWorker {
 public:
  explicit NetworkWorker(SpotifyClient& spotify) : spotify_(spotify) {}

  void start();
  // Accepts only the network effects; the caller handles kVibrate and kRender.
  void request(const Effect& effect) { requests_.push(effect); }
  std::optional<NetworkResult> tryPopResult() { return results_.tryPop(); }
  // Takes effect after the currently scheduled poll.
  void setPollIntervalMs(uint32_t intervalMs) { pollIntervalMs_ = intervalMs; }

 private:
  static void runTask(void* worker);
  void runLoop();
  void handleRequest(const Effect& effect);
  void pollPlayback();
  void reportConnectionStatus(ConnectionStatus status);
  void fetchLikeStatus(const std::string& trackUri);
  ArtworkImage loadArtwork(const std::string& url);
  void prefetchNextArtwork();

  SpotifyClient& spotify_;
  ThreadSafeQueue<Effect> requests_;
  ThreadSafeQueue<NetworkResult> results_;
  uint32_t nextPollMs_ = 0;
  std::atomic<uint32_t> pollIntervalMs_{3000};
  bool hasEverConnectedWifi_ = false;
  // Holds the previous, current, and next artworks plus one spare.
  LruCache<std::string, ArtworkImage> artworkCache_{4};
  std::string lastPolledTrackUri_;
  std::optional<uint32_t> prefetchDueMs_;
};
