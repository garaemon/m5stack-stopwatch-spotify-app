#include <unity.h>

#include "display_text.h"

void should_keep_ascii_text() { TEST_ASSERT_EQUAL_STRING("Hello", normalizeForDisplay("Hello").c_str()); }

void should_keep_fullwidth_japanese_text() {
  TEST_ASSERT_EQUAL_STRING("\u591C\u306B\u99C6\u3051\u308B", normalizeForDisplay("\u591C\u306B\u99C6\u3051\u308B").c_str());
}

void should_convert_halfwidth_katakana_to_fullwidth() {
  TEST_ASSERT_EQUAL_STRING("\u30C6\u30B9\u30C8", normalizeForDisplay("\uFF83\uFF7D\uFF84").c_str());
}

void should_merge_voiced_sound_mark() { TEST_ASSERT_EQUAL_STRING("\u30AC", normalizeForDisplay("\uFF76\uFF9E").c_str()); }

void should_merge_semi_voiced_sound_mark() { TEST_ASSERT_EQUAL_STRING("\u30D1", normalizeForDisplay("\uFF8A\uFF9F").c_str()); }

void should_merge_voiced_mark_into_vu() { TEST_ASSERT_EQUAL_STRING("\u30F4", normalizeForDisplay("\uFF73\uFF9E").c_str()); }

void should_keep_standalone_voiced_mark_as_fullwidth() {
  TEST_ASSERT_EQUAL_STRING("\u30A2\u309B", normalizeForDisplay("\uFF71\uFF9E").c_str());
}

void should_convert_halfwidth_punctuation() {
  TEST_ASSERT_EQUAL_STRING("\u300C\u30FC\u300D", normalizeForDisplay("\uFF62\uFF70\uFF63").c_str());
}

void should_convert_fullwidth_tilde_to_wave_dash() {
  TEST_ASSERT_EQUAL_STRING("A\u301CB", normalizeForDisplay("A\uFF5EB").c_str());
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_keep_ascii_text);
  RUN_TEST(should_keep_fullwidth_japanese_text);
  RUN_TEST(should_convert_halfwidth_katakana_to_fullwidth);
  RUN_TEST(should_merge_voiced_sound_mark);
  RUN_TEST(should_merge_semi_voiced_sound_mark);
  RUN_TEST(should_merge_voiced_mark_into_vu);
  RUN_TEST(should_keep_standalone_voiced_mark_as_fullwidth);
  RUN_TEST(should_convert_halfwidth_punctuation);
  RUN_TEST(should_convert_fullwidth_tilde_to_wave_dash);
  return UNITY_END();
}
