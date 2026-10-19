#include "usd_stage_runner/stage/physics_scene_runtime.h"

#ifdef USD_STAGE_RUNNER_HAS_PHYSICS_SCENE
#include "usd_physics/core/six_dof.h"
#include "usd_physics/usd/box_scene.h"
#include "usd_physics/usd/joint_conversion.h"
#endif

#include <map>
#include <unordered_map>
#include <utility>

namespace usd_stage_runner::stage {
namespace physics = usd_physics::core;

PhysicsSceneBuildError::PhysicsSceneBuildError(physics::PhysicsErrorCode code,
                                             std::string path,
                                             const std::string& detail)
    : std::runtime_error(detail), code_(code), path_(std::move(path)) {}

physics::PhysicsErrorCode PhysicsSceneBuildError::code() const noexcept { return code_; }
const std::string& PhysicsSceneBuildError::path() const noexcept { return path_; }

struct PhysicsSceneRuntime::State {
  pxr::UsdStageRefPtr stage;
  std::unique_ptr<physics::PhysicsWorld> world;
  std::map<std::string, physics::BodyHandle> bodies;
  std::unordered_map<physics::BodyHandle, std::string,
                     physics::PhysicsHandleHash<physics::BodyHandle>> paths;
  std::map<std::string, physics::ConstraintHandle> constraints;
};

PhysicsSceneRuntime::PhysicsSceneRuntime(WorldFactory factory) : factory_(std::move(factory)) {
  if (!factory_)
    throw std::invalid_argument("physics scene runtime requires a world factory");
}
PhysicsSceneRuntime::~PhysicsSceneRuntime() = default;

bool PhysicsSceneRuntime::available() noexcept {
#ifdef USD_STAGE_RUNNER_HAS_PHYSICS_SCENE
  return true;
#else
  return false;
#endif
}

void PhysicsSceneRuntime::rebuild(const pxr::UsdStageRefPtr& stage) {
  clear();
#ifdef USD_STAGE_RUNNER_HAS_PHYSICS_SCENE
  namespace usd = usd_physics::usd;
  const auto snapshot = usd::readPhysicsScene(stage);
  auto next = std::make_unique<State>();
  next->stage = stage;
  std::string path = snapshot.settings ? snapshot.settings->path : "";
  try {
    next->world = factory_(snapshot.settings
                               ? std::optional<physics::Vector3>{snapshot.settings->gravity}
                               : std::nullopt);
    if (!next->world)
      throw physics::PhysicsError(physics::PhysicsErrorCode::backendUnavailable,
                                  "physics scene factory returned no world");
    for (const auto& body : snapshot.bodies) {
      path = body.path;
      const auto shape = next->world->createShape(body.shape);
      const auto handle = next->world->createBody(
          {shape, body.motionType, body.transform, body.mass});
      next->bodies.emplace(path, handle);
      next->paths.emplace(handle, path);
    }
    for (const auto& joint : snapshot.joints) {
      path = joint.path;
      const auto endpoint = [&](const std::string& body) {
        return body.empty() ? physics::BodyHandle{} : next->bodies.at(body);
      };
      const auto descriptor = usd::toSixDofConstraint(
          joint, endpoint(joint.body0), endpoint(joint.body1));
      if (!descriptor)
        continue;
      auto* constraints = dynamic_cast<physics::SixDofConstraints*>(next->world.get());
      if (!constraints)
        throw usd::ImportError(usd::ImportErrorCode::unsupportedDeclaration, path,
                               "joint execution requires SixDofConstraints");
      next->constraints.emplace(path, constraints->createSixDofConstraint(*descriptor));
    }
  } catch (const usd::ImportError&) {
    throw;
  } catch (const physics::PhysicsError& error) {
    throw PhysicsSceneBuildError(error.code(), path, error.what());
  } catch (const std::exception& error) {
    throw PhysicsSceneBuildError(physics::PhysicsErrorCode::solverFailure, path, error.what());
  }
  state_ = std::move(next);
#else
  (void)stage;
  throw std::runtime_error("physics scene rebuild requires USD_STAGE_RUNNER_ENABLE_PHYSICS_SCENE");
#endif
}

void PhysicsSceneRuntime::clear() noexcept { state_.reset(); }
physics::PhysicsWorld* PhysicsSceneRuntime::world() noexcept {
  return state_ ? state_->world.get() : nullptr;
}
const physics::PhysicsWorld* PhysicsSceneRuntime::world() const noexcept {
  return state_ ? state_->world.get() : nullptr;
}
pxr::UsdStageRefPtr PhysicsSceneRuntime::sourceStage() const {
  return state_ ? state_->stage : pxr::UsdStageRefPtr{};
}
physics::BodyHandle PhysicsSceneRuntime::bodyForPrim(const std::string& path) const {
  if (!state_)
    return {};
  const auto found = state_->bodies.find(path);
  return found == state_->bodies.end() ? physics::BodyHandle{} : found->second;
}
std::optional<std::string> PhysicsSceneRuntime::primForBody(physics::BodyHandle body) const {
  if (!state_)
    return std::nullopt;
  const auto found = state_->paths.find(body);
  return found == state_->paths.end() ? std::nullopt : std::optional<std::string>{found->second};
}
std::size_t PhysicsSceneRuntime::bodyCount() const noexcept {
  return state_ ? state_->bodies.size() : 0;
}
std::size_t PhysicsSceneRuntime::constraintCount() const noexcept {
  return state_ ? state_->constraints.size() : 0;
}
} // namespace usd_stage_runner::stage
