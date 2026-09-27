#include <unity.h>

#include "spotify_parser.h"

namespace {

const char* const kTrackJson = R"({
  "is_playing": true,
  "progress_ms": 12345,
  "currently_playing_type": "track",
  "item": {
    "uri": "spotify:track:abc123",
    "name": "Song Title",
    "duration_ms": 200000,
    "artists": [{"name": "Artist One"}, {"name": "Artist Two"}],
    "album": {
      "images": [
        {"url": "https://i.scdn.co/image/large", "width": 640, "height": 640},
        {"url": "https://i.scdn.co/image/medium", "width": 300, "height": 300},
        {"url": "https://i.scdn.co/image/small", "width": 64, "height": 64}
      ]
    }
  }
})";

const char* const kSmallImagesOnlyJson = R"({
  "is_playing": true,
  "currently_playing_type": "track",
  "item": {
    "uri": "spotify:track:abc123",
    "name": "Song Title",
    "artists": [],
    "album": {"images": [
      {"url": "https://i.scdn.co/image/small", "width": 64, "height": 64},
      {"url": "https://i.scdn.co/image/medium", "width": 300, "height": 300}
    ]}
  }
})";

const char* const kEpisodeJson = R"({
  "is_playing": true,
  "currently_playing_type": "episode",
  "item": null
})";

const char* const kQueueJson = R"({
  "currently_playing": {"uri": "spotify:track:now"},
  "queue": [
    {"uri": "spotify:track:next", "album": {"images": [
      {"url": "https://i.scdn.co/image/next-large", "width": 640, "height": 640},
      {"url": "https://i.scdn.co/image/next-small", "width": 64, "height": 64}
    ]}},
    {"uri": "spotify:track:later", "album": {"images": [
      {"url": "https://i.scdn.co/image/later", "width": 640, "height": 640}
    ]}}
  ]
})";

const char* const kQueueStartingWithEpisodeJson = R"({
  "queue": [{"uri": "spotify:episode:ep", "images": [{"url": "https://i.scdn.co/image/ep", "width": 640}]}]
})";

PlaybackState parseTrackJson() { return parseCurrentlyPlaying(kTrackJson).value(); }

}  // namespace

void should_report_track_when_track_is_playing() { TEST_ASSERT_TRUE(parseTrackJson().hasTrack); }

void should_read_is_playing() { TEST_ASSERT_TRUE(parseTrackJson().isPlaying); }

void should_read_track_uri() { TEST_ASSERT_EQUAL_STRING("spotify:track:abc123", parseTrackJson().trackUri.c_str()); }

void should_read_title() { TEST_ASSERT_EQUAL_STRING("Song Title", parseTrackJson().title.c_str()); }

void should_join_artist_names_with_comma() {
  TEST_ASSERT_EQUAL_STRING("Artist One, Artist Two", parseTrackJson().artists.c_str());
}

void should_read_progress_ms() { TEST_ASSERT_EQUAL_UINT32(12345, parseTrackJson().progressMs); }

void should_read_duration_ms() { TEST_ASSERT_EQUAL_UINT32(200000, parseTrackJson().durationMs); }

void should_pick_smallest_image_covering_display() {
  TEST_ASSERT_EQUAL_STRING("https://i.scdn.co/image/large", parseTrackJson().artworkUrl.c_str());
}

void should_pick_largest_image_when_none_covers_display() {
  const PlaybackState playback = parseCurrentlyPlaying(kSmallImagesOnlyJson).value();

  TEST_ASSERT_EQUAL_STRING("https://i.scdn.co/image/medium", playback.artworkUrl.c_str());
}

void should_report_no_track_for_episode() {
  TEST_ASSERT_FALSE(parseCurrentlyPlaying(kEpisodeJson).value().hasTrack);
}

void should_report_no_track_for_empty_body() { TEST_ASSERT_FALSE(parseCurrentlyPlaying("").value().hasTrack); }

void should_return_nullopt_for_invalid_json() { TEST_ASSERT_FALSE(parseCurrentlyPlaying("{broken").has_value()); }

void should_return_true_when_library_contains_track() {
  TEST_ASSERT_TRUE(parseLibraryContains("[true]").value());
}

void should_return_false_when_library_lacks_track() {
  TEST_ASSERT_FALSE(parseLibraryContains("[false]").value());
}

void should_return_nullopt_for_empty_contains_array() { TEST_ASSERT_FALSE(parseLibraryContains("[]").has_value()); }

void should_read_access_token() {
  const auto token = parseTokenResponse(R"({"access_token":"at","expires_in":3600})");

  TEST_ASSERT_EQUAL_STRING("at", token.value().accessToken.c_str());
}

void should_read_expires_in() {
  const auto token = parseTokenResponse(R"({"access_token":"at","expires_in":3600})");

  TEST_ASSERT_EQUAL_UINT32(3600, token.value().expiresInSeconds);
}

void should_read_rotated_refresh_token() {
  const auto token = parseTokenResponse(R"({"access_token":"at","expires_in":3600,"refresh_token":"rt"})");

  TEST_ASSERT_EQUAL_STRING("rt", token.value().refreshToken.c_str());
}

void should_return_nullopt_when_access_token_missing() {
  TEST_ASSERT_FALSE(parseTokenResponse(R"({"error":"invalid_grant"})").has_value());
}

void should_return_artwork_of_first_queued_track() {
  TEST_ASSERT_EQUAL_STRING("https://i.scdn.co/image/next-large", parseNextQueuedArtworkUrl(kQueueJson).value().c_str());
}

void should_return_nullopt_for_empty_queue() {
  TEST_ASSERT_FALSE(parseNextQueuedArtworkUrl(R"({"queue": []})").has_value());
}

void should_return_nullopt_when_queue_starts_with_episode() {
  TEST_ASSERT_FALSE(parseNextQueuedArtworkUrl(kQueueStartingWithEpisodeJson).has_value());
}

void should_return_nullopt_for_invalid_queue_json() {
  TEST_ASSERT_FALSE(parseNextQueuedArtworkUrl("{broken").has_value());
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_report_track_when_track_is_playing);
  RUN_TEST(should_read_is_playing);
  RUN_TEST(should_read_track_uri);
  RUN_TEST(should_read_title);
  RUN_TEST(should_join_artist_names_with_comma);
  RUN_TEST(should_read_progress_ms);
  RUN_TEST(should_read_duration_ms);
  RUN_TEST(should_pick_smallest_image_covering_display);
  RUN_TEST(should_pick_largest_image_when_none_covers_display);
  RUN_TEST(should_report_no_track_for_episode);
  RUN_TEST(should_report_no_track_for_empty_body);
  RUN_TEST(should_return_nullopt_for_invalid_json);
  RUN_TEST(should_return_true_when_library_contains_track);
  RUN_TEST(should_return_false_when_library_lacks_track);
  RUN_TEST(should_return_nullopt_for_empty_contains_array);
  RUN_TEST(should_read_access_token);
  RUN_TEST(should_read_expires_in);
  RUN_TEST(should_read_rotated_refresh_token);
  RUN_TEST(should_return_nullopt_when_access_token_missing);
  RUN_TEST(should_return_artwork_of_first_queued_track);
  RUN_TEST(should_return_nullopt_for_empty_queue);
  RUN_TEST(should_return_nullopt_when_queue_starts_with_episode);
  RUN_TEST(should_return_nullopt_for_invalid_queue_json);
  return UNITY_END();
}
