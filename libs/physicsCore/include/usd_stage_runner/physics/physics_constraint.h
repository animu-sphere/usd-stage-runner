#pragma once

#include "usd_stage_runner/physics/handles.h"

namespace usd_stage_runner::physics {

enum class ConstraintType {
  fixed,
};

struct ConstraintDescriptor {
  ConstraintType type{ConstraintType::fixed};
  BodyHandle firstBody;
  BodyHandle secondBody;
};

struct PhysicsConstraint {
  ConstraintHandle handle;
};

void validateConstraintDescriptor(const ConstraintDescriptor& descriptor);

} // namespace usd_stage_runner::physics
