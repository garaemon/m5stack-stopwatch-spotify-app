#include <unity.h>

#include <algorithm>

#include "app_controller.h"

namespace {

const char* const kTrackUri = "spotify:track:first";
const char* const kOtherTrackUri = "spotify:track:second";

PlaybackState makeTrack(const std::string& trackUri, const std::string& artworkUrl) {
  PlaybackState playback;
  playback.hasTrack = true;
  playback.isPlaying = true;
  playback.trackUri = trackUri;
  playback.artworkUrl = artworkUrl;
  return playback;
}

bool containsEffect(const std::vector<Effect>& effects, EffectType type, const std::string& argument = "") {
  return std::find(effects.begin(), effects.end(), Effect{type, argument}) != effects.end();
}

AppController makeControllerPlaying(bool isLiked) {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));
  controller.handleLikeStatus(kTrackUri, isLiked);
  return controller;
}

}  // namespace

void should_fetch_artwork_when_track_changes() {
  AppController controller;

  const auto effects = controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  TEST_ASSERT_TRUE(containsEffect(effects, EffectType::kFetchArtwork, "https://art/1"));
}

void should_fetch_like_status_when_track_changes() {
  AppController controller;

  const auto effects = controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  TEST_ASSERT_TRUE(containsEffect(effects, EffectType::kFetchLikeStatus, kTrackUri));
}

void should_not_refetch_artwork_for_same_track() {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  const auto effects = controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  TEST_ASSERT_FALSE(containsEffect(effects, EffectType::kFetchArtwork, "https://art/1"));
}

void should_not_refetch_artwork_when_next_track_shares_album() {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  const auto effects = controller.handlePlayback(makeTrack(kOtherTrackUri, "https://art/1"));

  TEST_ASSERT_FALSE(containsEffect(effects, EffectType::kFetchArtwork, "https://art/1"));
}

void should_render_on_every_playback_update() {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  const auto effects = controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  TEST_ASSERT_TRUE(containsEffect(effects, EffectType::kRender));
}

void should_reset_like_status_when_track_changes() {
  AppController controller = makeControllerPlaying(true);

  controller.handlePlayback(makeTrack(kOtherTrackUri, "https://art/2"));

  TEST_ASSERT_TRUE(controller.likeStatus() == LikeStatus::kUnknown);
}

void should_skip_next_on_next_command() {
  AppController controller = makeControllerPlaying(false);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kNext), EffectType::kSkipNext));
}

void should_skip_previous_on_previous_command() {
  AppController controller = makeControllerPlaying(false);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kPrevious), EffectType::kSkipPrevious));
}

void should_save_track_when_toggling_unliked_track() {
  AppController controller = makeControllerPlaying(false);

  const auto effects = controller.handleCommand(UserCommand::kToggleLike);

  TEST_ASSERT_TRUE(containsEffect(effects, EffectType::kSaveTrack, kTrackUri));
}

void should_mark_liked_optimistically_when_saving() {
  AppController controller = makeControllerPlaying(false);

  controller.handleCommand(UserCommand::kToggleLike);

  TEST_ASSERT_TRUE(controller.likeStatus() == LikeStatus::kLiked);
}

void should_remove_track_when_toggling_liked_track() {
  AppController controller = makeControllerPlaying(true);

  const auto effects = controller.handleCommand(UserCommand::kToggleLike);

  TEST_ASSERT_TRUE(containsEffect(effects, EffectType::kRemoveTrack, kTrackUri));
}

void should_vibrate_when_toggling_like() {
  AppController controller = makeControllerPlaying(false);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kToggleLike), EffectType::kVibrate));
}

void should_ignore_toggle_while_like_status_unknown() {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  TEST_ASSERT_TRUE(controller.handleCommand(UserCommand::kToggleLike).empty());
}

void should_ignore_toggle_without_track() {
  AppController controller;

  TEST_ASSERT_TRUE(controller.handleCommand(UserCommand::kToggleLike).empty());
}

void should_ignore_like_status_of_stale_track() {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  controller.handleLikeStatus(kOtherTrackUri, true);

  TEST_ASSERT_TRUE(controller.likeStatus() == LikeStatus::kUnknown);
}

void should_render_when_like_status_arrives() {
  AppController controller;
  controller.handlePlayback(makeTrack(kTrackUri, "https://art/1"));

  TEST_ASSERT_TRUE(containsEffect(controller.handleLikeStatus(kTrackUri, true), EffectType::kRender));
}

void should_start_in_connecting_status() {
  AppController controller;

  TEST_ASSERT_TRUE(controller.connectionStatus() == ConnectionStatus::kConnecting);
}

void should_store_new_connection_status() {
  AppController controller;

  controller.handleConnectionStatus(ConnectionStatus::kWifiDisconnected);

  TEST_ASSERT_TRUE(controller.connectionStatus() == ConnectionStatus::kWifiDisconnected);
}

void should_render_when_connection_status_changes() {
  AppController controller;

  TEST_ASSERT_TRUE(containsEffect(controller.handleConnectionStatus(ConnectionStatus::kOk), EffectType::kRender));
}

void should_not_render_when_connection_status_is_unchanged() {
  AppController controller;
  controller.handleConnectionStatus(ConnectionStatus::kOk);

  TEST_ASSERT_TRUE(controller.handleConnectionStatus(ConnectionStatus::kOk).empty());
}

void should_seek_to_start_on_restart_command() {
  AppController controller = makeControllerPlaying(false);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kRestartTrack), EffectType::kSeekToStart));
}

void should_pause_when_toggling_playing_track() {
  AppController controller = makeControllerPlaying(false);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kTogglePlayback), EffectType::kPause));
}

void should_mark_paused_optimistically_when_pausing() {
  AppController controller = makeControllerPlaying(false);

  controller.handleCommand(UserCommand::kTogglePlayback);

  TEST_ASSERT_FALSE(controller.playback().isPlaying);
}

void should_resume_when_toggling_paused_track() {
  AppController controller;
  PlaybackState pausedTrack = makeTrack(kTrackUri, "https://art/1");
  pausedTrack.isPlaying = false;
  controller.handlePlayback(pausedTrack);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kTogglePlayback), EffectType::kResume));
}

void should_render_when_toggling_playback() {
  AppController controller = makeControllerPlaying(false);

  TEST_ASSERT_TRUE(containsEffect(controller.handleCommand(UserCommand::kTogglePlayback), EffectType::kRender));
}

void should_ignore_toggle_playback_without_track() {
  AppController controller;

  TEST_ASSERT_TRUE(controller.handleCommand(UserCommand::kTogglePlayback).empty());
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_fetch_artwork_when_track_changes);
  RUN_TEST(should_fetch_like_status_when_track_changes);
  RUN_TEST(should_not_refetch_artwork_for_same_track);
  RUN_TEST(should_not_refetch_artwork_when_next_track_shares_album);
  RUN_TEST(should_render_on_every_playback_update);
  RUN_TEST(should_reset_like_status_when_track_changes);
  RUN_TEST(should_skip_next_on_next_command);
  RUN_TEST(should_skip_previous_on_previous_command);
  RUN_TEST(should_save_track_when_toggling_unliked_track);
  RUN_TEST(should_mark_liked_optimistically_when_saving);
  RUN_TEST(should_remove_track_when_toggling_liked_track);
  RUN_TEST(should_vibrate_when_toggling_like);
  RUN_TEST(should_ignore_toggle_while_like_status_unknown);
  RUN_TEST(should_ignore_toggle_without_track);
  RUN_TEST(should_ignore_like_status_of_stale_track);
  RUN_TEST(should_render_when_like_status_arrives);
  RUN_TEST(should_start_in_connecting_status);
  RUN_TEST(should_store_new_connection_status);
  RUN_TEST(should_render_when_connection_status_changes);
  RUN_TEST(should_not_render_when_connection_status_is_unchanged);
  RUN_TEST(should_seek_to_start_on_restart_command);
  RUN_TEST(should_pause_when_toggling_playing_track);
  RUN_TEST(should_mark_paused_optimistically_when_pausing);
  RUN_TEST(should_resume_when_toggling_paused_track);
  RUN_TEST(should_render_when_toggling_playback);
  RUN_TEST(should_ignore_toggle_playback_without_track);
  return UNITY_END();
}
