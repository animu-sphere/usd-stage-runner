#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

} // namespace

int main() {
  using namespace usd_stage_runner;

  if (!physics_jolt::isJoltPhysicsAvailable()) {
    try {
      static_cast<void>(physics_jolt::createJoltPhysicsWorld());
    } catch (const std::runtime_error&) {
      return 0;
    }
    return fail("an unavailable Jolt build must reject world creation");
  }

  auto world = physics_jolt::createJoltPhysicsWorld();
  const auto floorShape = world->createShape({physics::ShapeType::box, {5.0, 0.5, 5.0}});
  const auto cubeShape = world->createShape({physics::ShapeType::box, {0.5, 0.5, 0.5}});
  const auto floor = world->createBody(physics::BodyDescriptor{
      floorShape, physics::MotionType::staticBody,
      runtime::RuntimeTransform{{0.0, -0.5, 0.0}}, 0.0, 0});
  const auto cube = world->createBody(physics::BodyDescriptor{
      cubeShape, physics::MotionType::dynamicBody,
      runtime::RuntimeTransform{{0.0, 3.0, 0.0}}, 1.0, 1});

  for (int step = 0; step < 240; ++step) {
    world->step(physics::PhysicsWorld::Duration{1.0 / 60.0});
  }

  const auto settled = world->bodyState(cube);
  if (std::abs(settled.transform.translation.y - 0.5) > 0.03 ||
      std::abs(settled.linearVelocity.y) > 0.05) {
    return fail("the Jolt cube must fall and settle on the static floor");
  }
  const auto changed = world->takeChangedBodyStates();
  if (changed.size() != 1 || changed.front().body != cube) {
    return fail("Jolt stepping must expose only changed dynamic body state");
  }
  if (!world->takeChangedBodyStates().empty()) {
    return fail("taking Jolt body changes must drain the extraction queue");
  }

  const auto fixed = world->createConstraint(
      {physics::ConstraintType::fixed, floor, cube});
  if (!fixed || !world->destroyConstraint(fixed)) {
    return fail("the Jolt adapter must own fixed-constraint lifetime");
  }

  if (!world->destroyBody(cube) || !world->destroyBody(floor) ||
      !world->destroyShape(cubeShape) || !world->destroyShape(floorShape)) {
    return fail("Jolt resources must have explicit handle-based lifetime");
  }

  return 0;
}
