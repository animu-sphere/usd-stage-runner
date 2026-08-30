#pragma once

#include "usd_stage_runner/physics/physics_world.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#include <cstddef>
#include <optional>
#include <unordered_map>

namespace usd_stage_runner::physics {

class PhysicsRuntime {
public:
  PhysicsRuntime(PhysicsWorld& physicsWorld, runtime::RuntimeWorld& runtimeWorld) noexcept;

  bool bindBody(const runtime::PrimId& prim, BodyHandle body);
  bool unbindBody(const runtime::PrimId& prim) noexcept;

  [[nodiscard]] BodyHandle bodyForPrim(const runtime::PrimId& prim) const noexcept;
  [[nodiscard]] std::optional<runtime::PrimId> primForBody(BodyHandle body) const;
  [[nodiscard]] std::size_t bodyCount() const noexcept {
    return bodyToPrim_.size();
  }

  std::size_t step(PhysicsWorld::Duration fixedStep);
  std::size_t synchronizeChangedBodyStates();

private:
  PhysicsWorld& physicsWorld_;
  runtime::RuntimeWorld& runtimeWorld_;
  std::unordered_map<BodyHandle, runtime::PrimId, PhysicsHandleHash<BodyHandle>> bodyToPrim_;
};

} // namespace usd_stage_runner::physics
