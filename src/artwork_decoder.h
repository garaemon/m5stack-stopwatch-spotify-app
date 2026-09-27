#pragma once

#include <M5Unified.h>

#include <memory>
#include <string>

// Decoded album art, sized to the display. Never draw into one after it is
// published: the network task and the loop task share it without a lock.
// Not const because M5Canvas::pushSprite() is a non-const member.
using ArtworkImage = std::shared_ptr<M5Canvas>;

// Decodes a JPEG into a PSRAM canvas that fills the display, darkening the
// lower half so that overlaid text stays readable. Takes about 0.5 s.
// Returns nullptr when the JPEG is empty or the canvas cannot be allocated.
ArtworkImage decodeArtwork(const std::string& jpegBytes);
