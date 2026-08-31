#include "usd_stage_runner/character/character_controller.h"
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
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>
#endif

#if USD_STAGE_RUNNER_HAS_OPENUSD && defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
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
const pxr::TfToken physicsBodySchema{"RunnerPhysicsBodyAPI"};
const pxr::TfToken colliderSchema{"RunnerColliderAPI"};
const pxr::TfToken characterSchema{"RunnerCharacterAPI"};
const pxr::TfToken physicsMotionTypeAttribute{"runner:physics:motionType"};
const pxr::TfToken physicsMassAttribute{"runner:physics:mass"};
const pxr::TfToken physicsShapeAttribute{"runner:physics:shape"};
const pxr::TfToken physicsHalfExtentsAttribute{"runner:physics:halfExtents"};
const pxr::TfToken characterGroundProbeDistanceAttribute{"runner:character:groundProbeDistance"};
const pxr::TfToken characterMaximumSlopeAngleAttribute{"runner:character:maximumSlopeAngleRadians"};
const pxr::TfToken characterJumpSpeedAttribute{"runner:character:jumpSpeed"};
#endif

struct Options {
  std::filesystem::path executablePath;
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
  options.executablePath = argv[0];
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

struct CharacterImportSummary {
  std::size_t controllerCount{0};
};

std::filesystem::path executableDirectory(const std::filesystem::path& invocation) {
#ifdef _WIN32
  std::wstring buffer(32768, L'\0');
  const auto length =
      GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
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
  const auto resourcePath = executableDirectory(executablePath) /
                            USD_STAGE_RUNNER_SCHEMA_RESOURCE_RELATIVE_PATH;
  if (!std::filesystem::is_regular_file(resourcePath / "plugInfo.json")) {
    throw std::runtime_error("runnerSchema resources were not found at " +
                             resourcePath.lexically_normal().string());
  }
  (void)pxr::PlugRegistry::GetInstance().RegisterPlugins(resourcePath.string());
#endif
}

bool hasAppliedSchema(const pxr::UsdPrim& prim, const pxr::TfToken& schema) {
  const auto& schemas = prim.GetAppliedSchemas();
  return std::find(schemas.begin(), schemas.end(), schema) != schemas.end();
}

bool hasAuthoredAttribute(const pxr::UsdPrim& prim, const pxr::TfToken& name) {
  const auto attribute = prim.GetAttribute(name);
  return attribute && attribute.HasAuthoredValue();
}

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

StageContext openWorld(const std::filesystem::path& stagePath,
                       const std::filesystem::path& executablePath) {
  registerRunnerSchemas(executablePath);
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
  if (!hasAppliedSchema(prim, physicsBodySchema)) {
    return std::nullopt;
  }
  const auto attribute = prim.GetAttribute(physicsMotionTypeAttribute);
  if (!attribute) {
    throw std::runtime_error(physicsBodySchema.GetString() + " on " +
                             prim.GetPath().GetString() + " does not provide " +
                             physicsMotionTypeAttribute.GetString());
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

usd_stage_runner::runtime::Vec3d readBoxHalfExtents(const pxr::UsdPrim& prim,
                                                   const pxr::UsdGeomXformable& xformable) {
  pxr::TfToken shape;
  const auto shapeAttribute = prim.GetAttribute(physicsShapeAttribute);
  if (!shapeAttribute || !shapeAttribute.Get(&shape)) {
    throw std::runtime_error("could not read " + physicsShapeAttribute.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  if (shape != pxr::TfToken{"box"}) {
    throw std::runtime_error(physicsShapeAttribute.GetString() + " on " +
                             prim.GetPath().GetString() + " must be 'box'");
  }

  pxr::GfVec3d halfExtents;
  const auto halfExtentsAttribute = prim.GetAttribute(physicsHalfExtentsAttribute);
  if (!halfExtentsAttribute || !halfExtentsAttribute.Get(&halfExtents)) {
    throw std::runtime_error("could not read " + physicsHalfExtentsAttribute.GetString() +
                             " on " + prim.GetPath().GetString());
  }
  for (int axis = 0; axis < 3; ++axis) {
    if (!std::isfinite(halfExtents[axis]) || halfExtents[axis] <= 0.0) {
      throw std::runtime_error(physicsHalfExtentsAttribute.GetString() + " on " +
                               prim.GetPath().GetString() +
                               " must contain positive finite values");
    }
  }

  pxr::GfVec3d scale{1.0};
  bool resetsStack = false;
  for (const auto& operation : xformable.GetOrderedXformOps(&resetsStack)) {
    if (operation.GetOpType() == pxr::UsdGeomXformOp::TypeTranslate) {
      continue;
    }
    if (operation.GetOpType() != pxr::UsdGeomXformOp::TypeScale) {
      throw std::runtime_error("physics schema import supports only translate and scale ops: " +
                               prim.GetPath().GetString());
    }
    pxr::GfVec3d operationScale;
    if (!operation.GetAs(&operationScale, pxr::UsdTimeCode::Default())) {
      throw std::runtime_error("could not read scale op on " + prim.GetPath().GetString());
    }
    scale[0] *= operationScale[0];
    scale[1] *= operationScale[1];
    scale[2] *= operationScale[2];
  }

  return {std::abs(scale[0]) * halfExtents[0], std::abs(scale[1]) * halfExtents[1],
          std::abs(scale[2]) * halfExtents[2]};
}

void validatePhysicsTransform(const pxr::UsdPrim& prim,
                              const pxr::UsdGeomXformable& xformable) {
  bool resetsStack = false;
  const auto operations = xformable.GetOrderedXformOps(&resetsStack);
  if (operations.empty() || operations.front().IsInverseOp() ||
      operations.front().GetOpType() != pxr::UsdGeomXformOp::TypeTranslate) {
    throw std::runtime_error(
        "physics schema import requires one translate op before any scale ops: " +
        prim.GetPath().GetString());
  }
  pxr::GfVec3d translation;
  if (!operations.front().GetAs(&translation, pxr::UsdTimeCode::Default())) {
    throw std::runtime_error("could not read translate op on " + prim.GetPath().GetString());
  }
  for (std::size_t index = 1; index < operations.size(); ++index) {
    if (operations[index].IsInverseOp() ||
        operations[index].GetOpType() != pxr::UsdGeomXformOp::TypeScale) {
      throw std::runtime_error(
          "physics schema import requires one translate op followed only by scale ops: " +
          prim.GetPath().GetString());
    }
  }

  if (resetsStack) {
    return;
  }
  for (auto ancestor = prim.GetParent(); ancestor && !ancestor.IsPseudoRoot();
       ancestor = ancestor.GetParent()) {
    const pxr::UsdGeomXformable ancestorTransform(ancestor);
    if (ancestorTransform &&
        !pxr::GfIsClose(ancestorTransform.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default()),
                        pxr::GfMatrix4d{1.0}, 1e-9)) {
      throw std::runtime_error(
          "physics schema import requires identity parent transforms or resetXformStack: " +
          prim.GetPath().GetString());
    }
  }
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

std::size_t validateDeclarationsAndCountPhysicsBodies(const pxr::UsdStageRefPtr& stage) {
  std::size_t count = 0;
  for (const auto& prim : stage->Traverse()) {
    const bool hasBodySchema = hasAppliedSchema(prim, physicsBodySchema);
    const bool hasColliderSchema = hasAppliedSchema(prim, colliderSchema);
    const bool hasCharacterSchema = hasAppliedSchema(prim, characterSchema);
    if (!hasBodySchema &&
        (hasAuthoredAttribute(prim, physicsMotionTypeAttribute) ||
         hasAuthoredAttribute(prim, physicsMassAttribute))) {
      throw std::runtime_error("authored body attributes require RunnerPhysicsBodyAPI: " +
                               prim.GetPath().GetString());
    }
    if (!hasColliderSchema &&
        (hasAuthoredAttribute(prim, physicsShapeAttribute) ||
         hasAuthoredAttribute(prim, physicsHalfExtentsAttribute))) {
      throw std::runtime_error("authored collider attributes require RunnerColliderAPI: " +
                               prim.GetPath().GetString());
    }
    if (hasBodySchema != hasColliderSchema) {
      throw std::runtime_error("physics prim must apply both RunnerPhysicsBodyAPI and "
                               "RunnerColliderAPI: " +
                               prim.GetPath().GetString());
    }
    if (!hasCharacterSchema && (hasAuthoredAttribute(prim, characterGroundProbeDistanceAttribute) ||
                                hasAuthoredAttribute(prim, characterMaximumSlopeAngleAttribute) ||
                                hasAuthoredAttribute(prim, characterJumpSpeedAttribute))) {
      throw std::runtime_error("authored character attributes require RunnerCharacterAPI: " +
                               prim.GetPath().GetString());
    }
    if (hasCharacterSchema && (!hasBodySchema || !hasColliderSchema)) {
      throw std::runtime_error(
          "character prim must apply RunnerPhysicsBodyAPI and RunnerColliderAPI: " +
          prim.GetPath().GetString());
    }
    if (hasCharacterSchema && readMotionType(prim) != MotionType::dynamicBody) {
      throw std::runtime_error("RunnerCharacterAPI requires a dynamic physics body: " +
                               prim.GetPath().GetString());
    }
    if (hasBodySchema || hasColliderSchema) {
      ++count;
    }
  }
  return count;
}

double readCharacterAttribute(const pxr::UsdPrim& prim, const pxr::TfToken& name) {
  const auto attribute = prim.GetAttribute(name);
  double value = 0.0;
  if (!attribute || !attribute.Get(&value)) {
    throw std::runtime_error("could not read " + name.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  return value;
}

PhysicsImportSummary importPhysicsBodies(StageContext& context, PhysicsWorld& physicsWorld,
                                         PhysicsRuntime& physicsRuntime) {
  if (pxr::UsdGeomGetStageUpAxis(context.stage) != pxr::UsdGeomTokens->y) {
    throw std::runtime_error("physics schema import requires a Y-up Stage");
  }
  if (!pxr::UsdGeomLinearUnitsAre(pxr::UsdGeomGetStageMetersPerUnit(context.stage),
                                  pxr::UsdGeomLinearUnits::meters)) {
    throw std::runtime_error("physics schema import requires metersPerUnit = 1");
  }
  PhysicsImportSummary summary;
  for (const auto& prim : context.stage->Traverse()) {
    const bool hasBodySchema = hasAppliedSchema(prim, physicsBodySchema);
    const bool hasColliderSchema = hasAppliedSchema(prim, colliderSchema);
    if (!hasBodySchema && !hasColliderSchema) {
      continue;
    }
    if (!hasBodySchema || !hasColliderSchema) {
      throw std::runtime_error("physics prim must apply both RunnerPhysicsBodyAPI and "
                               "RunnerColliderAPI: " +
                               prim.GetPath().GetString());
    }
    const auto motionType = readMotionType(prim);
    const pxr::UsdGeomXformable xformable(prim);
    if (!xformable) {
      throw std::runtime_error("physics schema import requires an xformable prim: " +
                               prim.GetPath().GetString());
    }
    validatePhysicsTransform(prim, xformable);
    const auto primId = prim.GetPath().GetString();
    const auto shape = physicsWorld.createShape(
        ShapeDescriptor{usd_stage_runner::physics::ShapeType::box,
                        readBoxHalfExtents(prim, xformable)});
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

CharacterImportSummary importCharacters(StageContext& context, PhysicsWorld& physicsWorld,
                                        PhysicsRuntime& physicsRuntime) {
  CharacterImportSummary summary;
  const auto* groundQuery =
      dynamic_cast<const usd_stage_runner::physics::GroundQuery*>(&physicsWorld);
  for (const auto& prim : context.stage->Traverse()) {
    if (!hasAppliedSchema(prim, characterSchema)) {
      continue;
    }
    if (groundQuery == nullptr) {
      throw std::runtime_error("character schema import requires a physics ground query");
    }

    const auto primId = prim.GetPath().GetString();
    const auto body = physicsRuntime.bodyForPrim(primId);
    if (!body) {
      throw std::runtime_error("character schema import requires an imported physics body: " +
                               primId);
    }
    const usd_stage_runner::character::CharacterControllerConfig config{
        readCharacterAttribute(prim, characterGroundProbeDistanceAttribute),
        readCharacterAttribute(prim, characterMaximumSlopeAngleAttribute),
        readCharacterAttribute(prim, characterJumpSpeedAttribute)};
    context.world.emplaceComponent<usd_stage_runner::character::CharacterController>(
        primId, body, physicsWorld, *groundQuery, config);
    ++summary.controllerCount;
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
  StageContext context = openWorld(options.stagePath, options.executablePath);
  std::unique_ptr<PhysicsWorld> physicsWorld;
  std::unique_ptr<PhysicsRuntime> physicsRuntime;
  PhysicsImportSummary physicsImport;
  CharacterImportSummary characterImport;
  const std::size_t declaredPhysicsBodies =
      validateDeclarationsAndCountPhysicsBodies(context.stage);
  if (declaredPhysicsBodies != 0) {
    if (!usd_stage_runner::physics_jolt::isJoltPhysicsAvailable()) {
      throw std::runtime_error("Stage declares physics bodies, but Jolt Physics is unavailable "
                               "in this build");
    }
    physicsWorld = usd_stage_runner::physics_jolt::createJoltPhysicsWorld();
    physicsRuntime = std::make_unique<PhysicsRuntime>(*physicsWorld, context.world);
    physicsImport = importPhysicsBodies(context, *physicsWorld, *physicsRuntime);
    characterImport = importCharacters(context, *physicsWorld, *physicsRuntime);
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
            << ", physics_body_updates=" << synchronizedPhysicsBodies
            << ", character_controllers=" << characterImport.controllerCount;
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
