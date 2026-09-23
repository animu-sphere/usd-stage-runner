#include "usd_stage_runner/stage/physics_runtime.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

namespace physics = usd_physics::core;
using usd_stage_runner::runtime::PrimId;
using usd_stage_runner::runtime::RuntimeWorld;
using usd_stage_runner::stage::PhysicsRuntime;

class PhysicsWorldDouble final : public physics::PhysicsWorld {
public:
  physics::ShapeHandle createShape(const physics::ShapeDescriptor&) override {
    return physics::ShapeHandle{1};
  }
  bool destroyShape(physics::ShapeHandle) noexcept override { return true; }

  physics::BodyHandle createBody(const physics::BodyDescriptor&) override {
    const physics::BodyHandle body{nextBody_++};
    states_.emplace(body, physics::BodyState{body, {}, {}});
    return body;
  }
  bool destroyBody(physics::BodyHandle body) noexcept override {
    return states_.erase(body) != 0;
  }

  physics::ConstraintHandle
  createConstraint(const physics::ConstraintDescriptor&) override {
    return physics::ConstraintHandle{1};
  }
  bool destroyConstraint(physics::ConstraintHandle) noexcept override {
    return true;
  }

  bool applyForce(physics::BodyHandle body, physics::Vector3) override {
    return states_.find(body) != states_.end();
  }
  bool setLinearVelocity(physics::BodyHandle body,
                         physics::Vector3 velocity) override {
    const auto found = states_.find(body);
    if (found == states_.end()) {
      return false;
    }
    found->second.linearVelocity = velocity;
    return true;
  }
  physics::BodyState bodyState(physics::BodyHandle body) const override {
    const auto found = states_.find(body);
    if (found == states_.end()) {
      throw std::out_of_range("unknown body");
    }
    return found->second;
  }

  void step(Duration) override {
    for (auto& [body, state] : states_) {
      state.transform.translation.x += state.linearVelocity.x;
      changed_.push_back(state);
    }
  }
  std::vector<physics::BodyState> takeChangedBodyStates() override {
    return std::exchange(changed_, {});
  }

  void reportChanged(physics::BodyHandle body) {
    changed_.push_back(states_.at(body));
  }

private:
  physics::BodyHandle::ValueType nextBody_{1};
  std::unordered_map<physics::BodyHandle, physics::BodyState,
                     physics::PhysicsHandleHash<physics::BodyHandle>>
      states_;
  std::vector<physics::BodyState> changed_;
};

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

template <typename Exception, typename Function>
bool rejects(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  static_assert(!std::is_copy_constructible_v<PhysicsRuntime>);
  static_assert(!std::is_copy_assignable_v<PhysicsRuntime>);
  static_assert(!std::is_move_constructible_v<PhysicsRuntime>);
  static_assert(!std::is_move_assignable_v<PhysicsRuntime>);

  PhysicsWorldDouble physicsWorld;
  const auto firstBody = physicsWorld.createBody({});
  const auto secondBody = physicsWorld.createBody({});

  RuntimeWorld runtimeWorld;
  runtimeWorld.addPrim("/World/First");
  runtimeWorld.emplaceTransform("/World/First");
  runtimeWorld.addPrim("/World/Second");
  runtimeWorld.emplaceTransform("/World/Second");
  runtimeWorld.addPrim("/World/MissingTransform");

  PhysicsRuntime runtime(physicsWorld, runtimeWorld);
  if (!runtime.bindBody("/World/First", firstBody) ||
      runtime.bindBody("/World/First", firstBody) ||
      runtime.bodyForPrim("/World/First") != firstBody ||
      runtime.primForBody(firstBody) != PrimId{"/World/First"} ||
      runtime.bodyCount() != 1) {
    return fail("the bridge must maintain a stable prim/body mapping");
  }
  if (!rejects<std::invalid_argument>([&] {
        runtime.bindBody("/World/Second", firstBody);
      }) ||
      !rejects<std::out_of_range>([&] {
        runtime.bindBody("/World/MissingTransform", secondBody);
      })) {
    return fail("the bridge must reject duplicate bodies and missing transforms");
  }

  if (!physicsWorld.setLinearVelocity(firstBody, {2.0, 0.0, 0.0}) ||
      runtime.step(physics::PhysicsWorld::Duration{1.0}) != 1) {
    return fail("a fixed step must synchronize a changed mapped body");
  }
  const auto* transform = runtimeWorld.transform("/World/First");
  if (transform == nullptr || transform->translation.x != 2.0 ||
      runtimeWorld.takeDirtyTransforms() !=
          std::vector<PrimId>{"/World/First"}) {
    return fail("changed state must update and dirty only the mapped prim");
  }

  physicsWorld.reportChanged(firstBody);
  if (runtime.synchronizeChangedBodyStates() != 0 ||
      runtimeWorld.dirtyTransformCount() != 0) {
    return fail("unchanged translations must not enqueue redundant writeback");
  }

  if (!runtimeWorld.removePrim("/World/First") ||
      runtime.primForBody(firstBody).has_value() || runtime.bodyCount() != 0 ||
      !runtime.bindBody("/World/Second", firstBody) ||
      !runtime.unbindBody("/World/Second") || runtime.bodyCount() != 0) {
    return fail("stale mappings must be discarded and handles must remain reusable");
  }

  return 0;
}
