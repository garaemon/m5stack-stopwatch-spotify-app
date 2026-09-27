#pragma once

#include <string>

// Rewrites characters that the efont Japanese font lacks into equivalents
// it has: halfwidth katakana become fullwidth (merging voiced sound marks),
// and the fullwidth tilde U+FF5E becomes the wave dash U+301C.
std::string normalizeForDisplay(const std::string& utf8Text);
