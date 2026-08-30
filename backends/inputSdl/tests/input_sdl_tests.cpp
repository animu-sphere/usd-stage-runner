#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/input_sdl/physical_input.h"

#include <iostream>

int main() {
  usd_stage_runner::input::ActionState actions;
  usd_stage_runner::input_sdl::PhysicalInputState physical;
  physical.moveLeft = true;
  physical.moveForward = true;
  physical.gamepadMoveX = 0.4;
  usd_stage_runner::input_sdl::mapPhysicalInput(physical, actions);

  if (actions.value(usd_stage_runner::input::actions::moveX) != -1.0 ||
      actions.value(usd_stage_runner::input::actions::moveY) != 1.0) {
    std::cerr << "keyboard bindings must normalize to move.x and move.y actions\n";
    return 1;
  }

  physical = {};
  physical.gamepadMoveX = 0.75;
  physical.gamepadMoveY = -0.5;
  usd_stage_runner::input_sdl::mapPhysicalInput(physical, actions);
  if (actions.value(usd_stage_runner::input::actions::moveX) != 0.75 ||
      actions.value(usd_stage_runner::input::actions::moveY) != -0.5) {
    std::cerr << "gamepad axes must normalize to move.x and move.y actions\n";
    return 1;
  }
  return 0;
}
