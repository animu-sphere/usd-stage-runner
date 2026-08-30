#pragma once

#include "usd_stage_runner/input/action_state.h"

namespace usd_stage_runner::input {

class InputSource {
public:
  virtual ~InputSource() = default;
  virtual bool poll(ActionState& actions) = 0;
};

} // namespace usd_stage_runner::input
