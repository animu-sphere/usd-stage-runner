#pragma once

#include "usd_stage_runner/physics/ground_query.h"
#include "usd_stage_runner/physics/physics_world.h"
#include "usd_stage_runner/runtime/runtime_transform.h"

#include <chrono>

namespace usd_stage_runner::character {

struct CharacterIntent {
  // X/Z are desired planar velocity. Vertical motion is owned by grounding,
  // jumping, and the physics world.
  runtime::Vec3d desiredVelocity;
  // Only the X/Z direction is used; zero keeps the current facing.
  runtime::Vec3d facingDirection{0.0, 0.0, 1.0};
  bool jump{false};
};

enum class JumpState {
  grounded,
  rising,
  falling,
};

struct CharacterState {
  runtime::Vec3d velocity;
  bool grounded{false};
  JumpState jumpState{JumpState::falling};
  runtime::Vec3d facing{0.0, 0.0, 1.0};
  physics::BodyHandle supportBody;
};

struct CharacterControllerConfig {
  double groundProbeDistance{0.1};
  double maximumSlopeAngleRadians{0.7853981633974483};
  double jumpSpeed{5.0};
};

class CharacterController {
public:
  using Duration = std::chrono::duration<double>;

  CharacterController(physics::BodyHandle body, physics::PhysicsWorld& physicsWorld,
                      const physics::GroundQuery& groundQuery,
                      CharacterControllerConfig config = {});

  bool update(const CharacterIntent& intent, Duration fixedStep);

  [[nodiscard]] physics::BodyHandle body() const noexcept;
  [[nodiscard]] const CharacterState& state() const noexcept;
  [[nodiscard]] const CharacterControllerConfig& config() const noexcept;

private:
  physics::BodyHandle body_;
  physics::PhysicsWorld* physicsWorld_;
  const physics::GroundQuery* groundQuery_;
  CharacterControllerConfig config_;
  CharacterState state_;
  bool jumpHeld_{false};
};

void validateCharacterIntent(const CharacterIntent& intent);
void validateCharacterControllerConfig(const CharacterControllerConfig& config);

} // namespace usd_stage_runner::character
