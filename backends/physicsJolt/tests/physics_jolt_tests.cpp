#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"

#include "usd_stage_runner/physics/ground_query.h"

#include <cmath>
#include <iostream>
#include <limits>
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
  auto* groundQuery = dynamic_cast<physics::GroundQuery*>(world.get());
  if (groundQuery == nullptr) {
    return fail("the Jolt world must expose the optional ground-query capability");
  }
  const auto floorShape = world->createShape({physics::ShapeType::box, {5.0, 0.5, 5.0}});
  const auto cubeShape = world->createShape({physics::ShapeType::box, {0.5, 0.5, 0.5}});

  try {
    static_cast<void>(world->createBody(physics::BodyDescriptor{
        cubeShape, physics::MotionType::dynamicBody, {}, 1.0,
        physics_jolt::nonMovingCollisionLayer}));
    return fail("dynamic Jolt bodies must reject the non-moving collision layer");
  } catch (const std::invalid_argument&) {
  }

  const auto floor = world->createBody(physics::BodyDescriptor{
      floorShape, physics::MotionType::staticBody,
      runtime::RuntimeTransform{{0.0, -0.5, 0.0}}, 0.0,
      physics_jolt::nonMovingCollisionLayer});
  const auto probeBody = world->createBody(physics::BodyDescriptor{
      cubeShape, physics::MotionType::dynamicBody,
      runtime::RuntimeTransform{{2.0, 0.65, 0.0}}, 1.0,
      physics_jolt::movingCollisionLayer});
  const auto elevatedContact = groundQuery->groundContact(probeBody, 0.2);
  if (!elevatedContact || elevatedContact->supportBody != floor ||
      std::abs(elevatedContact->distance - 0.15) > 0.01 ||
      elevatedContact->normal.y < 0.99) {
    return fail("the Jolt ground query must report support, distance, and upward normal");
  }
  if (groundQuery->groundContact(probeBody, 0.1)) {
    return fail("the Jolt ground query must honor the requested probe distance");
  }
  try {
    static_cast<void>(groundQuery->groundContact(
        probeBody, std::numeric_limits<double>::quiet_NaN()));
    return fail("the Jolt ground query must reject invalid probe distances");
  } catch (const std::invalid_argument&) {
  }
  try {
    static_cast<void>(groundQuery->groundContact(physics::BodyHandle{9999}, 0.2));
    return fail("the Jolt ground query must reject unknown bodies");
  } catch (const std::out_of_range&) {
  }
  if (!world->destroyBody(probeBody)) {
    return fail("a probed Jolt body must retain ordinary body lifetime");
  }
  const auto cube = world->createBody(physics::BodyDescriptor{
      cubeShape, physics::MotionType::dynamicBody,
      runtime::RuntimeTransform{{0.0, 3.0, 0.0}}, 1.0,
      physics_jolt::movingCollisionLayer});

  for (int step = 0; step < 240; ++step) {
    world->step(physics::PhysicsWorld::Duration{1.0 / 60.0});
  }

  const auto settled = world->bodyState(cube);
  if (std::abs(settled.transform.translation.y - 0.5) > 0.03 ||
      std::abs(settled.linearVelocity.y) > 0.05) {
    return fail("the Jolt cube must fall and settle on the static floor");
  }
  const auto settledContact = groundQuery->groundContact(cube, 0.1);
  if (!settledContact || settledContact->supportBody != floor ||
      settledContact->distance > 0.02 || settledContact->normal.y < 0.99) {
    return fail("a settled Jolt body must be grounded on its supporting body");
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
