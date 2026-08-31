#include "usd_stage_runner/character/character_controller.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using namespace usd_stage_runner;

class CharacterPhysicsDouble final : public physics::PhysicsWorld,
                                     public physics::GroundQuery {
public:
  physics::ShapeHandle createShape(const physics::ShapeDescriptor&) override {
    return physics::ShapeHandle{1};
  }

  bool destroyShape(physics::ShapeHandle) noexcept override {
    return true;
  }

  physics::BodyHandle createBody(const physics::BodyDescriptor&) override {
    const physics::BodyHandle handle{nextBody_++};
    states_.emplace(handle, physics::BodyState{handle, {}, {}});
    return handle;
  }

  bool destroyBody(physics::BodyHandle body) noexcept override {
    return states_.erase(body) != 0;
  }

  physics::ConstraintHandle
  createConstraint(const physics::ConstraintDescriptor&) override {
    return physics::ConstraintHandle{1};
  }

  bool destroyConstraint(physics::ConstraintHandle) noexcept override {
    return true;
  }

  bool applyForce(physics::BodyHandle body, runtime::Vec3d) override {
    return states_.find(body) != states_.end();
  }

  bool setLinearVelocity(physics::BodyHandle body, runtime::Vec3d velocity) override {
    const auto found = states_.find(body);
    if (found == states_.end()) {
      return false;
    }
    found->second.linearVelocity = velocity;
    return true;
  }

  physics::BodyState bodyState(physics::BodyHandle body) const override {
    const auto found = states_.find(body);
    if (found == states_.end()) {
      throw std::out_of_range("unknown body");
    }
    return found->second;
  }

  void step(Duration) override {}

  std::vector<physics::BodyState> takeChangedBodyStates() override {
    return {};
  }

  std::optional<physics::GroundContact>
  groundContact(physics::BodyHandle body, double maxDistance) const override {
    if (states_.find(body) == states_.end()) {
      throw std::out_of_range("unknown body");
    }
    lastProbeDistance_ = maxDistance;
    return contact_;
  }

  void setContact(std::optional<physics::GroundContact> contact) {
    contact_ = contact;
  }

  void setVelocity(physics::BodyHandle body, runtime::Vec3d velocity) {
    states_.at(body).linearVelocity = velocity;
  }

  [[nodiscard]] double lastProbeDistance() const noexcept {
    return lastProbeDistance_;
  }

private:
  physics::BodyHandle::ValueType nextBody_{1};
  std::unordered_map<physics::BodyHandle, physics::BodyState,
                     physics::PhysicsHandleHash<physics::BodyHandle>>
      states_;
  std::optional<physics::GroundContact> contact_;
  mutable double lastProbeDistance_{-1.0};
};

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

} // namespace

int main() {
  using namespace usd_stage_runner;

  CharacterPhysicsDouble physicsWorld;
  const physics::BodyHandle support{99};
  const physics::BodyHandle characterBody = physicsWorld.createBody({});
  physicsWorld.setContact(physics::GroundContact{support, {0.0, 1.0, 0.0}, 0.02});

  character::CharacterController controller(
      characterBody, physicsWorld, physicsWorld,
      character::CharacterControllerConfig{0.2, 0.7853981633974483, 6.0});
  const character::CharacterIntent walk{{3.0, 20.0, 4.0}, {1.0, 1.0, 0.0}, false};
  if (!controller.update(walk, character::CharacterController::Duration{1.0 / 60.0})) {
    return fail("a known dynamic character body must accept controller velocity");
  }

  const auto groundedState = controller.state();
  if (!groundedState.grounded || groundedState.jumpState != character::JumpState::grounded ||
      groundedState.supportBody != support || !near(groundedState.velocity.x, 3.0) ||
      !near(groundedState.velocity.y, 0.0) || !near(groundedState.velocity.z, 4.0) ||
      !near(groundedState.facing.x, 1.0) || !near(groundedState.facing.y, 0.0) ||
      !near(groundedState.facing.z, 0.0) || !near(physicsWorld.lastProbeDistance(), 0.2)) {
    return fail("walkable ground must update velocity, facing, support, and grounded state");
  }

  const double inverseSqrtTwo = 1.0 / std::sqrt(2.0);
  physicsWorld.setContact(
      physics::GroundContact{support, {-inverseSqrtTwo, inverseSqrtTwo, 0.0}, 0.01});
  if (!controller.update({{2.0, 0.0, 0.0}, {}, false},
                         character::CharacterController::Duration{1.0 / 60.0}) ||
      !controller.state().grounded || controller.state().velocity.y <= 0.0 ||
      !near(std::sqrt(controller.state().velocity.x * controller.state().velocity.x +
                     controller.state().velocity.y * controller.state().velocity.y),
            2.0)) {
    return fail("walkable slopes must project desired motion onto the support plane");
  }
  if (!controller.update({{2.0, 0.0, 0.0}, {}, false},
                         character::CharacterController::Duration{1.0 / 60.0}) ||
      !controller.state().grounded) {
    return fail("uphill velocity must remain grounded on the following fixed step");
  }

  physicsWorld.setVelocity(characterBody, {});
  physicsWorld.setContact(physics::GroundContact{support, {1.0, 0.0, 0.0}, 0.01});
  controller.update({{2.0, 0.0, 0.0}, {}, false},
                    character::CharacterController::Duration{1.0 / 60.0});
  if (controller.state().grounded || controller.state().supportBody ||
      controller.state().jumpState != character::JumpState::falling ||
      !near(controller.state().velocity.x, 2.0)) {
    return fail("steep contacts must not count as walkable ground");
  }

  physicsWorld.setVelocity(characterBody, {});
  physicsWorld.setContact(physics::GroundContact{support, {0.0, 1.0, 0.0}, 0.01});
  if (!controller.update({{}, {}, true},
                         character::CharacterController::Duration{1.0 / 60.0}) ||
      controller.state().grounded || controller.state().supportBody ||
      controller.state().jumpState != character::JumpState::rising ||
      !near(controller.state().velocity.y, 6.0)) {
    return fail("a jump edge on walkable ground must start an upward jump");
  }

  controller.update({{}, {}, true}, character::CharacterController::Duration{1.0 / 60.0});
  if (controller.state().grounded || !near(controller.state().velocity.y, 6.0)) {
    return fail("holding jump must not retrigger or ground a rising character");
  }

  physicsWorld.setVelocity(characterBody, {0.0, -2.0, 0.0});
  physicsWorld.setContact(std::nullopt);
  controller.update({{}, {}, false}, character::CharacterController::Duration{1.0 / 60.0});
  if (controller.state().jumpState != character::JumpState::falling ||
      !near(controller.state().velocity.y, -2.0)) {
    return fail("airborne downward motion must transition to falling");
  }

  if (!rejectsInvalidArgument([&] {
        character::CharacterController invalid({}, physicsWorld, physicsWorld);
      }) ||
      !rejectsInvalidArgument([&] {
        character::CharacterController invalid(
            characterBody, physicsWorld, physicsWorld,
            character::CharacterControllerConfig{-1.0, 0.5, 5.0});
      }) ||
      !rejectsInvalidArgument([&] {
        controller.update(
            {{std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}, {}, false},
            character::CharacterController::Duration{1.0 / 60.0});
      }) ||
      !rejectsInvalidArgument([&] {
        controller.update({}, character::CharacterController::Duration{0.0});
      })) {
    return fail("character contracts must reject invalid bodies, configuration, intent, and steps");
  }

  physicsWorld.setVelocity(characterBody, {});
  physicsWorld.setContact(
      physics::GroundContact{{}, {0.0, 1.0, 0.0}, 0.0});
  if (!rejectsInvalidArgument([&] {
        controller.update({}, character::CharacterController::Duration{1.0 / 60.0});
      })) {
    return fail("invalid backend ground contacts must be rejected at the core boundary");
  }

  return 0;
}
