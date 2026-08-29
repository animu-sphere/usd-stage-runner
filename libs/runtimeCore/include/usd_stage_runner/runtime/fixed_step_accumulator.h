#pragma once

#include <chrono>
#include <cstddef>
#include <functional>

namespace usd_stage_runner::runtime {

class FixedStepAccumulator {
public:
  using Duration = std::chrono::duration<double>;
  using StepFunction = std::function<void(Duration)>;

  struct AdvanceResult {
    std::size_t steps{0};
    std::size_t droppedSteps{0};
    double interpolationAlpha{0.0};
  };

  explicit FixedStepAccumulator(Duration fixedStep = Duration{1.0 / 60.0},
                                std::size_t maxStepsPerFrame = 8);

  [[nodiscard]] AdvanceResult advance(Duration frameTime, const StepFunction& step);
  void reset() noexcept;

  [[nodiscard]] Duration fixedStep() const noexcept {
    return fixedStep_;
  }
  [[nodiscard]] Duration remainder() const noexcept {
    return accumulator_;
  }
  [[nodiscard]] std::size_t maxStepsPerFrame() const noexcept {
    return maxStepsPerFrame_;
  }

private:
  Duration fixedStep_;
  Duration accumulator_{Duration::zero()};
  std::size_t maxStepsPerFrame_;
};

} // namespace usd_stage_runner::runtime
