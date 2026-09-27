#include "artwork_decoder.h"

namespace {

// The gradient darkens the artwork from this row down to keep text readable.
constexpr int kShadeStartOffsetPx = 20;
constexpr int kShadeMinBrightness = 70;  // Out of 256 at the bottom edge.

// Scales one pixel of an M5Canvas buffer, which stores RGB565 byte-swapped.
uint16_t scaleSwappedRgb565(uint16_t swappedPixel, int brightness) {
  const uint16_t rgb565 = static_cast<uint16_t>((swappedPixel >> 8) | (swappedPixel << 8));
  const uint16_t red = ((rgb565 >> 11) & 0x1F) * brightness >> 8;
  const uint16_t green = ((rgb565 >> 5) & 0x3F) * brightness >> 8;
  const uint16_t blue = (rgb565 & 0x1F) * brightness >> 8;
  const uint16_t scaledRgb565 = static_cast<uint16_t>((red << 11) | (green << 5) | blue);
  return static_cast<uint16_t>((scaledRgb565 >> 8) | (scaledRgb565 << 8));
}

void shadeBottomHalf(M5Canvas& canvas) {
  auto* pixels = static_cast<uint16_t*>(canvas.getBuffer());
  const int32_t width = canvas.width();
  const int32_t height = canvas.height();
  const int32_t shadeStartY = height / 2 + kShadeStartOffsetPx;
  for (int32_t y = shadeStartY; y < height; ++y) {
    const int brightness = 256 - (256 - kShadeMinBrightness) * (y - shadeStartY) / (height - shadeStartY);
    uint16_t* row = pixels + y * width;
    for (int32_t x = 0; x < width; ++x) {
      row[x] = scaleSwappedRgb565(row[x], brightness);
    }
  }
}

}  // namespace

ArtworkImage decodeArtwork(const std::string& jpegBytes) {
  if (jpegBytes.empty()) {
    return nullptr;
  }
  auto canvas = std::make_shared<M5Canvas>(&M5.Display);
  canvas->setPsram(true);
  canvas->setColorDepth(16);
  if (canvas->createSprite(M5.Display.width(), M5.Display.height()) == nullptr) {
    return nullptr;
  }
  canvas->fillScreen(TFT_BLACK);
  const auto* jpegData = reinterpret_cast<const uint8_t*>(jpegBytes.data());
  // A zero scale makes M5GFX fit the image to the canvas, keeping its aspect.
  canvas->drawJpg(jpegData, jpegBytes.size(), 0, 0, canvas->width(), canvas->height(), 0, 0, 0.0f, 0.0f,
                  datum_t::middle_center);
  shadeBottomHalf(*canvas);
  return canvas;
}
