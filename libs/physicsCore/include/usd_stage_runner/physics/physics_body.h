#pragma once

#include "usd_stage_runner/physics/handles.h"
#include "usd_stage_runner/runtime/runtime_transform.h"

#include <cstdint>

namespace usd_stage_runner::physics {

enum class MotionType {
  staticBody,
  dynamicBody,
};

using CollisionLayer = std::uint16_t;

struct BodyDescriptor {
  ShapeHandle shape;
  MotionType motionType{MotionType::staticBody};
  runtime::RuntimeTransform initialTransform;
  double mass{1.0};
  CollisionLayer collisionLayer{0};
};

struct BodyState {
  BodyHandle body;
  runtime::RuntimeTransform transform;
  runtime::Vec3d linearVelocity;
};

struct PhysicsBody {
  BodyHandle handle;
};

void validateBodyDescriptor(const BodyDescriptor& descriptor);

} // namespace usd_stage_runner::physics
