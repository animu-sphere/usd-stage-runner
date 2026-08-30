#pragma once

#include "usd_stage_runner/input/action_state.h"

namespace usd_stage_runner::input_sdl {

struct PhysicalInputState {
  bool moveLeft{false};
  bool moveRight{false};
  bool moveForward{false};
  bool moveBackward{false};
  double gamepadMoveX{0.0};
  double gamepadMoveY{0.0};
};

void mapPhysicalInput(const PhysicalInputState& physical, input::ActionState& actions);

} // namespace usd_stage_runner::input_sdl
