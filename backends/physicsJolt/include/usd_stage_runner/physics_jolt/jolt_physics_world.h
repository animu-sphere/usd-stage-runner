#pragma once

#include "usd_stage_runner/physics/physics_world.h"

#include <memory>

namespace usd_stage_runner::physics_jolt {

[[nodiscard]] bool isJoltPhysicsAvailable() noexcept;
[[nodiscard]] std::unique_ptr<physics::PhysicsWorld> createJoltPhysicsWorld();

} // namespace usd_stage_runner::physics_jolt
