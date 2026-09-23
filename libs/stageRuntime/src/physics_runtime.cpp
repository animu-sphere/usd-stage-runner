#include "usd_stage_runner/stage/physics_runtime.h"

#include <algorithm>
#include <stdexcept>

namespace usd_stage_runner::stage {
namespace {

bool sameTranslation(const runtime::RuntimeTransform& left,
                     const usd_physics::core::Transform& right) noexcept {
  return left.translation.x == right.translation.x &&
         left.translation.y == right.translation.y &&
         left.translation.z == right.translation.z;
}

runtime::RuntimeTransform
toRuntimeTransform(const usd_physics::core::Transform& transform) noexcept {
  return {{transform.translation.x, transform.translation.y,
           transform.translation.z}};
}

} // namespace

PhysicsRuntime::PhysicsRuntime(usd_physics::core::PhysicsWorld& physicsWorld,
                               runtime::RuntimeWorld& runtimeWorld) noexcept
    : physicsWorld_(physicsWorld), runtimeWorld_(runtimeWorld) {}

bool PhysicsRuntime::bindBody(const runtime::PrimId& prim,
                              usd_physics::core::BodyHandle body) {
  if (!body) {
    throw std::invalid_argument("cannot bind an invalid physics body handle");
  }
  if (!runtimeWorld_.containsPrim(prim)) {
    throw std::out_of_range("cannot bind a physics body to an unknown prim: " + prim);
  }
  if (runtimeWorld_.transform(prim) == nullptr) {
    throw std::out_of_range(
        "cannot bind a physics body to a prim without a transform: " + prim);
  }

  (void)physicsWorld_.bodyState(body);

  auto mappedPrim = bodyToPrim_.find(body);
  if (mappedPrim != bodyToPrim_.end() &&
      !isMappingCurrent(mappedPrim->first, mappedPrim->second)) {
    bodyToPrim_.erase(mappedPrim);
    mappedPrim = bodyToPrim_.end();
  }
  if (mappedPrim != bodyToPrim_.end() && mappedPrim->second != prim) {
    throw std::invalid_argument("physics body is already bound to another prim");
  }

  const auto* currentBody = runtimeWorld_.component<PhysicsBody>(prim);
  if (currentBody != nullptr && currentBody->handle == body) {
    bodyToPrim_.insert_or_assign(body, prim);
    return false;
  }
  if (currentBody != nullptr) {
    const auto currentMapping = bodyToPrim_.find(currentBody->handle);
    if (currentMapping != bodyToPrim_.end() && currentMapping->second == prim) {
      bodyToPrim_.erase(currentMapping);
    }
  }

  runtimeWorld_.emplaceComponent<PhysicsBody>(prim, PhysicsBody{body});
  bodyToPrim_.insert_or_assign(body, prim);
  return true;
}

bool PhysicsRuntime::unbindBody(const runtime::PrimId& prim) noexcept {
  const auto* body = runtimeWorld_.component<PhysicsBody>(prim);
  if (body == nullptr) {
    const auto staleMapping =
        std::find_if(bodyToPrim_.begin(), bodyToPrim_.end(),
                     [&](const auto& entry) { return entry.second == prim; });
    if (staleMapping == bodyToPrim_.end()) {
      return false;
    }
    bodyToPrim_.erase(staleMapping);
    return true;
  }
  const auto mappedPrim = bodyToPrim_.find(body->handle);
  if (mappedPrim != bodyToPrim_.end() && mappedPrim->second == prim) {
    bodyToPrim_.erase(mappedPrim);
  }
  return runtimeWorld_.removeComponent<PhysicsBody>(prim);
}

usd_physics::core::BodyHandle
PhysicsRuntime::bodyForPrim(const runtime::PrimId& prim) const noexcept {
  const auto* body = runtimeWorld_.component<PhysicsBody>(prim);
  if (body == nullptr) {
    const auto staleMapping =
        std::find_if(bodyToPrim_.begin(), bodyToPrim_.end(),
                     [&](const auto& entry) { return entry.second == prim; });
    if (staleMapping != bodyToPrim_.end()) {
      bodyToPrim_.erase(staleMapping);
    }
    return {};
  }

  const auto mappedPrim = bodyToPrim_.find(body->handle);
  if (mappedPrim == bodyToPrim_.end() || mappedPrim->second != prim ||
      !isMappingCurrent(mappedPrim->first, mappedPrim->second)) {
    if (mappedPrim != bodyToPrim_.end() && mappedPrim->second == prim) {
      bodyToPrim_.erase(mappedPrim);
    }
    return {};
  }
  return body->handle;
}

std::optional<runtime::PrimId>
PhysicsRuntime::primForBody(usd_physics::core::BodyHandle body) const {
  const auto found = bodyToPrim_.find(body);
  if (found == bodyToPrim_.end() ||
      !isMappingCurrent(found->first, found->second)) {
    if (found != bodyToPrim_.end()) {
      bodyToPrim_.erase(found);
    }
    return std::nullopt;
  }
  return found->second;
}

std::size_t PhysicsRuntime::bodyCount() const noexcept {
  for (auto mapping = bodyToPrim_.begin(); mapping != bodyToPrim_.end();) {
    if (!isMappingCurrent(mapping->first, mapping->second)) {
      mapping = bodyToPrim_.erase(mapping);
    } else {
      ++mapping;
    }
  }
  return bodyToPrim_.size();
}

std::size_t
PhysicsRuntime::step(usd_physics::core::PhysicsWorld::Duration fixedStep) {
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

    if (!isMappingCurrent(mappedPrim->first, mappedPrim->second)) {
      bodyToPrim_.erase(mappedPrim);
      continue;
    }
    const auto& prim = mappedPrim->second;
    auto* transform = runtimeWorld_.transform(prim);
    if (sameTranslation(*transform, state.transform)) {
      continue;
    }

    *transform = toRuntimeTransform(state.transform);
    runtimeWorld_.markTransformDirty(prim);
    ++synchronized;
  }
  return synchronized;
}

bool PhysicsRuntime::isMappingCurrent(
    usd_physics::core::BodyHandle body,
    const runtime::PrimId& prim) const noexcept {
  const auto* boundBody = runtimeWorld_.component<PhysicsBody>(prim);
  return runtimeWorld_.containsPrim(prim) &&
         runtimeWorld_.transform(prim) != nullptr && boundBody != nullptr &&
         boundBody->handle == body;
}

} // namespace usd_stage_runner::stage
