#include "usd_stage_runner/stage/physics_scene_runtime.h"
#include "usd_physics/jolt/availability.h"
#include "usd_physics/core/six_dof.h"
#include "usd_physics/usd/box_scene.h"

#include <cmath>
#include <iostream>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/fixedJoint.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <pxr/usd/usdPhysics/scene.h>

namespace {
namespace physics = usd_physics::core;
namespace usd = usd_physics::usd;
using usd_stage_runner::stage::PhysicsSceneRuntime;
using usd_stage_runner::stage::PhysicsSceneBuildError;

void check(bool condition, const char* message) {
  if (!condition)
    throw std::runtime_error(message);
}

pxr::UsdStageRefPtr fixture(bool jointEnabled = false) {
  auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1);
  auto floor = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/Floor"));
  floor.AddTranslateOp().Set(pxr::GfVec3d(0, -.5, 0));
  floor.AddScaleOp().Set(pxr::GfVec3f(10, .5, 10));
  pxr::UsdPhysicsCollisionAPI::Apply(floor.GetPrim());
  for (const auto& name : {"/Box", "/Held"}) {
    auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath(name));
    cube.CreateSizeAttr().Set(1.0);
    cube.AddTranslateOp().Set(pxr::GfVec3d(name == std::string("/Box") ? 0 : 3, 4, 0));
    pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
    pxr::UsdPhysicsRigidBodyAPI::Apply(cube.GetPrim());
  }
  auto settings = pxr::UsdPhysicsScene::Define(stage, pxr::SdfPath("/Scene"));
  settings.CreateGravityMagnitudeAttr().Set(10.0f);
  auto joint = pxr::UsdPhysicsFixedJoint::Define(stage, pxr::SdfPath("/Joint"));
  joint.CreateBody1Rel().SetTargets({pxr::SdfPath("/Held")});
  joint.CreateLocalPos0Attr().Set(pxr::GfVec3f(3, 4, 0));
  joint.CreateJointEnabledAttr().Set(jointEnabled);
  return stage;
}

std::unique_ptr<physics::PhysicsWorld> factory(std::optional<physics::Vector3> gravity) {
  return gravity ? usd_physics::jolt::createWorld({}, *gravity)
                 : usd_physics::jolt::createWorld();
}

struct Tracking {
  int alive{0};
  int shapes{0};
  int bodies{0};
  int failShape{0};
  int failBody{0};
  int constraints{0};
};

// Real backend ownership with deterministic failure injection. Deliberately
// omits the optional constraint interface to exercise capability rejection.
class FaultWorld : public physics::PhysicsWorld {
public:
  FaultWorld(Tracking& tracking, std::optional<physics::Vector3> gravity)
      : tracking_(tracking), inner_(factory(gravity)) { ++tracking_.alive; }
  ~FaultWorld() override { --tracking_.alive; }
  physics::ShapeHandle createShape(const physics::ShapeDescriptor& descriptor) override {
    if (++tracking_.shapes == tracking_.failShape)
      throw physics::PhysicsError(physics::PhysicsErrorCode::resourceExhausted, "injected shape failure");
    return inner_->createShape(descriptor);
  }
  physics::BodyHandle createBody(const physics::BodyDescriptor& descriptor) override {
    if (++tracking_.bodies == tracking_.failBody)
      throw physics::PhysicsError(physics::PhysicsErrorCode::resourceExhausted, "injected body failure");
    return inner_->createBody(descriptor);
  }
  bool destroyShape(physics::ShapeHandle handle) noexcept override { return inner_->destroyShape(handle); }
  bool destroyBody(physics::BodyHandle handle) noexcept override { return inner_->destroyBody(handle); }
  physics::ConstraintHandle createConstraint(const physics::ConstraintDescriptor& descriptor) override {
    return inner_->createConstraint(descriptor);
  }
  bool destroyConstraint(physics::ConstraintHandle handle) noexcept override { return inner_->destroyConstraint(handle); }
  bool applyForce(physics::BodyHandle handle, physics::Vector3 force) override { return inner_->applyForce(handle, force); }
  bool setLinearVelocity(physics::BodyHandle handle, physics::Vector3 velocity) override {
    return inner_->setLinearVelocity(handle, velocity);
  }
  physics::BodyState bodyState(physics::BodyHandle handle) const override { return inner_->bodyState(handle); }
  void step(Duration dt) override { inner_->step(dt); }
  std::vector<physics::BodyState> takeChangedBodyStates() override { return inner_->takeChangedBodyStates(); }
protected:
  Tracking& tracking_;
  std::unique_ptr<physics::PhysicsWorld> inner_;
};

#ifdef TEST_SIX_DOF
class ConstraintFaultWorld final : public FaultWorld, public physics::SixDofConstraints {
public:
  using FaultWorld::FaultWorld;
  physics::ConstraintHandle createSixDofConstraint(
      const physics::SixDofConstraintDescriptor& descriptor) override {
    if (++tracking_.constraints == 2)
      throw physics::PhysicsError(physics::PhysicsErrorCode::resourceExhausted,
                                  "injected constraint failure");
    return dynamic_cast<physics::SixDofConstraints&>(*inner_).createSixDofConstraint(descriptor);
  }
};
#endif

void empty(const PhysicsSceneRuntime& runtime) {
  check(!runtime.world() && !runtime.sourceStage() && runtime.bodyCount() == 0 &&
            runtime.constraintCount() == 0 && !runtime.bodyForPrim("/Box"),
        "failure must expose no partial world or mappings");
}
} // namespace

int main() {
  try {
    check(PhysicsSceneRuntime::available(), "scene rebuild must be enabled");
    auto stage = fixture();
    std::string before, after;
    stage->GetRootLayer()->ExportToString(&before);
    std::optional<physics::Vector3> seenGravity;
    PhysicsSceneRuntime runtime([&](auto gravity) {
      seenGravity = gravity;
      return factory(gravity);
    });
    empty(runtime);
    runtime.rebuild(stage);
    check(runtime.sourceStage() == stage && runtime.bodyCount() == 3 &&
              runtime.constraintCount() == 0, "body import and disabled joint");
    const auto old = runtime.bodyForPrim("/Box");
    check(runtime.primForBody(old) == std::optional<std::string>{"/Box"} &&
              !runtime.bodyForPrim("/Missing") && !runtime.primForBody({}), "bidirectional mapping");
    for (int i = 0; i != 240; ++i)
      runtime.world()->step(physics::PhysicsWorld::Duration{1.0 / 60});
    const auto rest = runtime.world()->bodyState(old);
    check(std::abs(rest.transform.translation.y - .5) < .03 &&
              std::abs(rest.linearVelocity.y) < .05, "floor contact and rest");
    stage->GetRootLayer()->ExportToString(&after);
    check(before == after && stage->GetSessionLayer()->IsEmpty(), "rebuild and stepping must not author USD");

    auto replacement = fixture();
    replacement->RemovePrim(pxr::SdfPath("/Held"));
    replacement->RemovePrim(pxr::SdfPath("/Joint"));
    pxr::UsdPhysicsScene(replacement->GetPrimAtPath(pxr::SdfPath("/Scene")))
        .CreateGravityMagnitudeAttr().Set(0.0f);
    runtime.rebuild(replacement);
    const auto current = runtime.bodyForPrim("/Box");
    check(runtime.sourceStage() == replacement && current != old &&
              !runtime.primForBody(old) && !runtime.bodyForPrim("/Held") &&
              runtime.bodyCount() == 2, "reload discards prior Stage mappings");
    bool rejected = false;
    try { (void)runtime.world()->bodyState(old); }
    catch (const std::out_of_range&) { rejected = true; }
    check(rejected, "previous world handles must reject");
    runtime.world()->step(physics::PhysicsWorld::Duration{.1});
    check(runtime.world()->bodyState(current).transform.translation.y == 4, "reloaded zero gravity");
    auto settings = pxr::UsdPhysicsScene(replacement->GetPrimAtPath(pxr::SdfPath("/Scene")));
    settings.CreateGravityDirectionAttr().Set(pxr::GfVec3f(0, 1, 0));
    settings.CreateGravityMagnitudeAttr().Set(10.0f);
    runtime.world()->step(physics::PhysicsWorld::Duration{.1});
    check(runtime.world()->bodyState(current).transform.translation.y == 4,
          "authored edits do not mutate an existing world");
    runtime.rebuild(replacement);
    runtime.world()->step(physics::PhysicsWorld::Duration{.1});
    check(runtime.world()->bodyState(runtime.bodyForPrim("/Box")).linearVelocity.y > .99,
          "explicit rebuild consumes changed gravity");

    // Complete snapshot rejection happens before invoking a backend factory.
    int created = 0;
    PhysicsSceneRuntime invalid([&](auto gravity) { ++created; return factory(gravity); });
    auto ambiguous = fixture();
    pxr::UsdPhysicsScene::Define(ambiguous, pxr::SdfPath("/SecondScene"));
    try { invalid.rebuild(ambiguous); }
    catch (const usd::ImportError& error) {
      check(error.code() == usd::ImportErrorCode::unsupportedDeclaration &&
                error.path() == "/SecondScene", "snapshot code/path");
    }
    check(created == 0, "validate snapshot before creating world");
    empty(invalid);
    try { runtime.rebuild({}); }
    catch (const usd::ImportError& error) {
      check(error.code() == usd::ImportErrorCode::invalidStage, "invalid reload code");
    }
    empty(runtime);
    auto noScene = fixture();
    noScene->RemovePrim(pxr::SdfPath("/Scene"));
    runtime.rebuild(noScene);
    check(!seenGravity, "absent scene delegates defaults to the consumer factory");
    runtime.clear();

    for (int failure = 0; failure != 3; ++failure) {
      Tracking tracking;
      PhysicsSceneRuntime injected([&](auto gravity) {
        return std::make_unique<FaultWorld>(tracking, gravity);
      });
      injected.rebuild(fixture());
      check(tracking.alive == 1, "published world lifetime");
      tracking.shapes = tracking.bodies = 0;
      tracking.failShape = failure == 0 ? 2 : 0;
      tracking.failBody = failure == 1 ? 2 : 0;
      rejected = false;
      try { injected.rebuild(fixture(failure == 2)); }
      catch (const PhysicsSceneBuildError& error) {
        rejected = error.code() == physics::PhysicsErrorCode::resourceExhausted && error.path() == "/Floor";
      } catch (const usd::ImportError& error) {
        rejected = failure == 2 && error.code() == usd::ImportErrorCode::unsupportedDeclaration &&
                   error.path() == "/Joint";
      }
      check(rejected && tracking.alive == 0, "old and partial worlds destroyed on failure");
      empty(injected);
      tracking.failShape = tracking.failBody = 0;
      injected.rebuild(fixture());
      injected.clear();
      injected.clear();
      check(tracking.alive == 0, "clear and recovery after failed rebuild");
    }
    PhysicsSceneRuntime unavailable([](auto) { return std::unique_ptr<physics::PhysicsWorld>{}; });
    rejected = false;
    try { unavailable.rebuild(stage); }
    catch (const PhysicsSceneBuildError& error) {
      rejected = error.code() == physics::PhysicsErrorCode::backendUnavailable && error.path() == "/Scene";
    }
    check(rejected, "unavailable backend code/path");
    empty(unavailable);

#ifdef TEST_SIX_DOF
    runtime.rebuild(fixture(true));
    check(runtime.constraintCount() == 1, "fixed joint execution");
    const auto held = runtime.bodyForPrim("/Held");
    for (int i = 0; i != 180; ++i)
      runtime.world()->step(physics::PhysicsWorld::Duration{1.0 / 60});
    check(std::abs(runtime.world()->bodyState(held).transform.translation.y - 4) < .01,
          "world-anchored body stays fixed");
    runtime.rebuild(fixture(true));
    check(!runtime.primForBody(held) && runtime.constraintCount() == 1, "joint rebuild");

    auto twoJoints = fixture(true);
    auto second = pxr::UsdPhysicsFixedJoint::Define(twoJoints, pxr::SdfPath("/Joint2"));
    second.CreateBody1Rel().SetTargets({pxr::SdfPath("/Box")});
    second.CreateLocalPos0Attr().Set(pxr::GfVec3f(0, 4, 0));
    Tracking tracking;
    PhysicsSceneRuntime partialConstraints([&](auto gravity) {
      return std::make_unique<ConstraintFaultWorld>(tracking, gravity);
    });
    rejected = false;
    try { partialConstraints.rebuild(twoJoints); }
    catch (const PhysicsSceneBuildError& error) {
      rejected = error.code() == physics::PhysicsErrorCode::resourceExhausted && error.path() == "/Joint2";
    }
    check(rejected && tracking.constraints == 2 && tracking.alive == 0,
          "partial constraints and bodies cleaned up on failure");
    empty(partialConstraints);
#endif
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
