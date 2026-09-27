#include "display_text.h"

#include <vector>

namespace {

constexpr char32_t kHalfwidthFirst = 0xFF61;
constexpr char32_t kHalfwidthLast = 0xFF9F;
constexpr char32_t kHalfwidthVoicedMark = 0xFF9E;
constexpr char32_t kHalfwidthSemiVoicedMark = 0xFF9F;
constexpr char32_t kFullwidthTilde = 0xFF5E;
constexpr char32_t kWaveDash = 0x301C;
constexpr char32_t kKatakanaU = 0x30A6;
constexpr char32_t kKatakanaVu = 0x30F4;

// Fullwidth forms of U+FF61..U+FF9F in code point order.
constexpr char32_t kFullwidthForHalfwidth[] = {
    0x3002, 0x300C, 0x300D, 0x3001, 0x30FB, 0x30F2, 0x30A1, 0x30A3, 0x30A5, 0x30A7, 0x30A9, 0x30E3, 0x30E5,
    0x30E7, 0x30C3, 0x30FC, 0x30A2, 0x30A4, 0x30A6, 0x30A8, 0x30AA, 0x30AB, 0x30AD, 0x30AF, 0x30B1, 0x30B3,
    0x30B5, 0x30B7, 0x30B9, 0x30BB, 0x30BD, 0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8, 0x30CA, 0x30CB, 0x30CC,
    0x30CD, 0x30CE, 0x30CF, 0x30D2, 0x30D5, 0x30D8, 0x30DB, 0x30DE, 0x30DF, 0x30E0, 0x30E1, 0x30E2, 0x30E4,
    0x30E6, 0x30E8, 0x30E9, 0x30EA, 0x30EB, 0x30EC, 0x30ED, 0x30EF, 0x30F3, 0x309B, 0x309C,
};

// Katakana whose voiced form sits at the next code point (ka..to).
bool isVoiceableKaToTo(char32_t kana) {
  constexpr char32_t kVoiceable[] = {0x30AB, 0x30AD, 0x30AF, 0x30B1, 0x30B3, 0x30B5, 0x30B7, 0x30B9,
                                     0x30BB, 0x30BD, 0x30BF, 0x30C1, 0x30C4, 0x30C6, 0x30C8};
  for (const char32_t voiceable : kVoiceable) {
    if (kana == voiceable) {
      return true;
    }
  }
  return false;
}

// Ha, hi, fu, he, ho: voiced at +1 and semi-voiced at +2.
bool isHaRow(char32_t kana) { return kana >= 0x30CF && kana <= 0x30DB && (kana - 0x30CF) % 3 == 0; }

std::vector<char32_t> decodeUtf8(const std::string& utf8Text) {
  std::vector<char32_t> codePoints;
  size_t index = 0;
  while (index < utf8Text.size()) {
    const auto leadByte = static_cast<unsigned char>(utf8Text[index]);
    const size_t length = leadByte < 0x80 ? 1 : leadByte < 0xE0 ? 2 : leadByte < 0xF0 ? 3 : 4;
    char32_t codePoint = length == 1 ? leadByte : leadByte & (0xFF >> (length + 1));
    for (size_t offset = 1; offset < length && index + offset < utf8Text.size(); ++offset) {
      codePoint = (codePoint << 6) | (static_cast<unsigned char>(utf8Text[index + offset]) & 0x3F);
    }
    codePoints.push_back(codePoint);
    index += length;
  }
  return codePoints;
}

void appendUtf8(std::string& output, char32_t codePoint) {
  if (codePoint < 0x80) {
    output += static_cast<char>(codePoint);
  } else if (codePoint < 0x800) {
    output += static_cast<char>(0xC0 | (codePoint >> 6));
    output += static_cast<char>(0x80 | (codePoint & 0x3F));
  } else if (codePoint < 0x10000) {
    output += static_cast<char>(0xE0 | (codePoint >> 12));
    output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
    output += static_cast<char>(0x80 | (codePoint & 0x3F));
  } else {
    output += static_cast<char>(0xF0 | (codePoint >> 18));
    output += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
    output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
    output += static_cast<char>(0x80 | (codePoint & 0x3F));
  }
}

// Returns the kana with the sound mark merged in, or 0 when they do not merge.
char32_t mergeSoundMark(char32_t kana, char32_t halfwidthMark) {
  if (halfwidthMark == kHalfwidthVoicedMark) {
    if (kana == kKatakanaU) {
      return kKatakanaVu;
    }
    if (isVoiceableKaToTo(kana) || isHaRow(kana)) {
      return kana + 1;
    }
  }
  if (halfwidthMark == kHalfwidthSemiVoicedMark && isHaRow(kana)) {
    return kana + 2;
  }
  return 0;
}

}  // namespace

std::string normalizeForDisplay(const std::string& utf8Text) {
  std::vector<char32_t> normalized;
  for (const char32_t codePoint : decodeUtf8(utf8Text)) {
    if (codePoint == kFullwidthTilde) {
      normalized.push_back(kWaveDash);
      continue;
    }
    if (codePoint < kHalfwidthFirst || codePoint > kHalfwidthLast) {
      normalized.push_back(codePoint);
      continue;
    }
    const char32_t merged = normalized.empty() ? 0 : mergeSoundMark(normalized.back(), codePoint);
    if (merged != 0) {
      normalized.back() = merged;
    } else {
      normalized.push_back(kFullwidthForHalfwidth[codePoint - kHalfwidthFirst]);
    }
  }
  std::string output;
  for (const char32_t codePoint : normalized) {
    appendUtf8(output, codePoint);
  }
  return output;
}
