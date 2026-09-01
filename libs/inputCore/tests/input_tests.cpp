#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/input/movement_controller.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace {

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bool near(double left, double right) {
  return std::abs(left - right) < 1.0e-9;
}

} // namespace

int main() {
  using usd_stage_runner::input::ActionState;
  using usd_stage_runner::input::MovementIntent;
  using usd_stage_runner::input::actions::moveX;
  using usd_stage_runner::input::actions::moveY;
  using usd_stage_runner::input::actions::jump;
  using usd_stage_runner::input::applyMovementIntent;
  using usd_stage_runner::input::updateMovementIntent;
  using usd_stage_runner::runtime::RuntimeWorld;

  ActionState actions;
  actions.set(std::string(moveX), 2.0);
  actions.set(std::string(moveY), -0.25);
  actions.set(std::string(jump), 1.0);
  actions.set("invalid", std::numeric_limits<double>::quiet_NaN());
  if (actions.value(moveX) != 1.0 || actions.value(moveY) != -0.25 ||
      actions.value(jump) != 1.0 ||
      actions.value("invalid") != 0.0 || actions.value("missing") != 0.0) {
    return fail("action state must normalize injected values and default missing actions to zero");
  }

  RuntimeWorld world;
  world.addPrim("/World/PlayerCube");
  world.emplaceTransform("/World/PlayerCube", {{0.0, 1.0, 0.0}});
  updateMovementIntent(world, "/World/PlayerCube", actions);

  const auto* intent = world.component<MovementIntent>("/World/PlayerCube");
  if (intent == nullptr || intent->x != 1.0 || intent->y != -0.25) {
    return fail("move actions must produce a backend-neutral movement intent component");
  }
  if (!applyMovementIntent(world, "/World/PlayerCube", 3.0, 0.5)) {
    return fail("a non-zero movement intent must update the runtime transform");
  }
  const auto* transform = world.transform("/World/PlayerCube");
  if (!near(transform->translation.x, 1.5) || !near(transform->translation.y, 1.0) ||
      !near(transform->translation.z, -0.375) || world.dirtyTransformCount() != 1) {
    return fail("movement must update X/Z and enqueue the runtime transform for synchronization");
  }

  actions.clear();
  updateMovementIntent(world, "/World/PlayerCube", actions);
  world.takeDirtyTransforms();
  if (applyMovementIntent(world, "/World/PlayerCube", 3.0, 0.5) ||
      world.dirtyTransformCount() != 0) {
    return fail("zero movement intent must not dirty a transform");
  }

  return 0;
}
