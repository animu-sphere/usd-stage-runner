#include "usd_stage_runner/physics/ground_query.h"

#include "usd_stage_runner/physics/physics_world.h"

#include <cmath>
#include <stdexcept>

namespace usd_stage_runner::physics {

GroundQuery::~GroundQuery() = default;

void validateGroundContact(const GroundContact& contact) {
  if (!contact.supportBody) {
    throw std::invalid_argument("ground contact requires a support body");
  }
  validatePhysicsVector(contact.normal, "ground normal");
  const double lengthSquared = contact.normal.x * contact.normal.x +
                               contact.normal.y * contact.normal.y +
                               contact.normal.z * contact.normal.z;
  if (lengthSquared <= 0.0) {
    throw std::invalid_argument("ground normal must be non-zero");
  }
  if (!std::isfinite(contact.distance) || contact.distance < 0.0) {
    throw std::invalid_argument("ground distance must be finite and non-negative");
  }
}

} // namespace usd_stage_runner::physics
