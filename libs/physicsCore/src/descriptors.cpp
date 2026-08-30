#include "usd_stage_runner/physics/physics_body.h"
#include "usd_stage_runner/physics/physics_constraint.h"
#include "usd_stage_runner/physics/physics_shape.h"
#include "usd_stage_runner/physics/physics_world.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace usd_stage_runner::physics {
namespace {

bool isFinite(runtime::Vec3d value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

} // namespace

void validatePhysicsVector(runtime::Vec3d vector, const char* name) {
  if (!isFinite(vector)) {
    throw std::invalid_argument(std::string(name) + " must be finite");
  }
}

void validateShapeDescriptor(const ShapeDescriptor& descriptor) {
  validatePhysicsVector(descriptor.halfExtents, "box half extents");
  if (descriptor.type != ShapeType::box || descriptor.halfExtents.x <= 0.0 ||
      descriptor.halfExtents.y <= 0.0 || descriptor.halfExtents.z <= 0.0) {
    throw std::invalid_argument("box half extents must be positive");
  }
}

void validateBodyDescriptor(const BodyDescriptor& descriptor) {
  if (!descriptor.shape) {
    throw std::invalid_argument("a physics body requires a valid shape handle");
  }
  validatePhysicsVector(descriptor.initialTransform.translation, "body translation");
  if (!std::isfinite(descriptor.mass) ||
      (descriptor.motionType == MotionType::dynamicBody && descriptor.mass <= 0.0) ||
      (descriptor.motionType == MotionType::staticBody && descriptor.mass < 0.0)) {
    throw std::invalid_argument("body mass must be finite and positive for dynamic bodies");
  }
}

void validateConstraintDescriptor(const ConstraintDescriptor& descriptor) {
  if (!descriptor.firstBody || !descriptor.secondBody) {
    throw std::invalid_argument("a physics constraint requires two valid body handles");
  }
  if (descriptor.firstBody == descriptor.secondBody) {
    throw std::invalid_argument("a physics constraint requires two distinct bodies");
  }
}

void validatePhysicsStep(PhysicsWorld::Duration fixedStep) {
  if (!std::isfinite(fixedStep.count()) || fixedStep <= PhysicsWorld::Duration::zero()) {
    throw std::invalid_argument("physics step must be finite and positive");
  }
}

} // namespace usd_stage_runner::physics
