#include "usd_stage_runner/physics/collision_query.h"
#include "usd_stage_runner/physics/ground_query.h"
#include "usd_stage_runner/physics/physics_runtime.h"
#include "usd_stage_runner/physics/physics_world.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using namespace usd_stage_runner;

class DeterministicPhysicsWorld final : public physics::PhysicsWorld {
public:
  physics::ShapeHandle createShape(const physics::ShapeDescriptor& descriptor) override {
    physics::validateShapeDescriptor(descriptor);
    const physics::ShapeHandle handle{nextShape_++};
    shapes_.emplace(handle, descriptor);
    return handle;
  }

  bool destroyShape(physics::ShapeHandle shape) noexcept override {
    for (const auto& entry : bodies_) {
      if (entry.second.descriptor.shape == shape) {
        return false;
      }
    }
    return shapes_.erase(shape) != 0;
  }

  physics::BodyHandle createBody(const physics::BodyDescriptor& descriptor) override {
    physics::validateBodyDescriptor(descriptor);
    if (shapes_.find(descriptor.shape) == shapes_.end()) {
      throw std::invalid_argument("body references an unknown shape");
    }
    const physics::BodyHandle handle{nextBody_++};
    bodies_.emplace(handle,
                    BodyRecord{descriptor, {handle, descriptor.initialTransform, {}}, {}});
    return handle;
  }

  bool destroyBody(physics::BodyHandle body) noexcept override {
    for (auto iterator = constraints_.begin(); iterator != constraints_.end();) {
      if (iterator->second.firstBody == body || iterator->second.secondBody == body) {
        iterator = constraints_.erase(iterator);
      } else {
        ++iterator;
      }
    }
    changed_.erase(body);
    return bodies_.erase(body) != 0;
  }

  physics::ConstraintHandle
  createConstraint(const physics::ConstraintDescriptor& descriptor) override {
    physics::validateConstraintDescriptor(descriptor);
    if (bodies_.find(descriptor.firstBody) == bodies_.end() ||
        bodies_.find(descriptor.secondBody) == bodies_.end()) {
      throw std::invalid_argument("constraint references an unknown body");
    }
    const physics::ConstraintHandle handle{nextConstraint_++};
    constraints_.emplace(handle, descriptor);
    return handle;
  }

  bool destroyConstraint(physics::ConstraintHandle constraint) noexcept override {
    return constraints_.erase(constraint) != 0;
  }

  bool applyForce(physics::BodyHandle body, runtime::Vec3d force) override {
    physics::validatePhysicsVector(force, "force");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() ||
        found->second.descriptor.motionType != physics::MotionType::dynamicBody) {
      return false;
    }
    found->second.accumulatedForce.x += force.x;
    found->second.accumulatedForce.y += force.y;
    found->second.accumulatedForce.z += force.z;
    return true;
  }

  bool setLinearVelocity(physics::BodyHandle body, runtime::Vec3d velocity) override {
    physics::validatePhysicsVector(velocity, "linear velocity");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() ||
        found->second.descriptor.motionType != physics::MotionType::dynamicBody) {
      return false;
    }
    found->second.state.linearVelocity = velocity;
    return true;
  }

  physics::BodyState bodyState(physics::BodyHandle body) const override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      throw std::out_of_range("unknown body handle");
    }
    return found->second.state;
  }

  void step(Duration fixedStep) override {
    physics::validatePhysicsStep(fixedStep);
    const double seconds = fixedStep.count();
    for (auto& entry : bodies_) {
      auto& record = entry.second;
      if (record.descriptor.motionType != physics::MotionType::dynamicBody) {
        continue;
      }
      auto& velocity = record.state.linearVelocity;
      velocity.x += record.accumulatedForce.x / record.descriptor.mass * seconds;
      velocity.y += (-10.0 + record.accumulatedForce.y / record.descriptor.mass) * seconds;
      velocity.z += record.accumulatedForce.z / record.descriptor.mass * seconds;
      auto& translation = record.state.transform.translation;
      translation.x += velocity.x * seconds;
      translation.y += velocity.y * seconds;
      translation.z += velocity.z * seconds;
      record.accumulatedForce = {};
      changed_.insert(entry.first);
    }
  }

  std::vector<physics::BodyState> takeChangedBodyStates() override {
    std::vector<physics::BodyState> result;
    result.reserve(changed_.size());
    for (const auto body : changed_) {
      result.push_back(bodies_.at(body).state);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
      return left.body.value() < right.body.value();
    });
    changed_.clear();
    return result;
  }

  void reportBodyChanged(physics::BodyHandle body) {
    if (bodies_.find(body) == bodies_.end()) {
      throw std::out_of_range("unknown body handle");
    }
    changed_.insert(body);
  }

private:
  struct BodyRecord {
    physics::BodyDescriptor descriptor;
    physics::BodyState state;
    runtime::Vec3d accumulatedForce;
  };

  template <typename Handle, typename Value>
  using HandleMap = std::unordered_map<Handle, Value, physics::PhysicsHandleHash<Handle>>;
  template <typename Handle>
  using HandleSet = std::unordered_set<Handle, physics::PhysicsHandleHash<Handle>>;

  physics::ShapeHandle::ValueType nextShape_{1};
  physics::BodyHandle::ValueType nextBody_{1};
  physics::ConstraintHandle::ValueType nextConstraint_{1};
  HandleMap<physics::ShapeHandle, physics::ShapeDescriptor> shapes_;
  HandleMap<physics::BodyHandle, BodyRecord> bodies_;
  HandleMap<physics::ConstraintHandle, physics::ConstraintDescriptor> constraints_;
  HandleSet<physics::BodyHandle> changed_;
};

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bool near(double left, double right) {
  return std::abs(left - right) < 1.0e-9;
}

template <typename Function> bool rejectsInvalidArgument(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::invalid_argument&) {
    return true;
  }
  return false;
}

template <typename Function> bool rejectsOutOfRange(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::out_of_range&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  using namespace usd_stage_runner;

  static_assert(!std::is_copy_constructible_v<physics::PhysicsRuntime>);
  static_assert(!std::is_copy_assignable_v<physics::PhysicsRuntime>);
  static_assert(!std::is_move_constructible_v<physics::PhysicsRuntime>);
  static_assert(!std::is_move_assignable_v<physics::PhysicsRuntime>);

  DeterministicPhysicsWorld world;
  const auto box = world.createShape({physics::ShapeType::box, {0.5, 0.5, 0.5}});
  const auto floor = world.createBody(physics::BodyDescriptor{
      box, physics::MotionType::staticBody, runtime::RuntimeTransform{{0.0, -0.5, 0.0}},
      0.0, 1});
  const auto cube = world.createBody(physics::BodyDescriptor{
      box, physics::MotionType::dynamicBody, runtime::RuntimeTransform{{0.0, 10.0, 0.0}},
      2.0, 2});
  if (!box || !floor || !cube || floor == cube) {
    return fail("physics resources must receive valid typed handles");
  }

  if (!world.applyForce(cube, {4.0, 0.0, 0.0}) ||
      world.applyForce(floor, {4.0, 0.0, 0.0})) {
    return fail("force commands must apply only to known dynamic bodies");
  }
  world.step(physics::PhysicsWorld::Duration{0.5});
  const auto changed = world.takeChangedBodyStates();
  if (changed.size() != 1 || changed.front().body != cube ||
      !near(changed.front().linearVelocity.x, 1.0) ||
      !near(changed.front().linearVelocity.y, -5.0) ||
      !near(changed.front().transform.translation.x, 0.5) ||
      !near(changed.front().transform.translation.y, 7.5)) {
    return fail("a fixed mock step must deterministically expose changed body state");
  }
  if (!world.takeChangedBodyStates().empty()) {
    return fail("taking changed body states must drain the extraction queue");
  }
  if (!near(world.bodyState(floor).transform.translation.y, -0.5)) {
    return fail("static bodies must not move during a fixed step");
  }

  runtime::RuntimeWorld runtimeWorld;
  runtimeWorld.addPrim("/World/PlayerCube");
  runtimeWorld.emplaceTransform("/World/PlayerCube", world.bodyState(cube).transform);
  runtimeWorld.addPrim("/World/OtherCube");
  runtimeWorld.emplaceTransform("/World/OtherCube");
  runtimeWorld.addPrim("/World/MissingTransform");

  physics::PhysicsRuntime physicsRuntime(world, runtimeWorld);
  if (!physicsRuntime.bindBody("/World/PlayerCube", cube) ||
      physicsRuntime.bindBody("/World/PlayerCube", cube) ||
      physicsRuntime.bodyForPrim("/World/PlayerCube") != cube ||
      physicsRuntime.primForBody(cube) != runtime::PrimId{"/World/PlayerCube"} ||
      physicsRuntime.bodyCount() != 1) {
    return fail("the physics runtime must maintain a stable prim-to-body mapping");
  }
  if (!rejectsInvalidArgument([&] {
        physicsRuntime.bindBody("/World/OtherCube", cube);
      })) {
    return fail("a physics body must not be bound to more than one prim");
  }
  if (!rejectsOutOfRange([&] {
        physicsRuntime.bindBody("/World/MissingTransform", cube);
      })) {
    return fail("physics bodies must be bound only to prims with runtime transforms");
  }

  world.applyForce(cube, {4.0, 0.0, 0.0});
  if (physicsRuntime.step(physics::PhysicsWorld::Duration{0.5}) != 1) {
    return fail("a physics step must synchronize changed mapped bodies");
  }
  const auto* simulatedTransform = runtimeWorld.transform("/World/PlayerCube");
  if (simulatedTransform == nullptr || !near(simulatedTransform->translation.x, 1.5) ||
      !near(simulatedTransform->translation.y, 2.5) ||
      runtimeWorld.takeDirtyTransforms() !=
          std::vector<runtime::PrimId>{"/World/PlayerCube"}) {
    return fail("changed body transforms must update and dirty only their mapped prim");
  }
  if (physicsRuntime.synchronizeChangedBodyStates() != 0 ||
      runtimeWorld.dirtyTransformCount() != 0) {
    return fail("physics transform extraction and runtime dirty queues must drain");
  }
  world.reportBodyChanged(cube);
  if (physicsRuntime.synchronizeChangedBodyStates() != 0 ||
      runtimeWorld.dirtyTransformCount() != 0) {
    return fail("unchanged body transforms must not be dirtied for USD synchronization");
  }
  if (!runtimeWorld.removePrim("/World/PlayerCube") || physicsRuntime.primForBody(cube) ||
      physicsRuntime.bodyCount() != 0) {
    return fail("removed prims must be discarded from physics body mappings immediately");
  }

  runtimeWorld.addPrim("/World/PlayerCube");
  runtimeWorld.emplaceTransform("/World/PlayerCube", world.bodyState(floor).transform);
  if (!physicsRuntime.bindBody("/World/PlayerCube", floor) ||
      !runtimeWorld.removePrim("/World/PlayerCube") ||
      !physicsRuntime.bindBody("/World/OtherCube", floor) ||
      physicsRuntime.primForBody(floor) != runtime::PrimId{"/World/OtherCube"} ||
      !physicsRuntime.unbindBody("/World/OtherCube") || physicsRuntime.bodyCount() != 0) {
    return fail("removed static bodies must be reusable and unbind in both mapping directions");
  }

  if (!rejectsInvalidArgument([&] {
        world.createShape({physics::ShapeType::box, {0.0, 1.0, 1.0}});
      }) ||
      !rejectsInvalidArgument([&] {
        world.createBody(
            physics::BodyDescriptor{{}, physics::MotionType::dynamicBody, {}, 1.0, 0});
      }) ||
      !rejectsInvalidArgument([&] {
        world.createBody(physics::BodyDescriptor{
            box, static_cast<physics::MotionType>(-1), {}, 1.0, 0});
      }) ||
      !rejectsInvalidArgument([&] {
        world.createConstraint(
            physics::ConstraintDescriptor{physics::ConstraintType::fixed, cube, cube});
      }) ||
      !rejectsInvalidArgument([&] {
        world.createConstraint(physics::ConstraintDescriptor{
            static_cast<physics::ConstraintType>(-1), floor, cube});
      }) ||
      !rejectsInvalidArgument([&] {
        world.step(physics::PhysicsWorld::Duration{0.0});
      }) ||
      !rejectsInvalidArgument([&] {
        physicsRuntime.bindBody("/World/OtherCube", {});
      }) ||
      !rejectsInvalidArgument([&] {
        physics::validateGroundContact({{}, {0.0, 1.0, 0.0}, 0.0});
      }) ||
      !rejectsInvalidArgument([&] {
        physics::validateGroundContact({floor, {0.0, 0.0, 0.0}, 0.0});
      }) ||
      !rejectsInvalidArgument([&] {
        physics::validateGroundContact({floor, {0.0, 1.0, 0.0}, -0.1});
      }) ||
      !rejectsInvalidArgument([] {
        physics::validateCollisionSegment({}, {});
      }) ||
      !rejectsInvalidArgument([&] {
        physics::validateSegmentHit({{}, 0.5});
      }) ||
      !rejectsInvalidArgument([&] {
        physics::validateSegmentHit({floor, 1.1});
      })) {
    return fail("backend-neutral descriptors and fixed steps must reject invalid values");
  }

  const auto constraint = world.createConstraint(
      physics::ConstraintDescriptor{physics::ConstraintType::fixed, floor, cube});
  if (!constraint) {
    return fail("the world must create a constraint from backend-neutral body handles");
  }

  if (!world.destroyConstraint(constraint) || !world.destroyBody(cube) ||
      !world.destroyBody(floor) || !world.destroyShape(box)) {
    return fail("physics resource lifetime must be explicit and handle-based");
  }

  return 0;
}
