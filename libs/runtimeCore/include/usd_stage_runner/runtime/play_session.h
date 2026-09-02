#pragma once

#include "usd_stage_runner/runtime/fixed_step_accumulator.h"

#include <functional>

namespace usd_stage_runner::runtime {

class PlaySession {
public:
  using Duration = FixedStepAccumulator::Duration;
  using StepFunction = FixedStepAccumulator::StepFunction;
  using ResetFunction = std::function<void()>;
  using SynchronizeFunction = std::function<void()>;

  enum class State {
    paused,
    playing,
  };

  struct Callbacks {
    ResetFunction reset;
    StepFunction fixedStep;
    SynchronizeFunction synchronize;
  };

  explicit PlaySession(Callbacks callbacks, Duration fixedStep = Duration{1.0 / 60.0},
                       std::size_t maxStepsPerFrame = 8);

  void play() noexcept;
  void pause() noexcept;
  void stop() noexcept;
  void singleStep();
  void reset();

  [[nodiscard]] FixedStepAccumulator::AdvanceResult advance(Duration frameTime);
  [[nodiscard]] State state() const noexcept {
    return state_;
  }
  [[nodiscard]] Duration fixedStep() const noexcept {
    return accumulator_.fixedStep();
  }

private:
  Callbacks callbacks_;
  FixedStepAccumulator accumulator_;
  State state_{State::paused};
};

} // namespace usd_stage_runner::runtime
