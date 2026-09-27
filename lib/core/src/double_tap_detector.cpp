#include "double_tap_detector.h"

bool DoubleTapDetector::registerTap(uint32_t nowMs) {
  if (hasPendingTap_ && nowMs - pendingTapMs_ <= kMaxIntervalMs) {
    hasPendingTap_ = false;
    return true;
  }
  hasPendingTap_ = true;
  pendingTapMs_ = nowMs;
  return false;
}
