#include "usd_stage_runner/vehicle/vehicle_controller.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace usd_stage_runner::vehicle {
namespace {

void validateNormalized(double value, const char* name, bool signedValue) {
  const double minimum = signedValue ? -1.0 : 0.0;
  if (!std::isfinite(value) || value < minimum || value > 1.0) {
    throw std::invalid_argument(std::string{name} + (signedValue
                                                         ? " must be finite and in [-1, 1]"
                                                         : " must be finite and in [0, 1]"));
  }
}

void validateNonNegative(double value, const char* name) {
  if (!std::isfinite(value) || value < 0.0) {
    throw std::invalid_argument(std::string{name} + " must be finite and non-negative");
  }
}

double weightTotal(const std::vector<WheelConfig>& wheels, double WheelConfig::* member) noexcept {
  double total = 0.0;
  for (const auto& wheel : wheels) {
    total += wheel.*member;
  }
  return total;
}

void validateTorqueCircuit(const std::vector<WheelConfig>& wheels,
                           double WheelConfig::* weightMember, double maximumTorque,
                           const char* name) {
  const double total = weightTotal(wheels, weightMember);
  if (!std::isfinite(total)) {
    throw std::invalid_argument(std::string{name} + " wheel weight total must be finite");
  }
  if (maximumTorque > 0.0 && total == 0.0) {
    throw std::invalid_argument(std::string{"positive "} + name +
                                " torque requires at least one participating wheel");
  }
}

} // namespace

void validateVehicleIntent(const VehicleIntent& intent) {
  validateNormalized(intent.throttle, "vehicle throttle", true);
  validateNormalized(intent.brake, "vehicle brake", false);
  validateNormalized(intent.steering, "vehicle steering", true);
}

void validateVehicleControllerConfig(const VehicleControllerConfig& config) {
  if (!config.chassisBody) {
    throw std::invalid_argument("vehicle controller requires a chassis body");
  }
  if (!std::isfinite(config.steering.maximumAngleRadians) ||
      config.steering.maximumAngleRadians < 0.0) {
    throw std::invalid_argument("maximum steering angle must be finite and non-negative");
  }
  validateNonNegative(config.powertrain.maximumDriveTorque, "maximum drive torque");
  validateNonNegative(config.brakes.maximumServiceBrakeTorque, "maximum service brake torque");
  validateNonNegative(config.brakes.maximumHandbrakeTorque, "maximum handbrake torque");
  if (config.wheels.empty()) {
    throw std::invalid_argument("vehicle controller requires at least one wheel");
  }

  std::unordered_set<runtime::PrimId> wheelPrims;
  for (const auto& wheel : config.wheels) {
    if (wheel.prim.empty() || wheel.prim.front() != '/') {
      throw std::invalid_argument("vehicle wheel prim must be an absolute path");
    }
    if (!wheelPrims.insert(wheel.prim).second) {
      throw std::invalid_argument("vehicle wheel prims must be unique");
    }
    if (!std::isfinite(wheel.steeringRatio) || wheel.steeringRatio < -1.0 ||
        wheel.steeringRatio > 1.0) {
      throw std::invalid_argument("wheel steering ratio must be finite and in [-1, 1]");
    }
    validateNonNegative(wheel.driveWeight, "wheel drive weight");
    validateNonNegative(wheel.serviceBrakeWeight, "wheel service brake weight");
    validateNonNegative(wheel.handbrakeWeight, "wheel handbrake weight");
  }

  if (!std::isfinite(config.brakes.maximumServiceBrakeTorque +
                     config.brakes.maximumHandbrakeTorque)) {
    throw std::invalid_argument("combined brake torque must be finite");
  }
  validateTorqueCircuit(config.wheels, &WheelConfig::driveWeight,
                        config.powertrain.maximumDriveTorque, "drive");
  validateTorqueCircuit(config.wheels, &WheelConfig::serviceBrakeWeight,
                        config.brakes.maximumServiceBrakeTorque, "service brake");
  validateTorqueCircuit(config.wheels, &WheelConfig::handbrakeWeight,
                        config.brakes.maximumHandbrakeTorque, "handbrake");
}

VehicleController::VehicleController(VehicleControllerConfig config) : config_(std::move(config)) {
  validateVehicleControllerConfig(config_);
  update({});
}

void VehicleController::update(const VehicleIntent& intent) {
  validateVehicleIntent(intent);

  const double totalDriveWeight = weightTotal(config_.wheels, &WheelConfig::driveWeight);
  const double totalServiceBrakeWeight =
      weightTotal(config_.wheels, &WheelConfig::serviceBrakeWeight);
  const double totalHandbrakeWeight = weightTotal(config_.wheels, &WheelConfig::handbrakeWeight);

  VehicleControllerState nextState;
  nextState.intent = intent;
  nextState.wheelCommands.reserve(config_.wheels.size());
  for (const auto& wheel : config_.wheels) {
    WheelCommand command;
    command.prim = wheel.prim;
    command.steeringAngleRadians =
        intent.steering * config_.steering.maximumAngleRadians * wheel.steeringRatio;
    if (totalDriveWeight > 0.0) {
      command.driveTorque = intent.throttle * (wheel.driveWeight / totalDriveWeight) *
                            config_.powertrain.maximumDriveTorque;
    }
    if (totalServiceBrakeWeight > 0.0) {
      command.brakeTorque = intent.brake * (wheel.serviceBrakeWeight / totalServiceBrakeWeight) *
                            config_.brakes.maximumServiceBrakeTorque;
    }
    if (intent.handbrake && totalHandbrakeWeight > 0.0) {
      command.brakeTorque +=
          (wheel.handbrakeWeight / totalHandbrakeWeight) * config_.brakes.maximumHandbrakeTorque;
    }
    nextState.wheelCommands.push_back(std::move(command));
  }
  state_ = std::move(nextState);
}

const VehicleControllerConfig& VehicleController::config() const noexcept {
  return config_;
}

const VehicleControllerState& VehicleController::state() const noexcept {
  return state_;
}

} // namespace usd_stage_runner::vehicle
