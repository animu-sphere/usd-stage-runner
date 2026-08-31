#include "usd_stage_runner/character/character_controller.h"

#include <cmath>
#include <stdexcept>

namespace usd_stage_runner::character {
namespace {

double lengthSquared(runtime::Vec3d value) noexcept {
  return value.x * value.x + value.y * value.y + value.z * value.z;
}

runtime::Vec3d normalized(runtime::Vec3d value) {
  const double length = std::sqrt(lengthSquared(value));
  return {value.x / length, value.y / length, value.z / length};
}

double dot(runtime::Vec3d left, runtime::Vec3d right) noexcept {
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

runtime::Vec3d movementOnGround(runtime::Vec3d desiredVelocity,
                                runtime::Vec3d groundNormal) {
  const runtime::Vec3d horizontal{desiredVelocity.x, 0.0, desiredVelocity.z};
  const double desiredSpeed = std::sqrt(lengthSquared(horizontal));
  if (desiredSpeed == 0.0) {
    return {};
  }

  const double intoNormal = dot(horizontal, groundNormal);
  runtime::Vec3d projected{horizontal.x - groundNormal.x * intoNormal,
                          horizontal.y - groundNormal.y * intoNormal,
                          horizontal.z - groundNormal.z * intoNormal};
  const double projectedLength = std::sqrt(lengthSquared(projected));
  if (projectedLength == 0.0) {
    return {};
  }
  const double scale = desiredSpeed / projectedLength;
  projected.x *= scale;
  projected.y *= scale;
  projected.z *= scale;
  return projected;
}

} // namespace

void validateCharacterIntent(const CharacterIntent& intent) {
  physics::validatePhysicsVector(intent.desiredVelocity, "character desired velocity");
  physics::validatePhysicsVector(intent.facingDirection, "character facing direction");
}

void validateCharacterControllerConfig(const CharacterControllerConfig& config) {
  constexpr double halfPi = 1.5707963267948966;
  if (!std::isfinite(config.groundProbeDistance) || config.groundProbeDistance < 0.0) {
    throw std::invalid_argument("ground probe distance must be finite and non-negative");
  }
  if (!std::isfinite(config.maximumSlopeAngleRadians) ||
      config.maximumSlopeAngleRadians < 0.0 ||
      config.maximumSlopeAngleRadians >= halfPi) {
    throw std::invalid_argument("maximum slope angle must be in [0, pi / 2)");
  }
  if (!std::isfinite(config.jumpSpeed) || config.jumpSpeed <= 0.0) {
    throw std::invalid_argument("jump speed must be finite and positive");
  }
}

CharacterController::CharacterController(physics::BodyHandle body,
                                         physics::PhysicsWorld& physicsWorld,
                                         const physics::GroundQuery& groundQuery,
                                         CharacterControllerConfig config)
    : body_(body), physicsWorld_(&physicsWorld), groundQuery_(&groundQuery),
      config_(config) {
  if (!body_) {
    throw std::invalid_argument("character controller requires a body");
  }
  validateCharacterControllerConfig(config_);
  state_.velocity = physicsWorld_->bodyState(body_).linearVelocity;
  state_.jumpState = state_.velocity.y > 0.0 ? JumpState::rising : JumpState::falling;
}

bool CharacterController::update(const CharacterIntent& intent, Duration fixedStep) {
  validateCharacterIntent(intent);
  physics::validatePhysicsStep(fixedStep);

  const physics::BodyState bodyState = physicsWorld_->bodyState(body_);
  const auto contact = groundQuery_->groundContact(body_, config_.groundProbeDistance);
  bool walkableGround = false;
  runtime::Vec3d groundNormal{0.0, 1.0, 0.0};
  if (contact) {
    physics::validateGroundContact(*contact);
    if (contact->supportBody == body_) {
      throw std::invalid_argument("character cannot support itself");
    }
    if (contact->distance > config_.groundProbeDistance) {
      throw std::invalid_argument("ground contact exceeds the requested probe distance");
    }
    groundNormal = normalized(contact->normal);
    const bool movingAwayFromGround = dot(bodyState.linearVelocity, groundNormal) > 1.0e-9;
    walkableGround = !movingAwayFromGround &&
                     groundNormal.y >= std::cos(config_.maximumSlopeAngleRadians);
  }

  runtime::Vec3d velocity{intent.desiredVelocity.x, bodyState.linearVelocity.y,
                          intent.desiredVelocity.z};
  if (walkableGround && velocity.y <= 0.0) {
    velocity = movementOnGround(intent.desiredVelocity, groundNormal);
  }

  const bool startsJump = intent.jump && !jumpHeld_ && walkableGround;
  if (startsJump) {
    velocity.y = config_.jumpSpeed;
    walkableGround = false;
  }

  const runtime::Vec3d horizontalFacing{intent.facingDirection.x, 0.0,
                                        intent.facingDirection.z};
  CharacterState nextState = state_;
  if (lengthSquared(horizontalFacing) > 0.0) {
    nextState.facing = normalized(horizontalFacing);
  }

  if (!physicsWorld_->setLinearVelocity(body_, velocity)) {
    return false;
  }

  nextState.velocity = velocity;
  nextState.grounded = walkableGround;
  nextState.supportBody = walkableGround ? contact->supportBody : physics::BodyHandle{};
  nextState.jumpState = walkableGround ? JumpState::grounded
                                       : (velocity.y > 0.0 ? JumpState::rising
                                                         : JumpState::falling);
  state_ = nextState;
  jumpHeld_ = intent.jump;
  return true;
}

physics::BodyHandle CharacterController::body() const noexcept {
  return body_;
}

const CharacterState& CharacterController::state() const noexcept {
  return state_;
}

const CharacterControllerConfig& CharacterController::config() const noexcept {
  return config_;
}

} // namespace usd_stage_runner::character
