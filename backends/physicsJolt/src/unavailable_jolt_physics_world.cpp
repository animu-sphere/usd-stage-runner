#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"

#include <stdexcept>

namespace usd_stage_runner::physics_jolt {

bool isJoltPhysicsAvailable() noexcept {
  return false;
}

std::unique_ptr<physics::PhysicsWorld> createJoltPhysicsWorld() {
  throw std::runtime_error("Jolt Physics is unavailable in this build");
}

} // namespace usd_stage_runner::physics_jolt
