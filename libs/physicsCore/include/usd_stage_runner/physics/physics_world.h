#pragma once

#include "usd_stage_runner/physics/physics_body.h"
#include "usd_stage_runner/physics/physics_constraint.h"
#include "usd_stage_runner/physics/physics_shape.h"

#include <chrono>
#include <vector>

namespace usd_stage_runner::physics {

class PhysicsWorld {
public:
  using Duration = std::chrono::duration<double>;

  virtual ~PhysicsWorld();

  virtual ShapeHandle createShape(const ShapeDescriptor& descriptor) = 0;
  virtual bool destroyShape(ShapeHandle shape) noexcept = 0;

  virtual BodyHandle createBody(const BodyDescriptor& descriptor) = 0;
  virtual bool destroyBody(BodyHandle body) noexcept = 0;

  virtual ConstraintHandle createConstraint(const ConstraintDescriptor& descriptor) = 0;
  virtual bool destroyConstraint(ConstraintHandle constraint) noexcept = 0;

  virtual bool applyForce(BodyHandle body, runtime::Vec3d force) = 0;
  virtual bool setLinearVelocity(BodyHandle body, runtime::Vec3d velocity) = 0;
  [[nodiscard]] virtual BodyState bodyState(BodyHandle body) const = 0;

  virtual void step(Duration fixedStep) = 0;
  virtual std::vector<BodyState> takeChangedBodyStates() = 0;
};

void validatePhysicsStep(PhysicsWorld::Duration fixedStep);
void validatePhysicsVector(runtime::Vec3d vector, const char* name);

} // namespace usd_stage_runner::physics
