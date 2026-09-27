#include "text_fitting.h"

namespace {

constexpr const char* kEllipsis = "...";

bool isUtf8ContinuationByte(char byte) { return (static_cast<unsigned char>(byte) & 0xC0) == 0x80; }

std::string removeLastUtf8Character(const std::string& text) {
  size_t cutPosition = text.size();
  while (cutPosition > 0) {
    --cutPosition;
    if (!isUtf8ContinuationByte(text[cutPosition])) {
      break;
    }
  }
  return text.substr(0, cutPosition);
}

}  // namespace

std::string fitTextToWidth(const std::string& text, int32_t maxWidthPx, const TextWidthMeasurer& measureWidth) {
  if (measureWidth(text) <= maxWidthPx) {
    return text;
  }
  std::string keptText = removeLastUtf8Character(text);
  while (!keptText.empty() && measureWidth(keptText + kEllipsis) > maxWidthPx) {
    keptText = removeLastUtf8Character(keptText);
  }
  return keptText + kEllipsis;
}
