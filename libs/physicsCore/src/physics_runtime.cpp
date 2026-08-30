#include "usd_stage_runner/physics/physics_runtime.h"

#include <algorithm>
#include <stdexcept>

namespace usd_stage_runner::physics {
namespace {

bool sameTransform(const runtime::RuntimeTransform& left,
                   const runtime::RuntimeTransform& right) noexcept {
  return left.translation.x == right.translation.x &&
         left.translation.y == right.translation.y &&
         left.translation.z == right.translation.z;
}

} // namespace

PhysicsRuntime::PhysicsRuntime(PhysicsWorld& physicsWorld,
                               runtime::RuntimeWorld& runtimeWorld) noexcept
    : physicsWorld_(physicsWorld), runtimeWorld_(runtimeWorld) {}

bool PhysicsRuntime::bindBody(const runtime::PrimId& prim, BodyHandle body) {
  if (!body) {
    throw std::invalid_argument("cannot bind an invalid physics body handle");
  }
  if (!runtimeWorld_.containsPrim(prim)) {
    throw std::out_of_range("cannot bind a physics body to an unknown prim: " + prim);
  }
  if (runtimeWorld_.transform(prim) == nullptr) {
    throw std::out_of_range("cannot bind a physics body to a prim without a transform: " + prim);
  }

  (void)physicsWorld_.bodyState(body);

  const auto mappedPrim = bodyToPrim_.find(body);
  if (mappedPrim != bodyToPrim_.end() && mappedPrim->second != prim) {
    throw std::invalid_argument("physics body is already bound to another prim");
  }

  const auto* currentBody = runtimeWorld_.component<PhysicsBody>(prim);
  if (currentBody != nullptr && currentBody->handle == body) {
    bodyToPrim_.insert_or_assign(body, prim);
    return false;
  }
  if (currentBody != nullptr) {
    bodyToPrim_.erase(currentBody->handle);
  }

  runtimeWorld_.emplaceComponent<PhysicsBody>(prim, PhysicsBody{body});
  bodyToPrim_.insert_or_assign(body, prim);
  return true;
}

bool PhysicsRuntime::unbindBody(const runtime::PrimId& prim) noexcept {
  const auto* body = runtimeWorld_.component<PhysicsBody>(prim);
  if (body == nullptr) {
    const auto staleMapping =
        std::find_if(bodyToPrim_.begin(), bodyToPrim_.end(), [&](const auto& entry) {
          return entry.second == prim;
        });
    if (staleMapping == bodyToPrim_.end()) {
      return false;
    }
    bodyToPrim_.erase(staleMapping);
    return true;
  }
  bodyToPrim_.erase(body->handle);
  return runtimeWorld_.removeComponent<PhysicsBody>(prim);
}

BodyHandle PhysicsRuntime::bodyForPrim(const runtime::PrimId& prim) const noexcept {
  const auto* body = runtimeWorld_.component<PhysicsBody>(prim);
  return body == nullptr ? BodyHandle{} : body->handle;
}

std::optional<runtime::PrimId> PhysicsRuntime::primForBody(BodyHandle body) const {
  const auto found = bodyToPrim_.find(body);
  if (found == bodyToPrim_.end()) {
    return std::nullopt;
  }
  return found->second;
}

std::size_t PhysicsRuntime::step(PhysicsWorld::Duration fixedStep) {
  physicsWorld_.step(fixedStep);
  return synchronizeChangedBodyStates();
}

std::size_t PhysicsRuntime::synchronizeChangedBodyStates() {
  std::size_t synchronized = 0;
  for (const auto& state : physicsWorld_.takeChangedBodyStates()) {
    auto mappedPrim = bodyToPrim_.find(state.body);
    if (mappedPrim == bodyToPrim_.end()) {
      continue;
    }

    const auto& prim = mappedPrim->second;
    const auto* boundBody = runtimeWorld_.component<PhysicsBody>(prim);
    auto* transform = runtimeWorld_.transform(prim);
    if (boundBody == nullptr || boundBody->handle != state.body || transform == nullptr) {
      bodyToPrim_.erase(mappedPrim);
      continue;
    }
    if (sameTransform(*transform, state.transform)) {
      continue;
    }

    *transform = state.transform;
    runtimeWorld_.markTransformDirty(prim);
    ++synchronized;
  }
  return synchronized;
}

} // namespace usd_stage_runner::physics
