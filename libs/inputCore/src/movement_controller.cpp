#include "usd_stage_runner/input/movement_controller.h"

#include <cmath>
#include <stdexcept>

namespace usd_stage_runner::input {

void updateMovementIntent(runtime::RuntimeWorld& world, const runtime::PrimId& prim,
                          const ActionState& actions) {
  world.emplaceComponent<MovementIntent>(
      prim, MovementIntent{actions.value(actions::moveX), actions.value(actions::moveY)});
}

bool applyMovementIntent(runtime::RuntimeWorld& world, const runtime::PrimId& prim,
                         double speed, double seconds) {
  if (!std::isfinite(speed) || speed < 0.0 || !std::isfinite(seconds) || seconds < 0.0) {
    throw std::invalid_argument("movement speed and timestep must be finite and non-negative");
  }

  const auto* intent = world.component<MovementIntent>(prim);
  auto* transform = world.transform(prim);
  if (intent == nullptr || transform == nullptr || seconds == 0.0 || speed == 0.0 ||
      (intent->x == 0.0 && intent->y == 0.0)) {
    return false;
  }

  transform->translation.x += intent->x * speed * seconds;
  transform->translation.z += intent->y * speed * seconds;
  world.markTransformDirty(prim);
  return true;
}

} // namespace usd_stage_runner::input
