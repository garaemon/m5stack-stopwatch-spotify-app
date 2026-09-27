#include <unity.h>

#include "text_fitting.h"

namespace {

// Counts one pixel per UTF-8 character so that tests read in characters.
int32_t countCharacters(const std::string& text) {
  int32_t characterCount = 0;
  for (const char byte : text) {
    if ((static_cast<unsigned char>(byte) & 0xC0) != 0x80) {
      ++characterCount;
    }
  }
  return characterCount;
}

}  // namespace

void should_keep_text_that_fits() {
  TEST_ASSERT_EQUAL_STRING("Hello", fitTextToWidth("Hello", 5, countCharacters).c_str());
}

void should_append_ellipsis_when_text_overflows() {
  TEST_ASSERT_EQUAL_STRING("He...", fitTextToWidth("Hello World", 5, countCharacters).c_str());
}

void should_cut_multibyte_text_on_character_boundary() {
  TEST_ASSERT_EQUAL_STRING("\xE3\x81\x82\xE3\x81\x84...",
                           fitTextToWidth("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A\xE3\x81\x8B",
                                          5, countCharacters)
                               .c_str());
}

void should_return_ellipsis_only_when_nothing_else_fits() {
  TEST_ASSERT_EQUAL_STRING("...", fitTextToWidth("Hello", 3, countCharacters).c_str());
}

void setUp() {}
void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(should_keep_text_that_fits);
  RUN_TEST(should_append_ellipsis_when_text_overflows);
  RUN_TEST(should_cut_multibyte_text_on_character_boundary);
  RUN_TEST(should_return_ellipsis_only_when_nothing_else_fits);
  return UNITY_END();
}
