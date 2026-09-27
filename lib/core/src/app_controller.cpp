#include "app_controller.h"

std::vector<Effect> AppController::handlePlayback(const PlaybackState& playback) {
  std::vector<Effect> effects;
  const bool isArtworkChanged = playback.hasTrack && playback.artworkUrl != playback_.artworkUrl;
  const bool isTrackChanged = playback.hasTrack && playback.trackUri != playback_.trackUri;
  if (isArtworkChanged) {
    effects.push_back({EffectType::kFetchArtwork, playback.artworkUrl});
  }
  if (isTrackChanged) {
    likeStatus_ = LikeStatus::kUnknown;
    effects.push_back({EffectType::kFetchLikeStatus, playback.trackUri});
  }
  playback_ = playback;
  effects.push_back({EffectType::kRender, ""});
  return effects;
}

std::vector<Effect> AppController::handleCommand(UserCommand command) {
  switch (command) {
    case UserCommand::kNext:
      return {{EffectType::kSkipNext, ""}};
    case UserCommand::kPrevious:
      return {{EffectType::kSkipPrevious, ""}};
    case UserCommand::kRestartTrack:
      return {{EffectType::kSeekToStart, ""}};
    case UserCommand::kTogglePlayback:
      return togglePlayback();
    case UserCommand::kToggleLike:
      return toggleLike();
  }
  return {};
}

std::vector<Effect> AppController::togglePlayback() {
  if (!playback_.hasTrack) {
    return {};
  }
  // Show the new state at once; the poll that follows the request corrects
  // it if Spotify refused, for example when no device is active.
  playback_.isPlaying = !playback_.isPlaying;
  const EffectType playbackEffect = playback_.isPlaying ? EffectType::kResume : EffectType::kPause;
  return {{EffectType::kRender, ""}, {playbackEffect, ""}};
}

std::vector<Effect> AppController::toggleLike() {
  if (!playback_.hasTrack || likeStatus_ == LikeStatus::kUnknown) {
    return {};
  }
  // Flip the heart before the API answers so that the tap feels instant;
  // the next track change re-reads the real status anyway.
  const bool isSaving = likeStatus_ == LikeStatus::kNotLiked;
  likeStatus_ = isSaving ? LikeStatus::kLiked : LikeStatus::kNotLiked;
  const EffectType libraryEffect = isSaving ? EffectType::kSaveTrack : EffectType::kRemoveTrack;
  return {
      {EffectType::kVibrate, ""},
      {EffectType::kRender, ""},
      {libraryEffect, playback_.trackUri},
  };
}

std::vector<Effect> AppController::handleLikeStatus(const std::string& trackUri, bool isLiked) {
  if (trackUri != playback_.trackUri) {
    return {};
  }
  likeStatus_ = isLiked ? LikeStatus::kLiked : LikeStatus::kNotLiked;
  return {{EffectType::kRender, ""}};
}

std::vector<Effect> AppController::handleConnectionStatus(ConnectionStatus status) {
  if (status == connectionStatus_) {
    return {};
  }
  connectionStatus_ = status;
  return {{EffectType::kRender, ""}};
}
