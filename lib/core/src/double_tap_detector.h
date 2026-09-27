#pragma once

#include <cstdint>

// Recognizes two taps in quick succession. A single tap never triggers.
class DoubleTapDetector {
 public:
  static constexpr uint32_t kMaxIntervalMs = 400;

  // Returns true when this tap completes a double tap. A third quick tap
  // starts a new pair instead of triggering again.
  bool registerTap(uint32_t nowMs);

 private:
  bool hasPendingTap_ = false;
  uint32_t pendingTapMs_ = 0;
};
