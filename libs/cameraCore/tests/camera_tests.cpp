#include "usd_stage_runner/camera/camera_rig.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

using namespace usd_stage_runner;

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bool near(double left, double right, double tolerance = 1.0e-9) {
  return std::abs(left - right) < tolerance;
}

bool near(const runtime::Vec3d& left, const runtime::Vec3d& right,
          double tolerance = 1.0e-9) {
  return near(left.x, right.x, tolerance) && near(left.y, right.y, tolerance) &&
         near(left.z, right.z, tolerance);
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

  runtime::RuntimeWorld world;
  world.addPrim("/World/Player");
  world.emplaceTransform("/World/Player", {{10.0, 2.0, -3.0}});
  world.addPrim("/World/Head");
  world.emplaceTransform("/World/Head", {{10.0, 3.0, -3.0}});
  world.addPrim("/World/Camera");
  world.emplaceTransform("/World/Camera", {{1.0, 2.0, 3.0}});

  camera::CameraRig freeRig;
  if (!freeRig.update(world, *world.transform("/World/Camera"),
                      camera::CameraRig::Duration{1.0 / 60.0}) ||
      !near(freeRig.state().current.position, {1.0, 2.0, 3.0}) ||
      !near(freeRig.state().current.forward, {0.0, 0.0, -1.0})) {
    return fail("free mode must preserve position and calculate a deterministic view direction");
  }

  camera::CameraRig firstPerson({camera::CameraRigMode::firstPerson,
                                 "/World/Player",
                                 runtime::PrimId{"/World/Head"},
                                 {0.25, 0.1, 0.0},
                                 4.0,
                                 0.0,
                                 0.0,
                                 0.0});
  if (!firstPerson.update(world, *world.transform("/World/Camera"),
                          camera::CameraRig::Duration{1.0 / 60.0}) ||
      !near(firstPerson.state().current.position, {10.25, 3.1, -3.0}) ||
      !near(firstPerson.state().current.forward, {0.0, 0.0, -1.0})) {
    return fail("first-person mode must place the camera at its optional anchor");
  }

  camera::CameraRig thirdPerson({camera::CameraRigMode::thirdPerson,
                                 "/World/Player",
                                 std::nullopt,
                                 {0.5, 1.5, 0.0},
                                 4.0,
                                 0.0,
                                 0.0,
                                 0.0});
  if (!thirdPerson.update(world, *world.transform("/World/Camera"),
                          camera::CameraRig::Duration{1.0 / 60.0}) ||
      !near(thirdPerson.state().current.position, {10.5, 3.5, 1.0}) ||
      !near(thirdPerson.state().current.forward, {0.0, 0.0, -1.0})) {
    return fail("third-person mode must follow behind its target at the configured distance");
  }

  bool collisionProbeCalled = false;
  camera::CameraRig collisionRig({camera::CameraRigMode::thirdPerson,
                                  "/World/Player",
                                  std::nullopt,
                                  {0.0, 1.5, 0.0},
                                  4.0,
                                  0.0,
                                  0.0,
                                  0.0,
                                  true,
                                  0.25});
  const camera::CameraCollisionProbe collisionProbe =
      [&](const runtime::Vec3d& origin, const runtime::Vec3d& desired,
          const runtime::PrimId& target) -> std::optional<double> {
    collisionProbeCalled = near(origin, {10.0, 3.5, -3.0}) &&
                           near(desired, {10.0, 3.5, 1.0}) &&
                           target == "/World/Player";
    return 0.5;
  };
  if (!collisionRig.update(world, *world.transform("/World/Camera"),
                           camera::CameraRig::Duration{1.0 / 60.0}, collisionProbe) ||
      !collisionProbeCalled ||
      !near(collisionRig.state().desired.position, {10.0, 3.5, -1.25}) ||
      !near(collisionRig.state().current.position, {10.0, 3.5, -1.25})) {
    return fail("third-person collision must shorten the desired distance before smoothing");
  }

  camera::CameraRig zeroDistanceCollision({camera::CameraRigMode::thirdPerson,
                                           "/World/Player",
                                           std::nullopt,
                                           {0.0, 1.5, 0.0},
                                           0.0,
                                           0.0,
                                           0.0,
                                           0.0,
                                           true});
  if (!zeroDistanceCollision.update(world, *world.transform("/World/Camera"),
                                    camera::CameraRig::Duration{1.0 / 60.0}) ||
      !near(zeroDistanceCollision.state().current.position, {10.0, 3.5, -3.0})) {
    return fail("a zero-distance camera must not issue a zero-length collision probe");
  }

  const double quarterTurn = 1.57079632679489661923;
  camera::CameraRig orbit({camera::CameraRigMode::orbit,
                           "/World/Player",
                           std::nullopt,
                           {0.0, 1.0, 0.0},
                           2.0,
                           0.0,
                           quarterTurn,
                           0.0});
  if (!orbit.update(world, *world.transform("/World/Camera"),
                    camera::CameraRig::Duration{1.0 / 60.0}) ||
      !near(orbit.state().current.position, {8.0, 3.0, -3.0}) ||
      !near(orbit.state().current.forward,
            {2.0 / std::sqrt(5.0), -1.0 / std::sqrt(5.0), 0.0})) {
    return fail("orbit mode must place the camera from yaw and pitch and aim at its origin");
  }

  world.emplaceComponent<camera::CameraRig>(
      "/World/Camera",
      camera::CameraRigConfig{camera::CameraRigMode::thirdPerson,
                              "/World/Player",
                              std::nullopt,
                              {0.0, 1.0, 0.0},
                              5.0,
                              0.0,
                              0.0,
                              2.0});
  if (!camera::updateCameraRig(world, "/World/Camera",
                               camera::CameraRig::Duration{1.0 / 60.0}) ||
      !near(world.transform("/World/Camera")->translation, {10.0, 3.0, 2.0}) ||
      world.dirtyTransformCount() != 1) {
    return fail("prim-indexed evaluation must update and dirty the camera transform");
  }
  world.takeDirtyTransforms();

  world.transform("/World/Player")->translation.x = 20.0;
  const double response = 1.0 - std::exp(-1.0);
  if (!camera::updateCameraRig(world, "/World/Camera", camera::CameraRig::Duration{0.5}) ||
      !near(world.transform("/World/Camera")->translation.x, 10.0 + 10.0 * response) ||
      !near(world.component<camera::CameraRig>("/World/Camera")
                ->state()
                .desired.position.x,
            20.0)) {
    return fail("damping must follow a moved target with deterministic exponential response");
  }
  world.takeDirtyTransforms();

  const auto heldPosition = world.transform("/World/Camera")->translation;
  if (camera::updateCameraRig(world, "/World/Camera", camera::CameraRig::Duration{0.0}) ||
      !near(world.transform("/World/Camera")->translation, heldPosition) ||
      world.dirtyTransformCount() != 0) {
    return fail("a zero elapsed update must retain initialized smoothing state without dirtying");
  }

  camera::CameraRig oppositeTurn({camera::CameraRigMode::firstPerson,
                                  "/World/Player",
                                  std::nullopt,
                                  {},
                                  0.0,
                                  0.0,
                                  0.0,
                                  std::log(2.0)});
  oppositeTurn.update(world, *world.transform("/World/Camera"),
                      camera::CameraRig::Duration{1.0});
  auto oppositeConfig = oppositeTurn.config();
  oppositeConfig.yawRadians = 3.14159265358979323846;
  oppositeTurn.setConfig(oppositeConfig);
  if (!oppositeTurn.update(world, *world.transform("/World/Camera"),
                           camera::CameraRig::Duration{1.0}) ||
      !near(oppositeTurn.state().current.forward, {1.0, 0.0, 0.0})) {
    return fail("smoothing must turn deterministically between opposite directions");
  }

  auto* attachedRig = world.component<camera::CameraRig>("/World/Camera");
  const auto beforeSwitch = attachedRig->state().current;
  auto switchedConfig = attachedRig->config();
  switchedConfig.mode = camera::CameraRigMode::firstPerson;
  attachedRig->setConfig(switchedConfig);
  if (!near(attachedRig->state().current.position, beforeSwitch.position) ||
      !near(attachedRig->state().current.forward, beforeSwitch.forward)) {
    return fail("mode switching must preserve live smoothing state until the next update");
  }

  if (!rejectsInvalidArgument([] {
        camera::CameraRig invalid({camera::CameraRigMode::thirdPerson, "Player"});
      }) ||
      !rejectsInvalidArgument([] {
        camera::CameraRig invalid(
            {camera::CameraRigMode::orbit,
             "/World/Player",
             std::nullopt,
             {},
             -1.0});
      }) ||
      !rejectsInvalidArgument([] {
        camera::CameraRig invalid(
            {camera::CameraRigMode::firstPerson,
             "/World/Player",
             std::nullopt,
             {},
             0.0,
             std::numeric_limits<double>::quiet_NaN()});
      }) ||
      !rejectsInvalidArgument([] {
        camera::CameraRig invalid({camera::CameraRigMode::orbit, "/World/Player",
                                   std::nullopt, {}, 0.0});
      }) ||
      !rejectsInvalidArgument([&] {
        freeRig.update(world, *world.transform("/World/Camera"),
                       camera::CameraRig::Duration{-0.1});
      }) ||
      !rejectsInvalidArgument([] {
        camera::CameraRig invalid({camera::CameraRigMode::thirdPerson,
                                   "/World/Player",
                                   std::nullopt,
                                   {},
                                   4.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   true,
                                   -0.1});
      }) ||
      !rejectsInvalidArgument([&] {
        camera::CameraRig enabled({camera::CameraRigMode::thirdPerson,
                                   "/World/Player",
                                   std::nullopt,
                                   {},
                                   4.0,
                                   0.0,
                                   0.0,
                                   0.0,
                                   true});
        enabled.update(world, *world.transform("/World/Camera"),
                       camera::CameraRig::Duration{1.0 / 60.0});
      })) {
    return fail("camera rig contracts must reject invalid identities, values, and timesteps");
  }

  camera::CameraRig missingTarget(
      {camera::CameraRigMode::thirdPerson, "/World/Missing"});
  if (!rejectsOutOfRange([&] {
        missingTarget.update(world, *world.transform("/World/Camera"),
                             camera::CameraRig::Duration{1.0 / 60.0});
      }) ||
      !rejectsOutOfRange([&] {
        camera::updateCameraRig(world, "/World/Player",
                                camera::CameraRig::Duration{1.0 / 60.0});
      })) {
    return fail("camera evaluation must reject unresolved targets and missing rig components");
  }

  return 0;
}
