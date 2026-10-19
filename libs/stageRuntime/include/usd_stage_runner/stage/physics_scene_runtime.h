#pragma once

#include "usd_physics/core/errors.h"
#include "usd_physics/core/world.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <pxr/usd/usd/stage.h>
#include <stdexcept>
#include <string>

namespace usd_stage_runner::stage {

// Backend construction failures retain a neutral code and the declaration path.
// Snapshot/conversion errors retain physicsUsd's own ImportError code/path.
class PhysicsSceneBuildError : public std::runtime_error {
public:
  PhysicsSceneBuildError(usd_physics::core::PhysicsErrorCode code,
                        std::string path, const std::string& detail);
  [[nodiscard]] usd_physics::core::PhysicsErrorCode code() const noexcept;
  [[nodiscard]] const std::string& path() const noexcept;

private:
  usd_physics::core::PhysicsErrorCode code_;
  std::string path_;
};

// Consumer-owned full rebuild. This does not schedule steps, mutate USD or
// apply display transforms. Every rebuild first discards the previous world;
// failure leaves an empty runtime. Returned pointers/handles expire on clear,
// rebuild or destruction. The source Stage is retained only while ready.
// Callers may step, query and issue body commands through world(); resource
// creation/destruction remains owned by this component.
class PhysicsSceneRuntime {
public:
  using WorldFactory = std::function<std::unique_ptr<usd_physics::core::PhysicsWorld>(
      std::optional<usd_physics::core::Vector3> gravity)>;

  explicit PhysicsSceneRuntime(WorldFactory factory);
  ~PhysicsSceneRuntime();
  PhysicsSceneRuntime(const PhysicsSceneRuntime&) = delete;
  PhysicsSceneRuntime& operator=(const PhysicsSceneRuntime&) = delete;

  [[nodiscard]] static bool available() noexcept;
  void rebuild(const pxr::UsdStageRefPtr& stage);
  void clear() noexcept;

  [[nodiscard]] usd_physics::core::PhysicsWorld* world() noexcept;
  [[nodiscard]] const usd_physics::core::PhysicsWorld* world() const noexcept;
  [[nodiscard]] pxr::UsdStageRefPtr sourceStage() const;
  [[nodiscard]] usd_physics::core::BodyHandle bodyForPrim(const std::string& path) const;
  [[nodiscard]] std::optional<std::string>
  primForBody(usd_physics::core::BodyHandle body) const;
  [[nodiscard]] std::size_t bodyCount() const noexcept;
  [[nodiscard]] std::size_t constraintCount() const noexcept;

private:
  struct State;
  WorldFactory factory_;
  std::unique_ptr<State> state_;
};

} // namespace usd_stage_runner::stage
