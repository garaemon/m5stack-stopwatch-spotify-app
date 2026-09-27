#pragma once

#include <string>
#include <vector>

#include "connection_status.h"
#include "playback_state.h"

enum class UserCommand { kRestartTrack, kPrevious, kTogglePlayback, kNext, kToggleLike };

enum class LikeStatus { kUnknown, kLiked, kNotLiked };

enum class EffectType {
  kFetchArtwork,     // argument: artwork URL
  kFetchLikeStatus,  // argument: track URI
  kSaveTrack,        // argument: track URI
  kRemoveTrack,      // argument: track URI
  kSkipNext,
  kSkipPrevious,
  kSeekToStart,
  kPause,
  kResume,
  kVibrate,
  kRender,
};

struct Effect {
  EffectType type;
  std::string argument;

  bool operator==(const Effect& other) const {
    return type == other.type && argument == other.argument;
  }
};

// Decides which side effects the device must run for each event.
// Owns no I/O so that the logic runs in native unit tests.
class AppController {
 public:
  std::vector<Effect> handlePlayback(const PlaybackState& playback);
  std::vector<Effect> handleCommand(UserCommand command);
  // Ignores results for a track that is no longer playing.
  std::vector<Effect> handleLikeStatus(const std::string& trackUri, bool isLiked);
  std::vector<Effect> handleConnectionStatus(ConnectionStatus status);

  const PlaybackState& playback() const { return playback_; }
  LikeStatus likeStatus() const { return likeStatus_; }
  ConnectionStatus connectionStatus() const { return connectionStatus_; }

 private:
  std::vector<Effect> toggleLike();
  std::vector<Effect> togglePlayback();

  PlaybackState playback_;
  LikeStatus likeStatus_ = LikeStatus::kUnknown;
  ConnectionStatus connectionStatus_ = ConnectionStatus::kConnecting;
};
