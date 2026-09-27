#pragma once

#include <cstdint>
#include <functional>
#include <string>

// Returns the pixel width of a UTF-8 string in the current font.
using TextWidthMeasurer = std::function<int32_t(const std::string&)>;

// Returns text unchanged when it fits in maxWidthPx; otherwise cuts whole
// UTF-8 characters from the end and appends "..." so that the result fits.
std::string fitTextToWidth(const std::string& text, int32_t maxWidthPx, const TextWidthMeasurer& measureWidth);
