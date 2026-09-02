#include "usd_stage_runner/runtime/play_session.h"

#include <stdexcept>
#include <utility>

namespace usd_stage_runner::runtime {

PlaySession::PlaySession(Callbacks callbacks, Duration fixedStep,
                         std::size_t maxStepsPerFrame)
    : callbacks_(std::move(callbacks)), accumulator_(fixedStep, maxStepsPerFrame) {
  if (!callbacks_.reset) {
    throw std::invalid_argument("play session reset callback must be callable");
  }
  if (!callbacks_.fixedStep) {
    throw std::invalid_argument("play session fixed-step callback must be callable");
  }
  if (!callbacks_.synchronize) {
    throw std::invalid_argument("play session synchronize callback must be callable");
  }
}

void PlaySession::play() noexcept {
  state_ = State::playing;
}

void PlaySession::pause() noexcept {
  state_ = State::paused;
}

void PlaySession::stop() noexcept {
  state_ = State::paused;
  accumulator_.reset();
}

void PlaySession::singleStep() {
  if (state_ != State::paused) {
    throw std::logic_error("single-step requires a paused play session");
  }
  accumulator_.reset();
  callbacks_.fixedStep(accumulator_.fixedStep());
  callbacks_.synchronize();
}

void PlaySession::reset() {
  state_ = State::paused;
  accumulator_.reset();
  callbacks_.reset();
  callbacks_.synchronize();
}

FixedStepAccumulator::AdvanceResult PlaySession::advance(Duration frameTime) {
  if (state_ != State::playing) {
    FixedStepAccumulator::AdvanceResult result;
    result.interpolationAlpha =
        accumulator_.remainder().count() / accumulator_.fixedStep().count();
    return result;
  }
  const auto result = accumulator_.advance(frameTime, callbacks_.fixedStep);
  callbacks_.synchronize();
  return result;
}

} // namespace usd_stage_runner::runtime
