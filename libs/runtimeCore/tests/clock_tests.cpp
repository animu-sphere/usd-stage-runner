#include "usd_stage_runner/runtime/fixed_step_accumulator.h"
#include "usd_stage_runner/runtime/frame_clock.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

bool near(double actual, double expected) {
  return std::abs(actual - expected) < 1e-9;
}

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

} // namespace

int main() {
  using usd_stage_runner::runtime::FixedStepAccumulator;
  using usd_stage_runner::runtime::FrameClock;

  auto now = FrameClock::Clock::time_point{};
  FrameClock clock([&] { return now; });
  if (!near(clock.tick().count(), 0.0)) {
    return fail("the first clock sample must be zero");
  }
  now += std::chrono::milliseconds(25);
  if (!near(clock.tick().count(), 0.025)) {
    return fail("the clock must use its injected time source");
  }
  now -= std::chrono::milliseconds(5);
  if (!near(clock.tick().count(), 0.0)) {
    return fail("a backwards clock must not produce a negative frame time");
  }
  clock.reset();
  if (!near(clock.tick().count(), 0.0)) {
    return fail("a reset clock must restart with a zero sample");
  }

  FixedStepAccumulator accumulator(FixedStepAccumulator::Duration{0.01}, 2);
  std::size_t calls = 0;
  const auto partial =
      accumulator.advance(FixedStepAccumulator::Duration{0.005}, [&](const auto) { ++calls; });
  if (partial.steps != 0 || !near(partial.interpolationAlpha, 0.5)) {
    return fail("a partial fixed step must be retained");
  }
  const auto bounded =
      accumulator.advance(FixedStepAccumulator::Duration{0.04}, [&](const auto dt) {
        if (!near(dt.count(), 0.01)) {
          calls = 100;
        } else {
          ++calls;
        }
      });
  if (calls != 2 || bounded.steps != 2 || bounded.droppedSteps != 2 ||
      !near(bounded.interpolationAlpha, 0.5)) {
    return fail("fixed stepping must be bounded and preserve only the fractional remainder");
  }

  try {
    FixedStepAccumulator invalid(
        FixedStepAccumulator::Duration{std::numeric_limits<double>::infinity()});
    return fail("a non-finite fixed step must be rejected");
  } catch (const std::invalid_argument&) {
  }

  try {
    (void)accumulator.advance(
        FixedStepAccumulator::Duration{std::numeric_limits<double>::quiet_NaN()},
        [&](const auto) { ++calls; });
    return fail("a non-finite frame time must be rejected");
  } catch (const std::invalid_argument&) {
  }

  return 0;
}
