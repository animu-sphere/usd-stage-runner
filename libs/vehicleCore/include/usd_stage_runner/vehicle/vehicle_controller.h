#pragma once

#include "usd_stage_runner/physics/handles.h"
#include "usd_stage_runner/runtime/component_registry.h"

#include <vector>

namespace usd_stage_runner::vehicle {

struct VehicleIntent {
  // Signed normalized demand: positive drives forward and negative drives in
  // reverse. Brake is an independent non-negative demand.
  double throttle{0.0};
  double brake{0.0};
  // Signed normalized steering demand. Positive turns toward the vehicle's
  // configured positive steering direction.
  double steering{0.0};
  bool handbrake{false};
};

struct SteeringConfig {
  double maximumAngleRadians{0.6};
};

struct PowertrainConfig {
  // Total torque distributed across wheels with a positive drive weight.
  double maximumDriveTorque{0.0};
};

struct BrakeConfig {
  // Each value is total vehicle torque distributed independently across the
  // wheels participating in that brake circuit.
  double maximumServiceBrakeTorque{0.0};
  double maximumHandbrakeTorque{0.0};
};

struct WheelConfig {
  runtime::PrimId prim;
  // Steering ratio may be negative for opposite-phase rear steering. A zero
  // ratio opts the wheel out of steering.
  double steeringRatio{0.0};
  // Non-negative weights determine torque distribution. Zero opts a wheel out
  // of that subsystem; weights are normalized per subsystem.
  double driveWeight{0.0};
  double serviceBrakeWeight{0.0};
  double handbrakeWeight{0.0};
};

struct VehicleControllerConfig {
  physics::BodyHandle chassisBody;
  SteeringConfig steering;
  PowertrainConfig powertrain;
  BrakeConfig brakes;
  std::vector<WheelConfig> wheels;
};

struct WheelCommand {
  runtime::PrimId prim;
  double steeringAngleRadians{0.0};
  double driveTorque{0.0};
  double brakeTorque{0.0};
};

struct VehicleControllerState {
  VehicleIntent intent;
  std::vector<WheelCommand> wheelCommands;
};

class VehicleController {
public:
  explicit VehicleController(VehicleControllerConfig config);

  // Converts normalized gameplay intent into backend-neutral, per-wheel
  // commands. A later physics adapter consumes these commands.
  void update(const VehicleIntent& intent);

  [[nodiscard]] const VehicleControllerConfig& config() const noexcept;
  [[nodiscard]] const VehicleControllerState& state() const noexcept;

private:
  VehicleControllerConfig config_;
  VehicleControllerState state_;
};

void validateVehicleIntent(const VehicleIntent& intent);
void validateVehicleControllerConfig(const VehicleControllerConfig& config);

} // namespace usd_stage_runner::vehicle
