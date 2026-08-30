#pragma once

#include "usd_stage_runner/physics/handles.h"
#include "usd_stage_runner/runtime/runtime_transform.h"

namespace usd_stage_runner::physics {

enum class ShapeType {
  box,
};

struct ShapeDescriptor {
  ShapeType type{ShapeType::box};
  runtime::Vec3d halfExtents{0.5, 0.5, 0.5};
};

struct PhysicsShape {
  ShapeHandle handle;
};

void validateShapeDescriptor(const ShapeDescriptor& descriptor);

} // namespace usd_stage_runner::physics
