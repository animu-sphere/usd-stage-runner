#include "usd_stage_runner/runtime/fixed_step_accumulator.h"
#include "usd_stage_runner/runtime/frame_clock.h"
#include "usd_stage_runner/runtime/runtime_world.h"
#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/input/movement_controller.h"
#include "usd_stage_runner/input_sdl/physical_input.h"
#include "usd_stage_runner/input_sdl/sdl_input_source.h"

#if USD_STAGE_RUNNER_HAS_OPENUSD
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>
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

using usd_stage_runner::runtime::FixedStepAccumulator;
using usd_stage_runner::runtime::FrameClock;
using usd_stage_runner::runtime::RuntimeWorld;
using usd_stage_runner::input::ActionState;
using usd_stage_runner::input::applyMovementIntent;
using usd_stage_runner::input::updateMovementIntent;

constexpr const char* playerPrim = "/World/PlayerCube";

struct Options {
  std::filesystem::path stagePath;
  std::size_t frameCount{300};
  double fixedDt{1.0 / 60.0};
  std::size_t maxFixedSteps{8};
  bool deterministic{false};
  std::optional<double> moveX;
  std::optional<double> moveY;
};

[[noreturn]] void usageError(const std::string& message) {
  throw std::invalid_argument(
      message + "\nusage: stage_runner <scene.usd[a|c]> [--frames N] [--fixed-dt SECONDS]"
                " [--max-fixed-steps N] [--deterministic] [--move-x VALUE]"
                " [--move-y VALUE]");
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
  return static_cast<std::size_t>(parsed);
}

Options parseOptions(int argc, char** argv) {
  if (argc < 2) {
    usageError("a Stage path is required");
  }

  Options options;
  options.stagePath = argv[1];
  for (int index = 2; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--deterministic") {
      options.deterministic = true;
    } else if (argument == "--frames" || argument == "--max-fixed-steps" ||
               argument == "--fixed-dt" || argument == "--move-x" ||
               argument == "--move-y") {
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
  return options;
}

#if USD_STAGE_RUNNER_HAS_OPENUSD
struct StageContext {
  pxr::UsdStageRefPtr stage;
  RuntimeWorld world;
};

usd_stage_runner::runtime::RuntimeTransform readTransform(const pxr::UsdPrim& prim) {
  usd_stage_runner::runtime::RuntimeTransform transform;
  const pxr::UsdGeomXformable xformable(prim);
  if (!xformable) {
    return transform;
  }
  bool resetsStack = false;
  for (const auto& operation : xformable.GetOrderedXformOps(&resetsStack)) {
    if (operation.GetOpType() != pxr::UsdGeomXformOp::TypeTranslate) {
      continue;
    }
    pxr::GfVec3d translation;
    if (operation.Get(&translation)) {
      transform.translation = {translation[0], translation[1], translation[2]};
    }
    break;
  }
  return transform;
}

StageContext openWorld(const std::filesystem::path& stagePath) {
  const auto stage = pxr::UsdStage::Open(stagePath.string());
  if (!stage) {
    throw std::runtime_error("could not open USD Stage: " + stagePath.string());
  }

  RuntimeWorld world;
  for (const auto& prim : stage->Traverse()) {
    const auto primId = prim.GetPath().GetString();
    world.addPrim(primId);
    if (pxr::UsdGeomXformable(prim)) {
      world.emplaceTransform(primId, readTransform(prim));
    }
  }
  return {stage, std::move(world)};
}

std::size_t synchronizeDirtyTransforms(StageContext& context) {
  std::size_t synchronized = 0;
  for (const auto& primId : context.world.takeDirtyTransforms()) {
    const auto* transform = context.world.transform(primId);
    const pxr::UsdPrim prim = context.stage->GetPrimAtPath(pxr::SdfPath(primId));
    pxr::UsdGeomXformable xformable(prim);
    if (transform == nullptr || !xformable) {
      continue;
    }

    pxr::UsdGeomXformOp translate;
    bool resetsStack = false;
    for (const auto& operation : xformable.GetOrderedXformOps(&resetsStack)) {
      if (operation.GetOpType() == pxr::UsdGeomXformOp::TypeTranslate) {
        translate = operation;
        break;
      }
    }
    if (!translate) {
      translate = xformable.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionDouble);
    }
    const auto& value = transform->translation;
    if (translate && translate.Set(pxr::GfVec3d(value.x, value.y, value.z))) {
      ++synchronized;
    }
  }
  return synchronized;
}
#endif

int run(const Options& options) {
#if !USD_STAGE_RUNNER_HAS_OPENUSD
  (void)options;
  throw std::runtime_error(
      "stage_runner was built without OpenUSD; configure with an OpenUSD SDK or an "
      "OpenStrata usd runtime");
#else
  StageContext context = openWorld(options.stagePath);
  const FixedStepAccumulator::Duration fixedStep{options.fixedDt};
  FixedStepAccumulator accumulator(fixedStep, options.maxFixedSteps);
  FrameClock clock;
  std::size_t fixedSteps = 0;
  std::size_t droppedSteps = 0;
  std::size_t synchronizedTransforms = 0;
  std::size_t processedFrames = 0;
  ActionState actions;
  std::unique_ptr<usd_stage_runner::input_sdl::SdlInputSource> inputSource;
  if (!options.deterministic) {
    inputSource = std::make_unique<usd_stage_runner::input_sdl::SdlInputSource>();
    if (!inputSource->available()) {
      throw std::runtime_error("SDL input is unavailable: " + std::string(inputSource->error()));
    }
  }

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
      usd_stage_runner::input_sdl::mapPhysicalInput(physical, actions);
    }
    if (context.world.containsPrim(playerPrim) && context.world.transform(playerPrim) != nullptr) {
      updateMovementIntent(context.world, playerPrim, actions);
    }

    FrameClock::Duration frameTime;
    if (options.deterministic) {
      frameTime = fixedStep;
    } else {
      nextFrame += std::chrono::duration_cast<FrameClock::Clock::duration>(fixedStep);
      std::this_thread::sleep_until(nextFrame);
      frameTime = clock.tick();
    }

    const auto result = accumulator.advance(frameTime, [&](const auto step) {
      ++fixedSteps;
      if (context.world.containsPrim(playerPrim)) {
        applyMovementIntent(context.world, playerPrim, 3.0, step.count());
      }
    });
    droppedSteps += result.droppedSteps;
    synchronizedTransforms += synchronizeDirtyTransforms(context);
    ++processedFrames;
  }

  std::cout << "opened " << options.stagePath.string() << " with " << context.world.primCount()
            << " prims; frames=" << processedFrames << ", fixed_steps=" << fixedSteps
            << ", dropped_steps=" << droppedSteps
            << ", synchronized_transforms=" << synchronizedTransforms;
  if (const auto* transform = context.world.transform(playerPrim)) {
    const auto& value = transform->translation;
    std::cout << ", player_translation=(" << value.x << ", " << value.y << ", " << value.z
              << ')';
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
