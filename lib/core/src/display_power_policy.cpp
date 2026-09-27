#include "display_power_policy.h"

DisplayPower DisplayPowerPolicy::update(uint32_t nowMs, bool isPlaying) {
  if (isPlaying) {
    lastActiveMs_ = nowMs;
  }
  const uint32_t idleMs = nowMs - lastActiveMs_;
  if (idleMs >= kOffAfterIdleMs) {
    power_ = DisplayPower::kOff;
  } else if (idleMs >= kDimAfterIdleMs) {
    power_ = DisplayPower::kDimmed;
  } else {
    power_ = DisplayPower::kOn;
  }
  return power_;
}

bool DisplayPowerPolicy::registerInteraction(uint32_t nowMs) {
  const bool wasOff = power_ == DisplayPower::kOff;
  lastActiveMs_ = nowMs;
  power_ = DisplayPower::kOn;
  return wasOff;
}
