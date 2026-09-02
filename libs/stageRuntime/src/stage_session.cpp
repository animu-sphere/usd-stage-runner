#include "usd_stage_runner/stage/stage_session.h"

#include "usd_stage_runner/camera/camera_rig.h"
#include "usd_stage_runner/character/character_controller.h"
#include "usd_stage_runner/input/movement_controller.h"
#include "usd_stage_runner/physics/collision_query.h"
#include "usd_stage_runner/physics/ground_query.h"
#include "usd_stage_runner/physics/physics_runtime.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/quatd.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace usd_stage_runner::stage {
namespace {

using physics::BodyDescriptor;
using physics::MotionType;
using physics::PhysicsRuntime;
using physics::PhysicsWorld;
using physics::ShapeDescriptor;
using runtime::PrimId;
using runtime::RuntimeTransform;
using runtime::RuntimeWorld;

const pxr::TfToken physicsBodySchema{"RunnerPhysicsBodyAPI"};
const pxr::TfToken colliderSchema{"RunnerColliderAPI"};
const pxr::TfToken characterSchema{"RunnerCharacterAPI"};
const pxr::TfToken cameraRigSchema{"RunnerCameraRigAPI"};
const pxr::TfToken physicsMotionTypeAttribute{"runner:physics:motionType"};
const pxr::TfToken physicsMassAttribute{"runner:physics:mass"};
const pxr::TfToken physicsShapeAttribute{"runner:physics:shape"};
const pxr::TfToken physicsHalfExtentsAttribute{"runner:physics:halfExtents"};
const pxr::TfToken characterGroundProbeDistanceAttribute{"runner:character:groundProbeDistance"};
const pxr::TfToken characterMaximumSlopeAngleAttribute{"runner:character:maximumSlopeAngleRadians"};
const pxr::TfToken characterJumpSpeedAttribute{"runner:character:jumpSpeed"};
const pxr::TfToken cameraTargetRelationship{"runner:camera:target"};
const pxr::TfToken cameraAnchorRelationship{"runner:camera:anchor"};
const pxr::TfToken cameraModeAttribute{"runner:camera:mode"};
const pxr::TfToken cameraOffsetAttribute{"runner:camera:offset"};
const pxr::TfToken cameraDistanceAttribute{"runner:camera:distance"};
const pxr::TfToken cameraPitchAttribute{"runner:camera:pitchRadians"};
const pxr::TfToken cameraYawAttribute{"runner:camera:yawRadians"};
const pxr::TfToken cameraDampingAttribute{"runner:camera:damping"};
const pxr::TfToken cameraCollisionEnabledAttribute{"runner:camera:collisionEnabled"};
const pxr::TfToken cameraCollisionClearanceAttribute{"runner:camera:collisionClearance"};
const pxr::TfToken cameraRuntimeOrientationSuffix{"runnerCamera"};
const pxr::TfToken cameraRuntimeOrientationOp{"xformOp:orient:runnerCamera"};

struct SessionState {
  // The runtime world can contain components that reference the physics world,
  // so declaration order intentionally destroys the world first.
  std::unique_ptr<PhysicsWorld> physicsWorld;
  RuntimeWorld world;
  std::unique_ptr<PhysicsRuntime> physicsRuntime;
  std::vector<PrimId> cameraPrims;
};

bool hasAppliedSchema(const pxr::UsdPrim& prim, const pxr::TfToken& schema) {
  const auto& schemas = prim.GetAppliedSchemas();
  return std::find(schemas.begin(), schemas.end(), schema) != schemas.end();
}

bool hasAuthoredAttribute(const pxr::UsdPrim& prim, const pxr::TfToken& name) {
  const auto attribute = prim.GetAttribute(name);
  return attribute && attribute.HasAuthoredValue();
}

bool hasRelationship(const pxr::UsdPrim& prim, const pxr::TfToken& name) {
  return static_cast<bool>(prim.GetRelationship(name));
}

RuntimeTransform readTransform(const pxr::UsdPrim& prim) {
  RuntimeTransform transform;
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

std::optional<MotionType> readMotionType(const pxr::UsdPrim& prim) {
  if (!hasAppliedSchema(prim, physicsBodySchema)) {
    return std::nullopt;
  }
  const auto attribute = prim.GetAttribute(physicsMotionTypeAttribute);
  if (!attribute) {
    throw std::runtime_error(physicsBodySchema.GetString() + " on " + prim.GetPath().GetString() +
                             " does not provide " + physicsMotionTypeAttribute.GetString());
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

runtime::Vec3d readBoxHalfExtents(const pxr::UsdPrim& prim,
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
    throw std::runtime_error("could not read " + physicsHalfExtentsAttribute.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  for (int axis = 0; axis < 3; ++axis) {
    if (!std::isfinite(halfExtents[axis]) || halfExtents[axis] <= 0.0) {
      throw std::runtime_error(physicsHalfExtentsAttribute.GetString() + " on " +
                               prim.GetPath().GetString() + " must contain positive finite values");
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

void validatePhysicsTransform(const pxr::UsdPrim& prim, const pxr::UsdGeomXformable& xformable) {
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
    const bool hasCameraSchema = hasAppliedSchema(prim, cameraRigSchema);
    if (!hasBodySchema && (hasAuthoredAttribute(prim, physicsMotionTypeAttribute) ||
                           hasAuthoredAttribute(prim, physicsMassAttribute))) {
      throw std::runtime_error("authored body attributes require RunnerPhysicsBodyAPI: " +
                               prim.GetPath().GetString());
    }
    if (!hasColliderSchema && (hasAuthoredAttribute(prim, physicsShapeAttribute) ||
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
    if (!hasCameraSchema && (hasRelationship(prim, cameraTargetRelationship) ||
                             hasRelationship(prim, cameraAnchorRelationship) ||
                             hasAuthoredAttribute(prim, cameraModeAttribute) ||
                             hasAuthoredAttribute(prim, cameraOffsetAttribute) ||
                             hasAuthoredAttribute(prim, cameraDistanceAttribute) ||
                             hasAuthoredAttribute(prim, cameraPitchAttribute) ||
                             hasAuthoredAttribute(prim, cameraYawAttribute) ||
                             hasAuthoredAttribute(prim, cameraDampingAttribute) ||
                             hasAuthoredAttribute(prim, cameraCollisionEnabledAttribute) ||
                             hasAuthoredAttribute(prim, cameraCollisionClearanceAttribute))) {
      throw std::runtime_error("authored camera rig properties require RunnerCameraRigAPI: " +
                               prim.GetPath().GetString());
    }
    if (hasBodySchema || hasColliderSchema) {
      ++count;
    }
  }
  return count;
}

camera::CameraRigMode readCameraMode(const pxr::UsdPrim& prim) {
  pxr::TfToken value;
  const auto attribute = prim.GetAttribute(cameraModeAttribute);
  if (!attribute || !attribute.Get(&value)) {
    throw std::runtime_error("could not read " + cameraModeAttribute.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  if (value == pxr::TfToken{"free"}) {
    return camera::CameraRigMode::free;
  }
  if (value == pxr::TfToken{"firstPerson"}) {
    return camera::CameraRigMode::firstPerson;
  }
  if (value == pxr::TfToken{"thirdPerson"}) {
    return camera::CameraRigMode::thirdPerson;
  }
  if (value == pxr::TfToken{"orbit"}) {
    return camera::CameraRigMode::orbit;
  }
  throw std::runtime_error(cameraModeAttribute.GetString() + " on " + prim.GetPath().GetString() +
                           " must be 'free', 'firstPerson', 'thirdPerson', or 'orbit'");
}

double readCameraDouble(const pxr::UsdPrim& prim, const pxr::TfToken& name) {
  double value = 0.0;
  const auto attribute = prim.GetAttribute(name);
  if (!attribute || !attribute.Get(&value)) {
    throw std::runtime_error("could not read " + name.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  return value;
}

bool readCameraBool(const pxr::UsdPrim& prim, const pxr::TfToken& name) {
  bool value = false;
  const auto attribute = prim.GetAttribute(name);
  if (!attribute || !attribute.Get(&value)) {
    throw std::runtime_error("could not read " + name.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  return value;
}

runtime::Vec3d readCameraOffset(const pxr::UsdPrim& prim) {
  pxr::GfVec3d value;
  const auto attribute = prim.GetAttribute(cameraOffsetAttribute);
  if (!attribute || !attribute.Get(&value)) {
    throw std::runtime_error("could not read " + cameraOffsetAttribute.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  return {value[0], value[1], value[2]};
}

void validateCameraWorldTranslation(const pxr::UsdPrim& prim, const std::string& role) {
  const pxr::UsdGeomXformable xformable(prim);
  if (!xformable) {
    throw std::runtime_error("camera schema import requires an xformable " + role + ": " +
                             prim.GetPath().GetString());
  }

  const auto localTransform = readTransform(prim);
  const pxr::GfVec3d worldTranslation =
      xformable.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default()).ExtractTranslation();
  const auto matches = [](double worldValue, double runtimeValue) {
    constexpr double relativeTolerance = 1.0e-9;
    const double scale = std::max({1.0, std::abs(worldValue), std::abs(runtimeValue)});
    return std::isfinite(worldValue) && std::isfinite(runtimeValue) &&
           std::abs(worldValue - runtimeValue) <= relativeTolerance * scale;
  };
  if (!matches(worldTranslation[0], localTransform.translation.x) ||
      !matches(worldTranslation[1], localTransform.translation.y) ||
      !matches(worldTranslation[2], localTransform.translation.z)) {
    throw std::runtime_error(
        "camera schema import requires local RuntimeTransform translation to match world "
        "translation for " +
        role +
        " (use identity parent transforms or resetXformStack): " + prim.GetPath().GetString());
  }

  bool resetsStack = false;
  (void)xformable.GetOrderedXformOps(&resetsStack);
  if (!resetsStack &&
      !pxr::GfIsClose(xformable.ComputeParentToWorldTransform(pxr::UsdTimeCode::Default()),
                      pxr::GfMatrix4d{1.0}, 1e-9)) {
    throw std::runtime_error(
        "camera schema import requires identity parent transforms or resetXformStack for " + role +
        ": " + prim.GetPath().GetString());
  }
}

void validateCameraTransformStack(const pxr::UsdPrim& prim) {
  const pxr::UsdGeomXformable xformable(prim);
  bool resetsStack = false;
  const auto operations = xformable.GetOrderedXformOps(&resetsStack);
  bool hasTranslate = false;
  bool hasRuntimeOrientation = false;
  for (const auto& operation : operations) {
    if (!operation.IsInverseOp() && !hasTranslate &&
        operation.GetOpType() == pxr::UsdGeomXformOp::TypeTranslate) {
      hasTranslate = true;
      continue;
    }
    if (!operation.IsInverseOp() && hasTranslate && !hasRuntimeOrientation &&
        operation.GetOpType() == pxr::UsdGeomXformOp::TypeOrient &&
        operation.GetOpName() == cameraRuntimeOrientationOp &&
        operation.GetPrecision() == pxr::UsdGeomXformOp::PrecisionDouble) {
      hasRuntimeOrientation = true;
      continue;
    }
    throw std::runtime_error(
        "camera schema import supports an empty transform stack or one translate op "
        "optionally followed by the double-precision runnerCamera orient op: " +
        prim.GetPath().GetString());
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
          "camera schema import requires identity parent transforms or resetXformStack: " +
          prim.GetPath().GetString());
    }
  }
}

std::optional<PrimId> readCameraPrimRelationship(const pxr::UsdStageRefPtr& stage,
                                                 const RuntimeWorld& world,
                                                 const pxr::UsdPrim& prim, const pxr::TfToken& name,
                                                 bool required) {
  const auto relationship = prim.GetRelationship(name);
  pxr::SdfPathVector targets;
  if (relationship && relationship.HasAuthoredTargets() && !relationship.GetTargets(&targets)) {
    throw std::runtime_error("could not read " + name.GetString() + " on " +
                             prim.GetPath().GetString());
  }
  if (targets.empty()) {
    if (required) {
      throw std::runtime_error(name.GetString() + " on " + prim.GetPath().GetString() +
                               " must target exactly one prim");
    }
    return std::nullopt;
  }
  if (targets.size() != 1 || !targets.front().IsAbsolutePath() || !targets.front().IsPrimPath()) {
    throw std::runtime_error(name.GetString() + " on " + prim.GetPath().GetString() +
                             " must target exactly one absolute prim path");
  }

  const auto target = targets.front().GetString();
  if (!world.containsPrim(target)) {
    throw std::runtime_error(name.GetString() + " on " + prim.GetPath().GetString() +
                             " does not resolve to a Runtime World prim: " + target);
  }
  if (world.transform(target) == nullptr) {
    throw std::runtime_error(name.GetString() + " on " + prim.GetPath().GetString() +
                             " must target an xformable prim: " + target);
  }
  validateCameraWorldTranslation(stage->GetPrimAtPath(targets.front()), name.GetString());
  return target;
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

void importPhysicsBodies(const pxr::UsdStageRefPtr& stage, SessionState& state,
                         const StageSessionConfig& config, StageSessionStats& stats) {
  if (pxr::UsdGeomGetStageUpAxis(stage) != pxr::UsdGeomTokens->y) {
    throw std::runtime_error("physics schema import requires a Y-up Stage");
  }
  if (!pxr::UsdGeomLinearUnitsAre(pxr::UsdGeomGetStageMetersPerUnit(stage),
                                  pxr::UsdGeomLinearUnits::meters)) {
    throw std::runtime_error("physics schema import requires metersPerUnit = 1");
  }
  for (const auto& prim : stage->Traverse()) {
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
    const auto shape = state.physicsWorld->createShape(
        ShapeDescriptor{physics::ShapeType::box, readBoxHalfExtents(prim, xformable)});
    ++stats.physicsShapeCount;
    const bool dynamic = *motionType == MotionType::dynamicBody;
    const auto body = state.physicsWorld->createBody(BodyDescriptor{
        shape, *motionType, *state.world.transform(primId), readBodyMass(prim, *motionType),
        dynamic ? config.dynamicCollisionLayer : config.staticCollisionLayer});
    state.physicsRuntime->bindBody(primId, body);
    ++stats.physicsBodyCount;
  }
}

void importCharacters(const pxr::UsdStageRefPtr& stage, SessionState& state,
                      StageSessionStats& stats) {
  const auto* groundQuery = dynamic_cast<const physics::GroundQuery*>(state.physicsWorld.get());
  for (const auto& prim : stage->Traverse()) {
    if (!hasAppliedSchema(prim, characterSchema)) {
      continue;
    }
    if (groundQuery == nullptr) {
      throw std::runtime_error("character schema import requires a physics ground query");
    }

    const auto primId = prim.GetPath().GetString();
    const auto body = state.physicsRuntime->bodyForPrim(primId);
    if (!body) {
      throw std::runtime_error("character schema import requires an imported physics body: " +
                               primId);
    }
    const character::CharacterControllerConfig controllerConfig{
        readCharacterAttribute(prim, characterGroundProbeDistanceAttribute),
        readCharacterAttribute(prim, characterMaximumSlopeAngleAttribute),
        readCharacterAttribute(prim, characterJumpSpeedAttribute)};
    state.world.emplaceComponent<character::CharacterController>(primId, body, *state.physicsWorld,
                                                                 *groundQuery, controllerConfig);
    ++stats.characterControllerCount;
  }
}

void importCameraRigs(const pxr::UsdStageRefPtr& stage, SessionState& state,
                      StageSessionStats& stats) {
  bool coordinateSystemValidated = false;
  for (const auto& prim : stage->Traverse()) {
    if (!hasAppliedSchema(prim, cameraRigSchema)) {
      continue;
    }
    if (!coordinateSystemValidated) {
      if (pxr::UsdGeomGetStageUpAxis(stage) != pxr::UsdGeomTokens->y) {
        throw std::runtime_error("camera schema import requires a Y-up Stage");
      }
      coordinateSystemValidated = true;
    }
    if (!pxr::UsdGeomCamera(prim)) {
      throw std::runtime_error("RunnerCameraRigAPI requires a UsdGeomCamera prim: " +
                               prim.GetPath().GetString());
    }

    const auto primId = prim.GetPath().GetString();
    if (state.world.transform(primId) == nullptr) {
      throw std::runtime_error("camera schema import requires a runtime transform: " + primId);
    }
    validateCameraTransformStack(prim);
    validateCameraWorldTranslation(prim, "camera prim");

    const auto mode = readCameraMode(prim);
    const bool targetRequired = mode != camera::CameraRigMode::free;
    const camera::CameraRigConfig rigConfig{
        mode,
        readCameraPrimRelationship(stage, state.world, prim, cameraTargetRelationship,
                                   targetRequired)
            .value_or(PrimId{}),
        readCameraPrimRelationship(stage, state.world, prim, cameraAnchorRelationship, false),
        readCameraOffset(prim),
        readCameraDouble(prim, cameraDistanceAttribute),
        readCameraDouble(prim, cameraPitchAttribute),
        readCameraDouble(prim, cameraYawAttribute),
        readCameraDouble(prim, cameraDampingAttribute),
        readCameraBool(prim, cameraCollisionEnabledAttribute),
        readCameraDouble(prim, cameraCollisionClearanceAttribute)};
    if (rigConfig.target == primId ||
        (rigConfig.anchor.has_value() && *rigConfig.anchor == primId)) {
      throw std::runtime_error(
          "camera rig target and anchor must not reference the camera itself: " + primId);
    }
    try {
      state.world.emplaceComponent<camera::CameraRig>(primId, rigConfig);
    } catch (const std::invalid_argument& error) {
      throw std::runtime_error("invalid camera rig configuration on " + primId + ": " +
                               error.what());
    }
    state.cameraPrims.push_back(primId);
    ++stats.cameraRigCount;
  }
}

std::size_t updateCameraRigs(SessionState& state, camera::CameraRig::Duration elapsed) {
  const auto* collisionQuery =
      dynamic_cast<const physics::CollisionQuery*>(state.physicsWorld.get());
  std::size_t updated = 0;
  for (const auto& prim : state.cameraPrims) {
    const auto* rig = state.world.component<camera::CameraRig>(prim);
    camera::CameraCollisionProbe probe;
    if (rig != nullptr && rig->config().mode == camera::CameraRigMode::thirdPerson &&
        rig->config().collisionEnabled && rig->config().distance > 0.0) {
      if (collisionQuery == nullptr) {
        throw std::runtime_error(
            "collision-enabled camera rig requires a physics collision query: " + prim);
      }
      probe = [collisionQuery, physicsRuntime = state.physicsRuntime.get()](
                  const auto& origin, const auto& desired,
                  const auto& target) -> std::optional<double> {
        const auto ignoredBody =
            physicsRuntime == nullptr ? physics::BodyHandle{} : physicsRuntime->bodyForPrim(target);
        const auto hit = collisionQuery->segmentHit(origin, desired, ignoredBody);
        return hit.has_value() ? std::optional<double>{hit->fraction} : std::nullopt;
      };
    }
    if (camera::updateCameraRig(state.world, prim, elapsed, probe)) {
      ++updated;
    }
  }
  return updated;
}

bool applyMovementIntentToPhysics(RuntimeWorld& world, const PrimId& prim,
                                  PhysicsWorld& physicsWorld, PhysicsRuntime& physicsRuntime,
                                  double speed) {
  const auto body = physicsRuntime.bodyForPrim(prim);
  const auto* intent = world.component<input::MovementIntent>(prim);
  if (!body || intent == nullptr) {
    return false;
  }
  const auto state = physicsWorld.bodyState(body);
  return physicsWorld.setLinearVelocity(
      body, {intent->x * speed, state.linearVelocity.y, intent->y * speed});
}

bool applyCharacterIntent(RuntimeWorld& world, const PrimId& prim,
                          const input::ActionState& actions,
                          character::CharacterController::Duration fixedStep, double speed) {
  auto* controller = world.component<character::CharacterController>(prim);
  const auto* movement = world.component<input::MovementIntent>(prim);
  if (controller == nullptr || movement == nullptr) {
    return false;
  }

  const runtime::Vec3d desiredVelocity{movement->x * speed, 0.0, movement->y * speed};
  return controller->update(character::CharacterIntent{desiredVelocity,
                                                       {desiredVelocity.x, 0.0, desiredVelocity.z},
                                                       actions.value(input::actions::jump) > 0.5},
                            fixedStep);
}

bool setTranslate(const pxr::UsdGeomXformOp& operation, const RuntimeTransform& transform) {
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

bool setCameraOrientation(const pxr::UsdGeomXformOp& operation, const camera::CameraRigPose& pose) {
  const auto& forward = pose.forward;
  const double pitch = std::asin(std::clamp(forward.y, -1.0, 1.0));
  const double yaw = std::atan2(forward.x, -forward.z);
  const double halfPitch = pitch * 0.5;
  const double halfNegativeYaw = yaw * -0.5;
  const double cosPitch = std::cos(halfPitch);
  const double sinPitch = std::sin(halfPitch);
  const double cosYaw = std::cos(halfNegativeYaw);
  const double sinYaw = std::sin(halfNegativeYaw);
  const pxr::GfQuatd orientation{
      cosYaw * cosPitch, pxr::GfVec3d{cosYaw * sinPitch, sinYaw * cosPitch, -sinYaw * sinPitch}};
  return operation.Set(orientation);
}

std::pair<std::size_t, std::size_t> synchronizeDirtyTransforms(const pxr::UsdStageRefPtr& stage,
                                                               RuntimeWorld& world) {
  std::size_t synchronizedTransforms = 0;
  std::size_t synchronizedCameraTransforms = 0;
  for (const auto& primId : world.takeDirtyTransforms()) {
    const auto* transform = world.transform(primId);
    const pxr::UsdPrim prim = stage->GetPrimAtPath(pxr::SdfPath(primId));
    pxr::UsdGeomXformable xformable(prim);
    if (transform == nullptr || !xformable) {
      if (transform != nullptr) {
        world.markTransformDirty(primId);
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
    bool synchronized = translate && setTranslate(translate, *transform);
    const auto* cameraRig = world.component<camera::CameraRig>(primId);
    if (synchronized && cameraRig != nullptr && cameraRig->state().initialized) {
      pxr::UsdGeomXformOp orientation;
      for (const auto& operation : xformable.GetOrderedXformOps(&resetsStack)) {
        if (operation.GetOpType() == pxr::UsdGeomXformOp::TypeOrient &&
            operation.GetOpName() == cameraRuntimeOrientationOp) {
          orientation = operation;
          break;
        }
      }
      if (!orientation) {
        orientation = xformable.AddOrientOp(pxr::UsdGeomXformOp::PrecisionDouble,
                                            cameraRuntimeOrientationSuffix);
      }
      synchronized = orientation && setCameraOrientation(orientation, cameraRig->state().current);
      if (synchronized) {
        ++synchronizedCameraTransforms;
      }
    }
    if (synchronized) {
      ++synchronizedTransforms;
    } else {
      world.markTransformDirty(primId);
    }
  }
  return {synchronizedTransforms, synchronizedCameraTransforms};
}

bool transformsEqual(const RuntimeTransform& left, const RuntimeTransform& right) noexcept {
  return left.translation.x == right.translation.x && left.translation.y == right.translation.y &&
         left.translation.z == right.translation.z;
}

} // namespace

class StageSession::Impl {
public:
  Impl(pxr::UsdStageRefPtr stage, StageSessionConfig config,
       PhysicsWorldFactory physicsWorldFactory)
      : stage_(std::move(stage)), config_(std::move(config)),
        physicsWorldFactory_(std::move(physicsWorldFactory)) {
    if (!stage_) {
      throw std::invalid_argument("stage play session requires an open USD Stage");
    }
    if (config_.playerPrim.empty() || config_.playerPrim.front() != '/') {
      throw std::invalid_argument("stage play-session player prim must be an absolute path");
    }
    if (!std::isfinite(config_.playerSpeed) || config_.playerSpeed < 0.0) {
      throw std::invalid_argument(
          "stage play-session player speed must be finite and non-negative");
    }

    captureInitialTransforms();
    rebuild(false);
    playSession_ =
        std::make_unique<runtime::PlaySession>(runtime::PlaySession::Callbacks{
                                                   [this] { rebuild(true); },
                                                   [this](const auto step) { fixedUpdate(step); },
                                                   [this] { synchronize(); },
                                               },
                                               config_.fixedStep, config_.maxFixedStepsPerFrame);
    attachRuntimeLayer();
  }

  ~Impl() {
    detachRuntimeLayer();
  }

  void setActions(input::ActionState actions) {
    actions_ = std::move(actions);
    updateMovementIntent();
  }

  void captureInitialTransforms() {
    for (const auto& prim : stage_->Traverse()) {
      if (pxr::UsdGeomXformable(prim)) {
        initialTransforms_.insert_or_assign(prim.GetPath().GetString(), readTransform(prim));
      }
    }
  }

  void attachRuntimeLayer() {
    sessionLayer_ = stage_->GetSessionLayer();
    runtimeLayer_ = pxr::SdfLayer::CreateAnonymous("usd-stage-runner-runtime.usda");
    if (!sessionLayer_ || !runtimeLayer_) {
      throw std::runtime_error("could not create the Stage play-session runtime layer");
    }

    auto sublayers = sessionLayer_->GetSubLayerPaths();
    sublayers.insert(sublayers.begin(), runtimeLayer_->GetIdentifier());
    sessionLayer_->SetSubLayerPaths(sublayers);
    const auto attachedSublayers = sessionLayer_->GetSubLayerPaths();
    if (std::find(attachedSublayers.begin(), attachedSublayers.end(),
                  runtimeLayer_->GetIdentifier()) == attachedSublayers.end()) {
      runtimeLayer_.Reset();
      sessionLayer_.Reset();
      throw std::runtime_error("could not attach the Stage play-session runtime layer");
    }
  }

  void detachRuntimeLayer() noexcept {
    if (!sessionLayer_ || !runtimeLayer_) {
      return;
    }
    auto sublayers = sessionLayer_->GetSubLayerPaths();
    const auto runtimeLayerIdentifier = runtimeLayer_->GetIdentifier();
    const auto runtimeLayer = std::find(sublayers.begin(), sublayers.end(), runtimeLayerIdentifier);
    if (runtimeLayer != sublayers.end()) {
      sublayers.erase(runtimeLayer);
      (void)sessionLayer_->SetSubLayerPaths(sublayers);
    }
    runtimeLayer_.Reset();
    sessionLayer_.Reset();
  }

  void clearRuntimeLayer() {
    if (!runtimeLayer_) {
      throw std::runtime_error("Stage play-session runtime layer is unavailable");
    }
    runtimeLayer_->Clear();
  }

  void rebuild(bool restoreInitialState) {
    auto next = std::make_unique<SessionState>();
    StageSessionStats nextStats;
    for (const auto& prim : stage_->Traverse()) {
      const auto primId = prim.GetPath().GetString();
      next->world.addPrim(primId);
      if (pxr::UsdGeomXformable(prim)) {
        const auto authoredTransform = readTransform(prim);
        const auto initial = initialTransforms_.find(primId);
        const auto transform = restoreInitialState && initial != initialTransforms_.end()
                                   ? initial->second
                                   : authoredTransform;
        next->world.emplaceTransform(primId, transform);
        if (restoreInitialState && !transformsEqual(transform, authoredTransform)) {
          next->world.markTransformDirty(primId);
        }
      }
    }

    const auto declaredPhysicsBodies = validateDeclarationsAndCountPhysicsBodies(stage_);
    if (declaredPhysicsBodies != 0) {
      if (!physicsWorldFactory_) {
        throw std::runtime_error(
            "Stage declares physics bodies, but no physics world factory was provided");
      }
      next->physicsWorld = physicsWorldFactory_();
      if (!next->physicsWorld) {
        throw std::runtime_error(
            "Stage declares physics bodies, but the configured physics backend is unavailable");
      }
      next->physicsRuntime = std::make_unique<PhysicsRuntime>(*next->physicsWorld, next->world);
      importPhysicsBodies(stage_, *next, config_, nextStats);
      importCharacters(stage_, *next, nextStats);
    }
    importCameraRigs(stage_, *next, nextStats);

    state_ = std::move(next);
    stats_ = nextStats;
    updateMovementIntent();
    if (restoreInitialState) {
      stats_.cameraRigUpdates += updateCameraRigs(*state_, Duration{0.0});
    }
  }

  void updateMovementIntent() {
    if (state_ && state_->world.containsPrim(config_.playerPrim) &&
        state_->world.transform(config_.playerPrim) != nullptr) {
      input::updateMovementIntent(state_->world, config_.playerPrim, actions_);
    }
  }

  void fixedUpdate(Duration step) {
    ++stats_.fixedSteps;
    if (state_->physicsRuntime) {
      const bool playerHasPhysicsBody =
          static_cast<bool>(state_->physicsRuntime->bodyForPrim(config_.playerPrim));
      if (playerHasPhysicsBody) {
        const bool playerHasCharacter =
            state_->world.component<character::CharacterController>(config_.playerPrim) != nullptr;
        if (playerHasCharacter) {
          (void)applyCharacterIntent(state_->world, config_.playerPrim, actions_, step,
                                     config_.playerSpeed);
        } else {
          (void)applyMovementIntentToPhysics(state_->world, config_.playerPrim,
                                             *state_->physicsWorld, *state_->physicsRuntime,
                                             config_.playerSpeed);
        }
      } else if (state_->world.containsPrim(config_.playerPrim)) {
        input::applyMovementIntent(state_->world, config_.playerPrim, config_.playerSpeed,
                                   step.count());
      }
      stats_.physicsBodyUpdates += state_->physicsRuntime->step(step);
    } else if (state_->world.containsPrim(config_.playerPrim)) {
      input::applyMovementIntent(state_->world, config_.playerPrim, config_.playerSpeed,
                                 step.count());
    }
    stats_.cameraRigUpdates += updateCameraRigs(*state_, step);
  }

  void synchronize() {
    pxr::UsdEditContext runtimeEditContext(stage_, pxr::UsdEditTarget(runtimeLayer_));
    const auto synchronized = synchronizeDirtyTransforms(stage_, state_->world);
    stats_.synchronizedTransforms += synchronized.first;
    stats_.synchronizedCameraTransforms += synchronized.second;
  }

  pxr::UsdStageRefPtr stage_;
  pxr::SdfLayerHandle sessionLayer_;
  pxr::SdfLayerRefPtr runtimeLayer_;
  StageSessionConfig config_;
  PhysicsWorldFactory physicsWorldFactory_;
  input::ActionState actions_;
  std::unordered_map<PrimId, RuntimeTransform> initialTransforms_;
  std::unique_ptr<SessionState> state_;
  StageSessionStats stats_;
  std::unique_ptr<runtime::PlaySession> playSession_;
};

StageSession::StageSession(pxr::UsdStageRefPtr stage, StageSessionConfig config,
                           PhysicsWorldFactory physicsWorldFactory)
    : impl_(std::make_unique<Impl>(std::move(stage), std::move(config),
                                   std::move(physicsWorldFactory))) {}

StageSession::~StageSession() = default;
StageSession::StageSession(StageSession&&) noexcept = default;
StageSession& StageSession::operator=(StageSession&&) noexcept = default;

void StageSession::setActions(input::ActionState actions) {
  impl_->setActions(std::move(actions));
}

void StageSession::play() noexcept {
  impl_->playSession_->play();
}

void StageSession::pause() noexcept {
  impl_->playSession_->pause();
}

void StageSession::singleStep() {
  impl_->playSession_->singleStep();
}

void StageSession::reset() {
  impl_->clearRuntimeLayer();
  impl_->playSession_->reset();
}

void StageSession::stop() {
  impl_->playSession_->stop();
  impl_->clearRuntimeLayer();
  impl_->rebuild(false);
}

StageSession::AdvanceResult StageSession::advance(Duration frameTime) {
  const auto result = impl_->playSession_->advance(frameTime);
  impl_->stats_.droppedSteps += result.droppedSteps;
  return result;
}

runtime::PlaySession::State StageSession::state() const noexcept {
  return impl_->playSession_->state();
}

StageSession::Duration StageSession::fixedStep() const noexcept {
  return impl_->playSession_->fixedStep();
}

const StageSessionConfig& StageSession::config() const noexcept {
  return impl_->config_;
}

const StageSessionStats& StageSession::stats() const noexcept {
  return impl_->stats_;
}

RuntimeWorld& StageSession::world() noexcept {
  return impl_->state_->world;
}

const RuntimeWorld& StageSession::world() const noexcept {
  return impl_->state_->world;
}

const std::vector<PrimId>& StageSession::cameraPrims() const noexcept {
  return impl_->state_->cameraPrims;
}

} // namespace usd_stage_runner::stage
