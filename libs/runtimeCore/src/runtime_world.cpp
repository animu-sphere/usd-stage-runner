#include "usd_stage_runner/runtime/runtime_world.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace usd_stage_runner::runtime {

bool RuntimeWorld::addPrim(PrimId prim) {
  if (prim.empty() || prim.front() != '/') {
    throw std::invalid_argument("prim identity must be an absolute path");
  }
  return prims_.insert(std::move(prim)).second;
}

bool RuntimeWorld::removePrim(const PrimId& prim) noexcept {
  const bool removed = prims_.erase(prim) != 0;
  if (removed) {
    components_.removeAll(prim);
    dirtyTransforms_.erase(prim);
  }
  return removed;
}

bool RuntimeWorld::containsPrim(const PrimId& prim) const noexcept {
  return prims_.find(prim) != prims_.end();
}

std::vector<PrimId> RuntimeWorld::prims() const {
  std::vector<PrimId> result(prims_.begin(), prims_.end());
  std::sort(result.begin(), result.end());
  return result;
}

RuntimeTransform& RuntimeWorld::emplaceTransform(const PrimId& prim, RuntimeTransform transform) {
  return emplaceComponent<RuntimeTransform>(prim, std::move(transform));
}

RuntimeTransform* RuntimeWorld::transform(const PrimId& prim) noexcept {
  return component<RuntimeTransform>(prim);
}

const RuntimeTransform* RuntimeWorld::transform(const PrimId& prim) const noexcept {
  return component<RuntimeTransform>(prim);
}

void RuntimeWorld::markTransformDirty(const PrimId& prim) {
  if (!containsPrim(prim)) {
    throw std::out_of_range("cannot dirty a transform for an unknown prim: " + prim);
  }
  if (transform(prim) == nullptr) {
    throw std::out_of_range("cannot dirty a missing runtime transform: " + prim);
  }
  dirtyTransforms_.mark(prim);
}

std::vector<PrimId> RuntimeWorld::takeDirtyTransforms() {
  return dirtyTransforms_.take();
}

} // namespace usd_stage_runner::runtime
