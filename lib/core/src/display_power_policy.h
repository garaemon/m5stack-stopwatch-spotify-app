#pragma once

#include <cstdint>

enum class DisplayPower { kOn, kDimmed, kOff };

// Decides how bright the display should be. Keeps the display on while
// music plays, and dims then turns it off once playback stops.
class DisplayPowerPolicy {
 public:
  static constexpr uint32_t kDimAfterIdleMs = 30 * 1000;
  static constexpr uint32_t kOffAfterIdleMs = 2 * 60 * 1000;

  // Returns the display power at nowMs; call it every loop iteration.
  DisplayPower update(uint32_t nowMs, bool isPlaying);
  // Records a button press or touch. Returns true when the display was
  // off, so the caller can treat the input as a wake-up only.
  bool registerInteraction(uint32_t nowMs);

 private:
  uint32_t lastActiveMs_ = 0;
  DisplayPower power_ = DisplayPower::kOn;
};
