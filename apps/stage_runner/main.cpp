#include "usd_stage_runner/camera/camera_rig.h"
#include "usd_stage_runner/character/character_controller.h"
#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/input_sdl/physical_input.h"
#include "usd_stage_runner/input_sdl/sdl_input_source.h"
#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"
#include "usd_stage_runner/runtime/frame_clock.h"

#if USD_STAGE_RUNNER_HAS_OPENUSD
#include "usd_stage_runner/stage/stage_session.h"

#include <pxr/base/plug/registry.h>
#include <pxr/usd/usd/stage.h>
#endif

#if USD_STAGE_RUNNER_HAS_OPENUSD && defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using usd_stage_runner::input::ActionState;
using usd_stage_runner::runtime::FrameClock;

struct Options {
  std::filesystem::path executablePath;
  std::filesystem::path stagePath;
  std::size_t frameCount{300};
  double fixedDt{1.0 / 60.0};
  std::size_t maxFixedSteps{8};
  bool deterministic{false};
  std::optional<double> moveX;
  std::optional<double> moveY;
  bool jump{false};
};

[[noreturn]] void usageError(const std::string& message) {
  throw std::invalid_argument(
      message + "\nusage: stage_runner <scene.usd[a|c]> [--frames N] [--fixed-dt SECONDS]"
                " [--max-fixed-steps N] [--deterministic] [--move-x VALUE]"
                " [--move-y VALUE] [--jump]");
}

double parseUnitValue(const std::string& value, const std::string& option) {
  std::size_t consumed = 0;
  const double parsed = std::stod(value, &consumed);
  if (consumed != value.size() || !std::isfinite(parsed) || parsed < -1.0 || parsed > 1.0) {
    usageError(option + " requires a number from -1 to 1");
  }
  return parsed;
}

std::size_t parsePositiveSize(const std::string& value, const std::string& option) {
  std::size_t parsed = 0;
  const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed == 0) {
    usageError(option + " requires a positive integer");
  }
  return parsed;
}

Options parseOptions(int argc, char** argv) {
  if (argc < 2) {
    usageError("a Stage path is required");
  }

  Options options;
  options.executablePath = argv[0];
  options.stagePath = argv[1];
  for (int index = 2; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--deterministic") {
      options.deterministic = true;
    } else if (argument == "--jump") {
      options.jump = true;
    } else if (argument == "--frames" || argument == "--max-fixed-steps" ||
               argument == "--fixed-dt" || argument == "--move-x" || argument == "--move-y") {
      if (++index >= argc) {
        usageError(argument + " requires a value");
      }
      const std::string value = argv[index];
      if (argument == "--frames") {
        options.frameCount = parsePositiveSize(value, argument);
      } else if (argument == "--max-fixed-steps") {
        options.maxFixedSteps = parsePositiveSize(value, argument);
      } else if (argument == "--move-x") {
        options.moveX = parseUnitValue(value, argument);
      } else if (argument == "--move-y") {
        options.moveY = parseUnitValue(value, argument);
      } else {
        std::size_t consumed = 0;
        options.fixedDt = std::stod(value, &consumed);
        if (consumed != value.size() || !std::isfinite(options.fixedDt) || options.fixedDt <= 0.0) {
          usageError(argument + " requires a positive number");
        }
      }
    } else {
      usageError("unknown option: " + argument);
    }
  }
  if (!options.deterministic && (options.moveX.has_value() || options.moveY.has_value())) {
    usageError("--move-x and --move-y require --deterministic");
  }
  if (!options.deterministic && options.jump) {
    usageError("--jump requires --deterministic");
  }
  return options;
}

#if USD_STAGE_RUNNER_HAS_OPENUSD
std::filesystem::path executableDirectory(const std::filesystem::path& invocation) {
#ifdef _WIN32
  std::wstring buffer(32768, L'\0');
  const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if (length != 0 && length < static_cast<DWORD>(buffer.size())) {
    buffer.resize(length);
    return std::filesystem::path{buffer}.parent_path();
  }
#elif defined(__linux__)
  std::error_code processError;
  const auto processPath = std::filesystem::read_symlink("/proc/self/exe", processError);
  if (!processError) {
    return processPath.parent_path();
  }
#endif

  std::error_code error;
  if (invocation.is_absolute() || invocation.has_parent_path()) {
    const auto absolutePath = std::filesystem::absolute(invocation, error);
    if (!error) {
      return absolutePath.parent_path();
    }
  }
  throw std::runtime_error("could not resolve the stage_runner executable path: " +
                           invocation.string());
}

void registerRunnerSchemas(const std::filesystem::path& executablePath) {
#ifdef USD_STAGE_RUNNER_SCHEMA_RESOURCE_RELATIVE_PATH
  const auto resourcePath =
      executableDirectory(executablePath) / USD_STAGE_RUNNER_SCHEMA_RESOURCE_RELATIVE_PATH;
  if (!std::filesystem::is_regular_file(resourcePath / "plugInfo.json")) {
    throw std::runtime_error("runnerSchema resources were not found at " +
                             resourcePath.lexically_normal().string());
  }
  (void)pxr::PlugRegistry::GetInstance().RegisterPlugins(resourcePath.string());
#else
  (void)executablePath;
#endif
}

const char* jumpStateName(usd_stage_runner::character::JumpState state) noexcept {
  switch (state) {
  case usd_stage_runner::character::JumpState::grounded:
    return "grounded";
  case usd_stage_runner::character::JumpState::rising:
    return "rising";
  case usd_stage_runner::character::JumpState::falling:
    return "falling";
  }
  return "unknown";
}
#endif

int run(const Options& options) {
#if !USD_STAGE_RUNNER_HAS_OPENUSD
  (void)options;
  throw std::runtime_error(
      "stage_runner was built without OpenUSD; configure with an OpenUSD SDK or an "
      "OpenStrata usd runtime");
#else
  registerRunnerSchemas(options.executablePath);
  const auto stage = pxr::UsdStage::Open(options.stagePath.string());
  if (!stage) {
    throw std::runtime_error("could not open USD Stage: " + options.stagePath.string());
  }

  usd_stage_runner::stage::StageSessionConfig sessionConfig;
  sessionConfig.fixedStep = usd_stage_runner::stage::StageSession::Duration{options.fixedDt};
  sessionConfig.maxFixedStepsPerFrame = options.maxFixedSteps;
  sessionConfig.staticCollisionLayer = usd_stage_runner::physics_jolt::nonMovingCollisionLayer;
  sessionConfig.dynamicCollisionLayer = usd_stage_runner::physics_jolt::movingCollisionLayer;
  usd_stage_runner::stage::StageSession session(
      stage, sessionConfig, []() -> std::unique_ptr<usd_stage_runner::physics::PhysicsWorld> {
        if (!usd_stage_runner::physics_jolt::isJoltPhysicsAvailable()) {
          throw std::runtime_error(
              "Stage declares physics bodies, but Jolt Physics is unavailable in this build");
        }
        return usd_stage_runner::physics_jolt::createJoltPhysicsWorld();
      });

  FrameClock clock;
  std::size_t processedFrames = 0;
  ActionState actions;
  std::unique_ptr<usd_stage_runner::input_sdl::SdlInputSource> inputSource;
  if (!options.deterministic) {
    inputSource = std::make_unique<usd_stage_runner::input_sdl::SdlInputSource>();
    if (!inputSource->available()) {
      throw std::runtime_error("SDL input is unavailable: " + std::string(inputSource->error()));
    }
  }

  session.play();
  auto nextFrame = FrameClock::Clock::now();
  (void)clock.tick();
  for (std::size_t frame = 0; frame < options.frameCount; ++frame) {
    if (inputSource && !inputSource->poll(actions)) {
      break;
    }
    if (options.deterministic) {
      usd_stage_runner::input_sdl::PhysicalInputState physical;
      physical.gamepadMoveX = options.moveX.value_or(0.0);
      physical.gamepadMoveY = options.moveY.value_or(0.0);
      physical.gamepadJump = options.jump;
      usd_stage_runner::input_sdl::mapPhysicalInput(physical, actions);
    }
    session.setActions(actions);

    FrameClock::Duration frameTime;
    if (options.deterministic) {
      frameTime = session.fixedStep();
    } else {
      nextFrame += std::chrono::duration_cast<FrameClock::Clock::duration>(session.fixedStep());
      std::this_thread::sleep_until(nextFrame);
      frameTime = clock.tick();
    }
    (void)session.advance(frameTime);
    ++processedFrames;
  }

  const auto& stats = session.stats();
  const auto& world = session.world();
  std::cout << "opened " << options.stagePath.string() << " with " << world.primCount()
            << " prims; frames=" << processedFrames << ", fixed_steps=" << stats.fixedSteps
            << ", dropped_steps=" << stats.droppedSteps
            << ", synchronized_transforms=" << stats.synchronizedTransforms;
  if (const auto* transform = world.transform(session.config().playerPrim)) {
    const auto& value = transform->translation;
    std::cout << ", player_translation=(" << value.x << ", " << value.y << ", " << value.z << ')';
  }
  std::cout << ", physics_shapes=" << stats.physicsShapeCount
            << ", physics_bodies=" << stats.physicsBodyCount
            << ", physics_body_updates=" << stats.physicsBodyUpdates
            << ", character_controllers=" << stats.characterControllerCount;
  if (const auto* controller = world.component<usd_stage_runner::character::CharacterController>(
          session.config().playerPrim)) {
    std::cout << ", character_grounded=" << (controller->state().grounded ? "true" : "false")
              << ", character_jump_state=" << jumpStateName(controller->state().jumpState);
  }
  std::cout << ", camera_rigs=" << stats.cameraRigCount
            << ", camera_rig_updates=" << stats.cameraRigUpdates
            << ", synchronized_camera_transforms=" << stats.synchronizedCameraTransforms;
  for (const auto& prim : session.cameraPrims()) {
    const auto* rig = world.component<usd_stage_runner::camera::CameraRig>(prim);
    if (rig == nullptr || !rig->state().initialized) {
      continue;
    }
    const auto& pose = rig->state().current;
    std::cout << ", camera_pose[" << prim << "]=(position=" << pose.position.x << ','
              << pose.position.y << ',' << pose.position.z << "; forward=" << pose.forward.x << ','
              << pose.forward.y << ',' << pose.forward.z << ')';
  }
  std::cout << '\n';
  return 0;
#endif
}

} // namespace

int main(int argc, char** argv) {
  try {
    return run(parseOptions(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "stage_runner: " << error.what() << '\n';
    return 2;
  }
}
