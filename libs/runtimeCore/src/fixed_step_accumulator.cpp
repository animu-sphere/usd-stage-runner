#include "usd_stage_runner/runtime/fixed_step_accumulator.h"

#include <cmath>
#include <stdexcept>

namespace usd_stage_runner::runtime {

FixedStepAccumulator::FixedStepAccumulator(Duration fixedStep, std::size_t maxStepsPerFrame)
    : fixedStep_(fixedStep), maxStepsPerFrame_(maxStepsPerFrame) {
  if (!std::isfinite(fixedStep_.count()) || fixedStep_ <= Duration::zero()) {
    throw std::invalid_argument("fixed step must be positive");
  }
  if (maxStepsPerFrame_ == 0) {
    throw std::invalid_argument("maximum steps per frame must be positive");
  }
}

FixedStepAccumulator::AdvanceResult FixedStepAccumulator::advance(Duration frameTime,
                                                                  const StepFunction& step) {
  if (!step) {
    throw std::invalid_argument("fixed-step callback must be callable");
  }
  if (!std::isfinite(frameTime.count())) {
    throw std::invalid_argument("frame time must be finite");
  }
  if (frameTime > Duration::zero()) {
    accumulator_ += frameTime;
  }

  AdvanceResult result;
  while (accumulator_ >= fixedStep_ && result.steps < maxStepsPerFrame_) {
    step(fixedStep_);
    accumulator_ -= fixedStep_;
    ++result.steps;
  }

  if (accumulator_ >= fixedStep_) {
    result.droppedSteps =
        static_cast<std::size_t>(std::floor(accumulator_.count() / fixedStep_.count()));
    accumulator_ -= fixedStep_ * static_cast<double>(result.droppedSteps);
  }

  result.interpolationAlpha = accumulator_.count() / fixedStep_.count();
  return result;
}

void FixedStepAccumulator::reset() noexcept {
  accumulator_ = Duration::zero();
}

} // namespace usd_stage_runner::runtime
