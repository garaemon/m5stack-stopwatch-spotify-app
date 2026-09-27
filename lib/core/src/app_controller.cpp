#include "app_controller.h"

std::vector<Effect> AppController::handlePlayback(const PlaybackState& playback) {
  (void)playback;
  return {};
}

std::vector<Effect> AppController::handleCommand(UserCommand command) {
  (void)command;
  return {};
}

std::vector<Effect> AppController::handleLikeStatus(const std::string& trackUri, bool isLiked) {
  (void)trackUri;
  (void)isLiked;
  return {};
}
