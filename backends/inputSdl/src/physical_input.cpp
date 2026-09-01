#include "usd_stage_runner/input_sdl/physical_input.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace usd_stage_runner::input_sdl {
namespace {

double strongest(double first, double second) {
  const double selected = std::abs(first) >= std::abs(second) ? first : second;
  return std::clamp(selected, -1.0, 1.0);
}

} // namespace

void mapPhysicalInput(const PhysicalInputState& physical, input::ActionState& actions) {
  const double keyboardX = static_cast<double>(physical.moveRight) -
                           static_cast<double>(physical.moveLeft);
  const double keyboardY = static_cast<double>(physical.moveForward) -
                           static_cast<double>(physical.moveBackward);
  actions.set(std::string(input::actions::moveX), strongest(keyboardX, physical.gamepadMoveX));
  actions.set(std::string(input::actions::moveY), strongest(keyboardY, physical.gamepadMoveY));
  actions.set(std::string(input::actions::jump),
              physical.keyboardJump || physical.gamepadJump ? 1.0 : 0.0);
}

} // namespace usd_stage_runner::input_sdl
