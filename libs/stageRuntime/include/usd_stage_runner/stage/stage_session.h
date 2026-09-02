#pragma once

#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/physics/physics_body.h"
#include "usd_stage_runner/physics/physics_world.h"
#include "usd_stage_runner/runtime/play_session.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <pxr/usd/usd/stage.h>
#include <vector>

namespace usd_stage_runner::stage {

struct StageSessionConfig {
  runtime::PlaySession::Duration fixedStep{1.0 / 60.0};
  std::size_t maxFixedStepsPerFrame{8};
  runtime::PrimId playerPrim{"/World/PlayerCube"};
  double playerSpeed{3.0};
  physics::CollisionLayer staticCollisionLayer{0};
  physics::CollisionLayer dynamicCollisionLayer{1};
};

struct StageSessionStats {
  std::size_t physicsShapeCount{0};
  std::size_t physicsBodyCount{0};
  std::size_t characterControllerCount{0};
  std::size_t cameraRigCount{0};
  std::size_t fixedSteps{0};
  std::size_t droppedSteps{0};
  std::size_t physicsBodyUpdates{0};
  std::size_t cameraRigUpdates{0};
  std::size_t synchronizedTransforms{0};
  std::size_t synchronizedCameraTransforms{0};
};

class StageSession {
public:
  using Duration = runtime::PlaySession::Duration;
  using AdvanceResult = runtime::FixedStepAccumulator::AdvanceResult;
  using PhysicsWorldFactory = std::function<std::unique_ptr<physics::PhysicsWorld>()>;

  StageSession(pxr::UsdStageRefPtr stage, StageSessionConfig config = {},
               PhysicsWorldFactory physicsWorldFactory = {});
  ~StageSession();

  StageSession(const StageSession&) = delete;
  StageSession& operator=(const StageSession&) = delete;
  StageSession(StageSession&&) noexcept;
  StageSession& operator=(StageSession&&) noexcept;

  void setActions(input::ActionState actions);
  void play() noexcept;
  void pause() noexcept;
  void singleStep();
  void reset();
  void stop();
  [[nodiscard]] AdvanceResult advance(Duration frameTime);

  [[nodiscard]] runtime::PlaySession::State state() const noexcept;
  [[nodiscard]] Duration fixedStep() const noexcept;
  [[nodiscard]] const StageSessionConfig& config() const noexcept;
  [[nodiscard]] const StageSessionStats& stats() const noexcept;
  [[nodiscard]] runtime::RuntimeWorld& world() noexcept;
  [[nodiscard]] const runtime::RuntimeWorld& world() const noexcept;
  [[nodiscard]] const std::vector<runtime::PrimId>& cameraPrims() const noexcept;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace usd_stage_runner::stage
