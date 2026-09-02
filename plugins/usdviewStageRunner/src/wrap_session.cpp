#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"
#include "usd_stage_runner/runtime/play_session.h"
#include "usd_stage_runner/stage/stage_session.h"

#include <pxr/external/boost/python.hpp>
#include <pxr/usd/usd/stage.h>

#include <chrono>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

namespace python = PXR_BOOST_NAMESPACE::python;
using usd_stage_runner::stage::StageSession;

class PythonStageSession {
public:
  PythonStageSession(pxr::UsdStageRefPtr stage, double fixedStepSeconds,
                     std::size_t maxFixedStepsPerFrame, std::string playerPrim,
                     double playerSpeed)
      : session_(std::move(stage),
                 makeConfig(fixedStepSeconds, maxFixedStepsPerFrame, std::move(playerPrim),
                            playerSpeed),
                 []() -> std::unique_ptr<usd_stage_runner::physics::PhysicsWorld> {
                   if (!usd_stage_runner::physics_jolt::isJoltPhysicsAvailable()) {
                     throw std::runtime_error(
                         "Stage declares physics bodies, but Jolt Physics is unavailable in this "
                         "build");
                   }
                   return usd_stage_runner::physics_jolt::createJoltPhysicsWorld();
                 }) {}

  void play() noexcept {
    session_.play();
  }

  void pause() noexcept {
    session_.pause();
  }

  void stop() {
    session_.stop();
  }

  void singleStep() {
    session_.singleStep();
  }

  void reset() {
    session_.reset();
  }

  python::dict advance(double frameTimeSeconds) {
    const auto result = session_.advance(StageSession::Duration{frameTimeSeconds});
    python::dict value;
    value["steps"] = result.steps;
    value["droppedSteps"] = result.droppedSteps;
    value["interpolationAlpha"] = result.interpolationAlpha;
    return value;
  }

  void setActions(double moveX, double moveY, bool jump) {
    usd_stage_runner::input::ActionState actions;
    actions.set(std::string{usd_stage_runner::input::actions::moveX}, moveX);
    actions.set(std::string{usd_stage_runner::input::actions::moveY}, moveY);
    actions.set(std::string{usd_stage_runner::input::actions::jump}, jump ? 1.0 : 0.0);
    session_.setActions(std::move(actions));
  }

  [[nodiscard]] std::string state() const {
    return session_.state() == usd_stage_runner::runtime::PlaySession::State::playing ? "playing"
                                                                                       : "paused";
  }

  [[nodiscard]] double fixedStep() const noexcept {
    return session_.fixedStep().count();
  }

  [[nodiscard]] python::dict stats() const {
    const auto& stats = session_.stats();
    python::dict value;
    value["physicsShapeCount"] = stats.physicsShapeCount;
    value["physicsBodyCount"] = stats.physicsBodyCount;
    value["characterControllerCount"] = stats.characterControllerCount;
    value["cameraRigCount"] = stats.cameraRigCount;
    value["fixedSteps"] = stats.fixedSteps;
    value["droppedSteps"] = stats.droppedSteps;
    value["physicsBodyUpdates"] = stats.physicsBodyUpdates;
    value["cameraRigUpdates"] = stats.cameraRigUpdates;
    value["synchronizedTransforms"] = stats.synchronizedTransforms;
    value["synchronizedCameraTransforms"] = stats.synchronizedCameraTransforms;
    return value;
  }

private:
  static usd_stage_runner::stage::StageSessionConfig
  makeConfig(double fixedStepSeconds, std::size_t maxFixedStepsPerFrame,
             std::string playerPrim, double playerSpeed) {
    usd_stage_runner::stage::StageSessionConfig config;
    config.fixedStep = StageSession::Duration{fixedStepSeconds};
    config.maxFixedStepsPerFrame = maxFixedStepsPerFrame;
    config.playerPrim = usd_stage_runner::runtime::PrimId{std::move(playerPrim)};
    config.playerSpeed = playerSpeed;
    config.staticCollisionLayer = usd_stage_runner::physics_jolt::nonMovingCollisionLayer;
    config.dynamicCollisionLayer = usd_stage_runner::physics_jolt::movingCollisionLayer;
    return config;
  }

  StageSession session_;
};

PythonStageSession* createSession(const pxr::UsdStageRefPtr& stage, double fixedStepSeconds,
                                  std::size_t maxFixedStepsPerFrame,
                                  const std::string& playerPrim, double playerSpeed) {
  if (!stage) {
    throw std::invalid_argument("usdviewStageRunner requires a valid Usd.Stage");
  }
  return new PythonStageSession(stage, fixedStepSeconds, maxFixedStepsPerFrame, playerPrim,
                                playerSpeed);
}

} // namespace

PXR_BOOST_PYTHON_MODULE(_usdviewStageRunner) {
  python::class_<PythonStageSession, python::noncopyable>("Session", python::no_init)
      .def("play", &PythonStageSession::play)
      .def("pause", &PythonStageSession::pause)
      .def("stop", &PythonStageSession::stop)
      .def("singleStep", &PythonStageSession::singleStep)
      .def("reset", &PythonStageSession::reset)
      .def("advance", &PythonStageSession::advance)
      .def("setActions", &PythonStageSession::setActions)
      .add_property("state", &PythonStageSession::state)
      .add_property("fixedStep", &PythonStageSession::fixedStep)
      .add_property("stats", &PythonStageSession::stats);

  python::def("createSession", &createSession,
              python::return_value_policy<python::manage_new_object>(),
              (python::arg("stage"), python::arg("fixedStepSeconds") = 1.0 / 60.0,
               python::arg("maxFixedStepsPerFrame") = 8,
               python::arg("playerPrim") = "/World/PlayerCube",
               python::arg("playerSpeed") = 3.0));
}
