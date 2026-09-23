#pragma once

#include "usd_physics/core/handles.h"
#include "usd_physics/core/world.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#include <cstddef>
#include <optional>
#include <unordered_map>

namespace usd_stage_runner::stage {

struct PhysicsBody {
  usd_physics::core::BodyHandle handle;
};

class PhysicsRuntime {
public:
  PhysicsRuntime(usd_physics::core::PhysicsWorld& physicsWorld,
                 runtime::RuntimeWorld& runtimeWorld) noexcept;
  PhysicsRuntime(const PhysicsRuntime&) = delete;
  PhysicsRuntime& operator=(const PhysicsRuntime&) = delete;
  PhysicsRuntime(PhysicsRuntime&&) = delete;
  PhysicsRuntime& operator=(PhysicsRuntime&&) = delete;

  bool bindBody(const runtime::PrimId& prim, usd_physics::core::BodyHandle body);
  bool unbindBody(const runtime::PrimId& prim) noexcept;

  [[nodiscard]] usd_physics::core::BodyHandle
  bodyForPrim(const runtime::PrimId& prim) const noexcept;
  [[nodiscard]] std::optional<runtime::PrimId>
  primForBody(usd_physics::core::BodyHandle body) const;
  [[nodiscard]] std::size_t bodyCount() const noexcept;

  std::size_t step(usd_physics::core::PhysicsWorld::Duration fixedStep);
  std::size_t synchronizeChangedBodyStates();

private:
  [[nodiscard]] bool
  isMappingCurrent(usd_physics::core::BodyHandle body,
                   const runtime::PrimId& prim) const noexcept;

  usd_physics::core::PhysicsWorld& physicsWorld_;
  runtime::RuntimeWorld& runtimeWorld_;
  mutable std::unordered_map<
      usd_physics::core::BodyHandle, runtime::PrimId,
      usd_physics::core::PhysicsHandleHash<usd_physics::core::BodyHandle>>
      bodyToPrim_;
};

} // namespace usd_stage_runner::stage
