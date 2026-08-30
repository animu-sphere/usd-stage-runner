#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/input/movement_controller.h"
#include "usd_stage_runner/input_sdl/physical_input.h"
#include "usd_stage_runner/input_sdl/sdl_input_source.h"
#include "usd_stage_runner/physics/physics_runtime.h"
#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"
#include "usd_stage_runner/runtime/fixed_step_accumulator.h"
#include "usd_stage_runner/runtime/frame_clock.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#if USD_STAGE_RUNNER_HAS_OPENUSD
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>
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

using usd_stage_runner::input::ActionState;
using usd_stage_runner::input::applyMovementIntent;
using usd_stage_runner::input::updateMovementIntent;
using usd_stage_runner::physics::BodyDescriptor;
using usd_stage_runner::physics::MotionType;
using usd_stage_runner::physics::PhysicsRuntime;
using usd_stage_runner::physics::PhysicsWorld;
using usd_stage_runner::physics::ShapeDescriptor;
using usd_stage_runner::runtime::FixedStepAccumulator;
using usd_stage_runner::runtime::FrameClock;
using usd_stage_runner::runtime::RuntimeWorld;

constexpr const char* playerPrim = "/World/PlayerCube";
#if USD_STAGE_RUNNER_HAS_OPENUSD
const pxr::TfToken physicsMotionTypeAttribute{"runner:physics:motionType"};
const pxr::TfToken physicsMassAttribute{"runner:physics:mass"};
#endif

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
  return options;
}

#if USD_STAGE_RUNNER_HAS_OPENUSD
struct StageContext {
  pxr::UsdStageRefPtr stage;
  RuntimeWorld world;
};

struct PhysicsImportSummary {
  std::size_t shapeCount{0};
  std::size_t bodyCount{0};
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
    if (operation.GetAs(&translation, pxr::UsdTimeCode::Default())) {
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

std::optional<MotionType> readMotionType(const pxr::UsdPrim& prim) {
  const auto attribute = prim.GetAttribute(physicsMotionTypeAttribute);
  if (!attribute) {
    return std::nullopt;
  }

  pxr::TfToken value;
  if (!attribute.Get(&value)) {
    throw std::runtime_error("could not read " + physicsMotionTypeAttribute.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  if (value == pxr::TfToken{"static"}) {
    return MotionType::staticBody;
  }
  if (value == pxr::TfToken{"dynamic"}) {
    return MotionType::dynamicBody;
  }
  throw std::runtime_error(physicsMotionTypeAttribute.GetString() + " on " +
                           prim.GetPath().GetString() + " must be 'static' or 'dynamic'");
}

usd_stage_runner::runtime::Vec3d readBoxHalfExtents(const pxr::UsdGeomCube& cube) {
  double size = 2.0;
  if (!cube.GetSizeAttr().Get(&size) || !std::isfinite(size) || size <= 0.0) {
    throw std::runtime_error("physics Cube size must be finite and positive on " +
                             cube.GetPath().GetString());
  }

  pxr::GfVec3d scale{1.0};
  bool resetsStack = false;
  for (const auto& operation : cube.GetOrderedXformOps(&resetsStack)) {
    if (operation.GetOpType() == pxr::UsdGeomXformOp::TypeTranslate) {
      continue;
    }
    if (operation.GetOpType() != pxr::UsdGeomXformOp::TypeScale) {
      throw std::runtime_error("temporary physics import supports only translate and scale ops: " +
                               cube.GetPath().GetString());
    }
    pxr::GfVec3d operationScale;
    if (!operation.GetAs(&operationScale, pxr::UsdTimeCode::Default())) {
      throw std::runtime_error("could not read scale op on " + cube.GetPath().GetString());
    }
    scale[0] *= operationScale[0];
    scale[1] *= operationScale[1];
    scale[2] *= operationScale[2];
  }

  const double halfSize = size * 0.5;
  return {std::abs(scale[0]) * halfSize, std::abs(scale[1]) * halfSize,
          std::abs(scale[2]) * halfSize};
}

double readBodyMass(const pxr::UsdPrim& prim, MotionType motionType) {
  if (motionType == MotionType::staticBody) {
    return 1.0;
  }
  const auto attribute = prim.GetAttribute(physicsMassAttribute);
  if (!attribute) {
    return 1.0;
  }
  double mass = 0.0;
  if (!attribute.Get(&mass)) {
    throw std::runtime_error("could not read " + physicsMassAttribute.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  return mass;
}

std::size_t physicsDeclarationCount(const pxr::UsdStageRefPtr& stage) {
  std::size_t count = 0;
  for (const auto& prim : stage->Traverse()) {
    if (prim.GetAttribute(physicsMotionTypeAttribute)) {
      ++count;
    }
  }
  return count;
}

PhysicsImportSummary importPhysicsBodies(StageContext& context, PhysicsWorld& physicsWorld,
                                         PhysicsRuntime& physicsRuntime) {
  if (pxr::UsdGeomGetStageUpAxis(context.stage) != pxr::UsdGeomTokens->y) {
    throw std::runtime_error("temporary physics import requires a Y-up Stage");
  }
  PhysicsImportSummary summary;
  for (const auto& prim : context.stage->Traverse()) {
    const auto motionType = readMotionType(prim);
    if (!motionType.has_value()) {
      continue;
    }

    const pxr::UsdGeomCube cube(prim);
    if (!cube) {
      throw std::runtime_error("temporary physics import supports only UsdGeomCube prims: " +
                               prim.GetPath().GetString());
    }
    const auto primId = prim.GetPath().GetString();
    const auto shape = physicsWorld.createShape(
        ShapeDescriptor{usd_stage_runner::physics::ShapeType::box, readBoxHalfExtents(cube)});
    ++summary.shapeCount;
    const bool dynamic = *motionType == MotionType::dynamicBody;
    const auto body = physicsWorld.createBody(BodyDescriptor{
        shape, *motionType, *context.world.transform(primId), readBodyMass(prim, *motionType),
        dynamic ? usd_stage_runner::physics_jolt::movingCollisionLayer
                : usd_stage_runner::physics_jolt::nonMovingCollisionLayer});
    physicsRuntime.bindBody(primId, body);
    ++summary.bodyCount;
  }
  return summary;
}

bool applyMovementIntentToPhysics(RuntimeWorld& world, const std::string& prim,
                                  PhysicsWorld& physicsWorld, PhysicsRuntime& physicsRuntime,
                                  double speed) {
  const auto body = physicsRuntime.bodyForPrim(prim);
  const auto* intent = world.component<usd_stage_runner::input::MovementIntent>(prim);
  if (!body || intent == nullptr) {
    return false;
  }
  const auto state = physicsWorld.bodyState(body);
  return physicsWorld.setLinearVelocity(
      body, {intent->x * speed, state.linearVelocity.y, intent->y * speed});
}

bool setTranslate(const pxr::UsdGeomXformOp& operation,
                  const usd_stage_runner::runtime::RuntimeTransform& transform) {
  const auto& value = transform.translation;
  const pxr::GfVec3d translation(value.x, value.y, value.z);
  switch (operation.GetPrecision()) {
  case pxr::UsdGeomXformOp::PrecisionDouble:
    return operation.Set(translation);
  case pxr::UsdGeomXformOp::PrecisionFloat:
    return operation.Set(pxr::GfVec3f(translation));
  case pxr::UsdGeomXformOp::PrecisionHalf:
    return operation.Set(pxr::GfVec3h(translation));
  }
  return false;
}

std::size_t synchronizeDirtyTransforms(StageContext& context) {
  std::size_t synchronized = 0;
  for (const auto& primId : context.world.takeDirtyTransforms()) {
    const auto* transform = context.world.transform(primId);
    const pxr::UsdPrim prim = context.stage->GetPrimAtPath(pxr::SdfPath(primId));
    pxr::UsdGeomXformable xformable(prim);
    if (transform == nullptr || !xformable) {
      if (transform != nullptr) {
        context.world.markTransformDirty(primId);
      }
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
    if (translate && setTranslate(translate, *transform)) {
      ++synchronized;
    } else {
      context.world.markTransformDirty(primId);
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
  std::unique_ptr<PhysicsWorld> physicsWorld;
  std::unique_ptr<PhysicsRuntime> physicsRuntime;
  PhysicsImportSummary physicsImport;
  const std::size_t declaredPhysicsBodies = physicsDeclarationCount(context.stage);
  if (declaredPhysicsBodies != 0) {
    if (!usd_stage_runner::physics_jolt::isJoltPhysicsAvailable()) {
      throw std::runtime_error("Stage declares physics bodies, but Jolt Physics is unavailable "
                               "in this build");
    }
    physicsWorld = usd_stage_runner::physics_jolt::createJoltPhysicsWorld();
    physicsRuntime = std::make_unique<PhysicsRuntime>(*physicsWorld, context.world);
    physicsImport = importPhysicsBodies(context, *physicsWorld, *physicsRuntime);
  }
  const FixedStepAccumulator::Duration fixedStep{options.fixedDt};
  FixedStepAccumulator accumulator(fixedStep, options.maxFixedSteps);
  FrameClock clock;
  std::size_t fixedSteps = 0;
  std::size_t droppedSteps = 0;
  std::size_t synchronizedTransforms = 0;
  std::size_t synchronizedPhysicsBodies = 0;
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
      if (physicsRuntime) {
        const bool playerHasPhysicsBody =
            static_cast<bool>(physicsRuntime->bodyForPrim(playerPrim));
        if (playerHasPhysicsBody) {
          (void)applyMovementIntentToPhysics(context.world, playerPrim, *physicsWorld,
                                             *physicsRuntime, 3.0);
        } else if (context.world.containsPrim(playerPrim)) {
          applyMovementIntent(context.world, playerPrim, 3.0, step.count());
        }
        synchronizedPhysicsBodies += physicsRuntime->step(step);
      } else if (context.world.containsPrim(playerPrim)) {
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
    std::cout << ", player_translation=(" << value.x << ", " << value.y << ", " << value.z << ')';
  }
  std::cout << ", physics_shapes=" << physicsImport.shapeCount
            << ", physics_bodies=" << physicsImport.bodyCount
            << ", physics_body_updates=" << synchronizedPhysicsBodies;
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
