#include "usd_stage_runner/camera/camera_rig.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace usd_stage_runner::camera {
namespace {

constexpr double halfPi = 1.57079632679489661923;
constexpr double vectorEpsilonSquared = 1.0e-24;

bool finite(const runtime::Vec3d& value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

runtime::Vec3d add(const runtime::Vec3d& left, const runtime::Vec3d& right) noexcept {
  return {left.x + right.x, left.y + right.y, left.z + right.z};
}

runtime::Vec3d subtract(const runtime::Vec3d& left,
                        const runtime::Vec3d& right) noexcept {
  return {left.x - right.x, left.y - right.y, left.z - right.z};
}

runtime::Vec3d multiply(const runtime::Vec3d& value, double scalar) noexcept {
  return {value.x * scalar, value.y * scalar, value.z * scalar};
}

double lengthSquared(const runtime::Vec3d& value) noexcept {
  return value.x * value.x + value.y * value.y + value.z * value.z;
}

double dot(const runtime::Vec3d& left, const runtime::Vec3d& right) noexcept {
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

runtime::Vec3d normalized(const runtime::Vec3d& value) {
  const double squared = lengthSquared(value);
  if (!std::isfinite(squared) || squared <= vectorEpsilonSquared) {
    throw std::invalid_argument("camera viewing direction must be finite and non-zero");
  }
  return multiply(value, 1.0 / std::sqrt(squared));
}

bool same(const runtime::Vec3d& left, const runtime::Vec3d& right) noexcept {
  return left.x == right.x && left.y == right.y && left.z == right.z;
}

runtime::Vec3d lerp(const runtime::Vec3d& from, const runtime::Vec3d& to,
                    double amount) noexcept {
  return add(from, multiply(subtract(to, from), amount));
}

runtime::Vec3d deterministicPerpendicular(const runtime::Vec3d& direction) {
  runtime::Vec3d axis;
  const double absoluteX = std::abs(direction.x);
  const double absoluteY = std::abs(direction.y);
  const double absoluteZ = std::abs(direction.z);
  if (absoluteX <= absoluteY && absoluteX <= absoluteZ) {
    axis = {1.0, 0.0, 0.0};
  } else if (absoluteY <= absoluteZ) {
    axis = {0.0, 1.0, 0.0};
  } else {
    axis = {0.0, 0.0, 1.0};
  }
  return normalized(subtract(axis, multiply(direction, dot(axis, direction))));
}

runtime::Vec3d interpolateDirection(const runtime::Vec3d& from,
                                    const runtime::Vec3d& to, double amount) {
  const runtime::Vec3d start = normalized(from);
  const runtime::Vec3d end = normalized(to);
  if (amount <= 0.0) {
    return start;
  }
  if (amount >= 1.0) {
    return end;
  }

  const double cosine = std::clamp(dot(start, end), -1.0, 1.0);
  if (cosine > 0.9995) {
    return normalized(lerp(start, end, amount));
  }

  runtime::Vec3d tangent = subtract(end, multiply(start, cosine));
  if (lengthSquared(tangent) <= vectorEpsilonSquared) {
    tangent = deterministicPerpendicular(start);
  } else {
    tangent = normalized(tangent);
  }
  const double angle = std::acos(cosine) * amount;
  return normalized(add(multiply(start, std::cos(angle)),
                        multiply(tangent, std::sin(angle))));
}

runtime::Vec3d viewDirection(double yaw, double pitch) noexcept {
  const double horizontal = std::cos(pitch);
  return {std::sin(yaw) * horizontal, std::sin(pitch),
          -std::cos(yaw) * horizontal};
}

const runtime::RuntimeTransform& requiredTransform(const runtime::RuntimeWorld& world,
                                                   const runtime::PrimId& prim,
                                                   const char* role) {
  if (!world.containsPrim(prim)) {
    throw std::out_of_range(std::string("camera rig ") + role +
                            " is not present in the Runtime World: " + prim);
  }
  const auto* transform = world.transform(prim);
  if (transform == nullptr) {
    throw std::out_of_range(std::string("camera rig ") + role +
                            " has no RuntimeTransform: " + prim);
  }
  if (!finite(transform->translation)) {
    throw std::invalid_argument(std::string("camera rig ") + role +
                                " transform must be finite");
  }
  return *transform;
}

CameraRigPose desiredPose(const CameraRigConfig& config,
                          const runtime::RuntimeWorld& world,
                          const runtime::RuntimeTransform& cameraTransform,
                          const CameraCollisionProbe& collisionProbe) {
  const runtime::Vec3d direction = viewDirection(config.yawRadians, config.pitchRadians);
  if (config.mode == CameraRigMode::free) {
    return {cameraTransform.translation, direction};
  }

  const auto& target = requiredTransform(world, config.target, "target");
  const auto& anchor = config.anchor.has_value()
                           ? requiredTransform(world, *config.anchor, "anchor")
                           : target;
  const runtime::Vec3d shiftedOrigin = add(anchor.translation, config.offset);

  switch (config.mode) {
  case CameraRigMode::free:
    break;
  case CameraRigMode::firstPerson:
    return {shiftedOrigin, direction};
  case CameraRigMode::thirdPerson: {
    CameraRigPose pose{
        subtract(shiftedOrigin, multiply(direction, config.distance)), direction};
    if (!config.collisionEnabled || config.distance == 0.0) {
      return pose;
    }
    if (!collisionProbe) {
      throw std::invalid_argument(
          "collision-enabled third-person camera requires a collision probe");
    }
    const auto hitFraction = collisionProbe(shiftedOrigin, pose.position, config.target);
    if (!hitFraction.has_value()) {
      return pose;
    }
    if (!std::isfinite(*hitFraction) || *hitFraction < 0.0 || *hitFraction > 1.0) {
      throw std::invalid_argument("camera collision hit fraction must be in [0, 1]");
    }
    const runtime::Vec3d segment = subtract(pose.position, shiftedOrigin);
    const double segmentLength = std::sqrt(lengthSquared(segment));
    const double allowedDistance =
        std::max(0.0, segmentLength * *hitFraction - config.collisionClearance);
    pose.position = add(shiftedOrigin, multiply(segment, allowedDistance / segmentLength));
    return pose;
  }
  case CameraRigMode::orbit: {
    const runtime::Vec3d position =
        subtract(shiftedOrigin, multiply(direction, config.distance));
    return {position, normalized(subtract(anchor.translation, position))};
  }
  }
  throw std::invalid_argument("camera rig mode is invalid");
}

} // namespace

CameraRig::CameraRig(CameraRigConfig config) : config_(std::move(config)) {
  validateCameraRigConfig(config_);
}

void CameraRig::setConfig(CameraRigConfig config) {
  validateCameraRigConfig(config);
  config_ = std::move(config);
}

void CameraRig::reset() noexcept {
  state_ = {};
}

bool CameraRig::update(const runtime::RuntimeWorld& world,
                       const runtime::RuntimeTransform& cameraTransform,
                       Duration elapsed,
                       const CameraCollisionProbe& collisionProbe) {
  const double seconds = elapsed.count();
  if (!std::isfinite(seconds) || seconds < 0.0) {
    throw std::invalid_argument("camera rig timestep must be finite and non-negative");
  }
  if (!finite(cameraTransform.translation)) {
    throw std::invalid_argument("camera transform must be finite");
  }

  const CameraRigPose desired = desiredPose(config_, world, cameraTransform, collisionProbe);
  const CameraRigPose previous = state_.current;
  const bool wasInitialized = state_.initialized;
  state_.desired = desired;

  if (!state_.initialized || config_.damping == 0.0) {
    state_.current = desired;
    state_.initialized = true;
  } else if (seconds > 0.0) {
    const double response = -std::expm1(-config_.damping * seconds);
    state_.current.position = lerp(state_.current.position, desired.position, response);
    state_.current.forward =
        interpolateDirection(state_.current.forward, desired.forward, response);
  }

  return !wasInitialized || !same(previous.position, state_.current.position) ||
         !same(previous.forward, state_.current.forward);
}

const CameraRigConfig& CameraRig::config() const noexcept {
  return config_;
}

const CameraRigState& CameraRig::state() const noexcept {
  return state_;
}

bool updateCameraRig(runtime::RuntimeWorld& world, const runtime::PrimId& cameraPrim,
                     CameraRig::Duration elapsed,
                     const CameraCollisionProbe& collisionProbe) {
  if (!world.containsPrim(cameraPrim)) {
    throw std::out_of_range("camera rig prim is not present in the Runtime World: " +
                            cameraPrim);
  }
  auto* transform = world.transform(cameraPrim);
  if (transform == nullptr) {
    throw std::out_of_range("camera rig prim has no RuntimeTransform: " + cameraPrim);
  }
  auto* rig = world.component<CameraRig>(cameraPrim);
  if (rig == nullptr) {
    throw std::out_of_range("camera prim has no CameraRig component: " + cameraPrim);
  }

  const bool poseChanged = rig->update(world, *transform, elapsed, collisionProbe);
  if (!poseChanged) {
    return false;
  }
  transform->translation = rig->state().current.position;
  world.markTransformDirty(cameraPrim);
  return true;
}

void validateCameraRigConfig(const CameraRigConfig& config) {
  if (config.mode != CameraRigMode::free && config.mode != CameraRigMode::firstPerson &&
      config.mode != CameraRigMode::thirdPerson && config.mode != CameraRigMode::orbit) {
    throw std::invalid_argument("camera rig mode is invalid");
  }
  if (config.mode != CameraRigMode::free &&
      (config.target.empty() || config.target.front() != '/')) {
    throw std::invalid_argument("a following camera rig requires an absolute target identity");
  }
  if (config.anchor.has_value() &&
      (config.anchor->empty() || config.anchor->front() != '/')) {
    throw std::invalid_argument("camera rig anchor identity must be absolute");
  }
  if (!finite(config.offset) || !std::isfinite(config.distance) || config.distance < 0.0 ||
      !std::isfinite(config.pitchRadians) || config.pitchRadians <= -halfPi ||
      config.pitchRadians >= halfPi || !std::isfinite(config.yawRadians) ||
      !std::isfinite(config.damping) || config.damping < 0.0 ||
      !std::isfinite(config.collisionClearance) || config.collisionClearance < 0.0) {
    throw std::invalid_argument(
        "camera rig offset, distance, angles, damping, and collision clearance are outside "
        "their valid ranges");
  }
  if (config.mode == CameraRigMode::orbit &&
      lengthSquared(subtract(multiply(viewDirection(config.yawRadians, config.pitchRadians),
                                     config.distance),
                            config.offset)) <= vectorEpsilonSquared) {
    throw std::invalid_argument("orbit camera position must differ from its rig origin");
  }
}

} // namespace usd_stage_runner::camera
