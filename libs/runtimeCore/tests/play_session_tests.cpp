#include "usd_stage_runner/runtime/play_session.h"

#include <cmath>
#include <iostream>
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
  using usd_stage_runner::runtime::PlaySession;

  double value = 10.0;
  std::size_t resets = 0;
  std::size_t steps = 0;
  std::size_t synchronizations = 0;
  PlaySession session(
      PlaySession::Callbacks{
          [&] {
            value = 10.0;
            ++resets;
          },
          [&](const auto elapsed) {
            value += elapsed.count();
            ++steps;
          },
          [&] { ++synchronizations; },
      },
      PlaySession::Duration{0.01}, 2);

  if (session.state() != PlaySession::State::paused) {
    return fail("a play session must begin paused");
  }
  const auto paused = session.advance(PlaySession::Duration{1.0});
  if (paused.steps != 0 || steps != 0 || synchronizations != 0) {
    return fail("a paused play session must not advance or synchronize");
  }

  session.play();
  const auto partial = session.advance(PlaySession::Duration{0.005});
  if (partial.steps != 0 || !near(partial.interpolationAlpha, 0.5) ||
      synchronizations != 1) {
    return fail("a playing session must retain a partial step and synchronize its frame");
  }

  session.pause();
  const auto pausedWithRemainder = session.advance(PlaySession::Duration{1.0});
  if (pausedWithRemainder.steps != 0 ||
      !near(pausedWithRemainder.interpolationAlpha, 0.5) || steps != 0 ||
      synchronizations != 1) {
    return fail("a paused session must report but not advance its retained remainder");
  }
  session.play();
  const auto bounded = session.advance(PlaySession::Duration{0.04});
  if (bounded.steps != 2 || bounded.droppedSteps != 2 || steps != 2 ||
      synchronizations != 2 || !near(value, 10.02)) {
    return fail("play-session stepping must use the bounded accumulator");
  }

  session.pause();
  session.singleStep();
  if (steps != 3 || synchronizations != 3 || !near(value, 10.03) ||
      session.state() != PlaySession::State::paused) {
    return fail("single-step must advance and synchronize exactly once while paused");
  }

  session.play();
  try {
    session.singleStep();
    return fail("single-step while playing must be rejected");
  } catch (const std::logic_error&) {
  }

  session.reset();
  if (resets != 1 || synchronizations != 4 || !near(value, 10.0) ||
      session.state() != PlaySession::State::paused) {
    return fail("reset must pause, restore, and synchronize the session");
  }
  session.play();
  const auto afterReset = session.advance(PlaySession::Duration{0.005});
  if (afterReset.steps != 0 || !near(afterReset.interpolationAlpha, 0.5)) {
    return fail("reset must clear the accumulated frame remainder");
  }

  try {
    PlaySession invalid({{}, [](const auto) {}, [] {}});
    return fail("a missing reset callback must be rejected");
  } catch (const std::invalid_argument&) {
  }

  return 0;
}
