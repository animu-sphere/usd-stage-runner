#pragma once

#include <chrono>
#include <functional>

namespace usd_stage_runner::runtime {

class FrameClock {
public:
  using Clock = std::chrono::steady_clock;
  using Duration = std::chrono::duration<double>;
  using NowFunction = std::function<Clock::time_point()>;

  explicit FrameClock(NowFunction now = [] { return Clock::now(); });

  [[nodiscard]] Duration tick();
  void reset();

private:
  NowFunction now_;
  Clock::time_point previous_{};
  bool started_{false};
};

} // namespace usd_stage_runner::runtime
