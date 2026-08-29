#include "usd_stage_runner/runtime/frame_clock.h"

#include <stdexcept>
#include <utility>

namespace usd_stage_runner::runtime {

FrameClock::FrameClock(NowFunction now) : now_(std::move(now)) {
  if (!now_) {
    throw std::invalid_argument("FrameClock requires a time source");
  }
}

FrameClock::Duration FrameClock::tick() {
  const auto current = now_();
  if (!started_) {
    previous_ = current;
    started_ = true;
    return Duration::zero();
  }

  const auto elapsed = std::chrono::duration_cast<Duration>(current - previous_);
  previous_ = current;
  return elapsed < Duration::zero() ? Duration::zero() : elapsed;
}

void FrameClock::reset() {
  previous_ = Clock::time_point{};
  started_ = false;
}

} // namespace usd_stage_runner::runtime
