#include "usd_stage_runner/physics/collision_query.h"

#include "usd_stage_runner/physics/physics_world.h"

#include <cmath>
#include <stdexcept>

namespace usd_stage_runner::physics {

CollisionQuery::~CollisionQuery() = default;

void validateCollisionSegment(runtime::Vec3d origin, runtime::Vec3d target) {
  validatePhysicsVector(origin, "collision segment origin");
  validatePhysicsVector(target, "collision segment target");
  const double x = target.x - origin.x;
  const double y = target.y - origin.y;
  const double z = target.z - origin.z;
  const double lengthSquared = x * x + y * y + z * z;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0) {
    throw std::invalid_argument("collision segment must have non-zero finite length");
  }
}

void validateSegmentHit(const SegmentHit& hit) {
  if (!hit.body) {
    throw std::invalid_argument("collision segment hit requires a body");
  }
  if (!std::isfinite(hit.fraction) || hit.fraction < 0.0 || hit.fraction > 1.0) {
    throw std::invalid_argument("collision segment hit fraction must be in [0, 1]");
  }
}

} // namespace usd_stage_runner::physics
