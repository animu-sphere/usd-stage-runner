#pragma once

#include "usd_stage_runner/runtime/runtime_transform.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#include <chrono>
#include <functional>
#include <optional>

namespace usd_stage_runner::camera {

enum class CameraRigMode {
  free,
  firstPerson,
  thirdPerson,
  orbit,
};

struct CameraRigConfig {
  CameraRigMode mode{CameraRigMode::free};
  // Non-free modes resolve this absolute prim identity. The optional anchor
  // overrides its position as the rig origin (for example, a character head).
  runtime::PrimId target;
  std::optional<runtime::PrimId> anchor;
  // World-space framing offset. First- and third-person modes aim along the
  // configured angles from this shifted origin; orbit keeps aiming at the
  // unshifted origin.
  runtime::Vec3d offset;
  double distance{4.0};
  // Y-up radians: yaw zero faces -Z, and positive pitch looks upward.
  double pitchRadians{0.0};
  double yawRadians{0.0};
  // Exponential response rate in inverse seconds. Zero disables smoothing.
  double damping{0.0};
  // Collision probing currently affects third-person mode only. Clearance
  // shortens the hit distance so the camera remains in front of geometry.
  bool collisionEnabled{false};
  double collisionClearance{0.1};
};

struct CameraRigPose {
  runtime::Vec3d position;
  runtime::Vec3d forward{0.0, 0.0, -1.0};
};

struct CameraRigState {
  CameraRigPose desired;
  CameraRigPose current;
  bool initialized{false};
};

// Returns the normalized first-hit distance from origin (0) to desiredPosition
// (1), or no value when the segment is unobstructed. target identifies the
// followed prim so a host can ignore its physics body without exposing backend
// handles to cameraCore.
using CameraCollisionProbe = std::function<std::optional<double>(
    const runtime::Vec3d& origin, const runtime::Vec3d& desiredPosition,
    const runtime::PrimId& target)>;

class CameraRig {
public:
  using Duration = std::chrono::duration<double>;

  explicit CameraRig(CameraRigConfig config = {});

  // Configuration changes preserve the current live pose so mode switches
  // follow the same smoothing path. reset() requests a snap on the next update.
  void setConfig(CameraRigConfig config);
  void reset() noexcept;

  // Resolves the configured target and anchor from the Runtime World and
  // advances the live pose. The camera transform supplies the free-mode
  // position and the initial position before the first evaluation.
  bool update(const runtime::RuntimeWorld& world,
              const runtime::RuntimeTransform& cameraTransform, Duration elapsed,
              const CameraCollisionProbe& collisionProbe = {});

  [[nodiscard]] const CameraRigConfig& config() const noexcept;
  [[nodiscard]] const CameraRigState& state() const noexcept;

private:
  CameraRigConfig config_;
  CameraRigState state_;
};

// Evaluates a CameraRig attached to cameraPrim, writes its current position to
// the prim's RuntimeTransform, and marks the transform dirty only when the
// position or viewing direction changed.
bool updateCameraRig(runtime::RuntimeWorld& world, const runtime::PrimId& cameraPrim,
                     CameraRig::Duration elapsed,
                     const CameraCollisionProbe& collisionProbe = {});

void validateCameraRigConfig(const CameraRigConfig& config);

} // namespace usd_stage_runner::camera
