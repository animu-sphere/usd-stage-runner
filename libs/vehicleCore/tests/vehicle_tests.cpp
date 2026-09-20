#include "usd_stage_runner/vehicle/vehicle_controller.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bool near(double left, double right) {
  return std::abs(left - right) < 1.0e-9;
}

template <typename Function> bool rejectsInvalidArgument(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::invalid_argument&) {
    return true;
  }
  return false;
}

usd_stage_runner::vehicle::VehicleControllerConfig fourWheelDriveConfig() {
  using namespace usd_stage_runner;
  return {
      physics::BodyHandle{1},
      vehicle::SteeringConfig{0.5},
      vehicle::PowertrainConfig{800.0},
      vehicle::BrakeConfig{1200.0, 600.0},
      {{"/Vehicle/FrontLeft", 1.0, 1.0, 1.0, 0.0},
       {"/Vehicle/FrontRight", 1.0, 1.0, 1.0, 0.0},
       {"/Vehicle/RearLeft", 0.0, 1.0, 1.0, 1.0},
       {"/Vehicle/RearRight", 0.0, 1.0, 1.0, 1.0}},
  };
}

} // namespace

int main() {
  using namespace usd_stage_runner;

  vehicle::VehicleController controller(fourWheelDriveConfig());
  controller.update({0.75, 0.5, -0.4, true});
  const auto& commands = controller.state().wheelCommands;
  if (commands.size() != 4 || !near(commands[0].steeringAngleRadians, -0.2) ||
      !near(commands[1].steeringAngleRadians, -0.2) ||
      !near(commands[2].steeringAngleRadians, 0.0) || !near(commands[0].driveTorque, 150.0) ||
      !near(commands[3].driveTorque, 150.0) || !near(commands[0].brakeTorque, 150.0) ||
      !near(commands[1].brakeTorque, 150.0) || !near(commands[2].brakeTorque, 450.0) ||
      !near(commands[3].brakeTorque, 450.0)) {
    return fail("four-wheel intent must compose steering, drive, and brake commands");
  }

  auto threeWheelConfig = fourWheelDriveConfig();
  threeWheelConfig.wheels = {{"/Vehicle/Front", 1.0, 1.0, 1.0, 0.0},
                             {"/Vehicle/RearLeft", -0.25, 2.0, 1.0, 1.0},
                             {"/Vehicle/RearRight", -0.25, 1.0, 1.0, 1.0}};
  vehicle::VehicleController threeWheelController(std::move(threeWheelConfig));
  threeWheelController.update({-1.0, 0.0, 1.0, false});
  const auto& threeWheelCommands = threeWheelController.state().wheelCommands;
  if (threeWheelCommands.size() != 3 || !near(threeWheelCommands[0].steeringAngleRadians, 0.5) ||
      !near(threeWheelCommands[1].steeringAngleRadians, -0.125) ||
      !near(threeWheelCommands[0].driveTorque, -200.0) ||
      !near(threeWheelCommands[1].driveTorque, -400.0) ||
      !near(threeWheelCommands[2].driveTorque, -200.0)) {
    return fail("wheel command composition must not assume a four-wheel layout");
  }

  vehicle::VehicleController passiveController(
      {physics::BodyHandle{2}, {}, {}, {}, {{"/Trailer/Wheel"}}});
  passiveController.update({});
  if (passiveController.state().wheelCommands.size() != 1 ||
      !near(passiveController.state().wheelCommands[0].driveTorque, 0.0) ||
      !near(passiveController.state().wheelCommands[0].brakeTorque, 0.0)) {
    return fail("disabled torque circuits must allow passive wheel assemblies");
  }

  auto invalidConfig = fourWheelDriveConfig();
  invalidConfig.wheels[1].prim = invalidConfig.wheels[0].prim;
  auto noDrivenWheels = fourWheelDriveConfig();
  for (auto& wheel : noDrivenWheels.wheels) {
    wheel.driveWeight = 0.0;
  }
  if (!rejectsInvalidArgument(
          [] { vehicle::VehicleController invalid(vehicle::VehicleControllerConfig{}); }) ||
      !rejectsInvalidArgument([&] { vehicle::VehicleController invalid(invalidConfig); }) ||
      !rejectsInvalidArgument([&] { vehicle::VehicleController invalid(noDrivenWheels); }) ||
      !rejectsInvalidArgument([&] {
        controller.update({std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, false});
      }) ||
      !rejectsInvalidArgument([&] { controller.update({0.0, -0.1, 0.0, false}); }) ||
      !rejectsInvalidArgument([&] { controller.update({0.0, 0.0, 1.1, false}); })) {
    return fail("vehicle contracts must reject invalid configuration and intent");
  }

  return 0;
}
