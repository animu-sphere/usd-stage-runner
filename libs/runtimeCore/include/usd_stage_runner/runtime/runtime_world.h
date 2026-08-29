#pragma once

#include "usd_stage_runner/runtime/component_registry.h"

#include <cstddef>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace usd_stage_runner::runtime {

class RuntimeWorld {
public:
  bool addPrim(PrimId prim);
  bool removePrim(const PrimId& prim) noexcept;

  [[nodiscard]] bool containsPrim(const PrimId& prim) const noexcept;
  [[nodiscard]] std::size_t primCount() const noexcept {
    return prims_.size();
  }
  [[nodiscard]] std::vector<PrimId> prims() const;

  template <typename Component, typename... Args>
  Component& emplaceComponent(const PrimId& prim, Args&&... args) {
    if (!containsPrim(prim)) {
      throw std::out_of_range("cannot attach a component to an unknown prim: " + prim);
    }
    return components_.emplace<Component>(prim, std::forward<Args>(args)...);
  }

  template <typename Component> [[nodiscard]] Component* component(const PrimId& prim) noexcept {
    return components_.get<Component>(prim);
  }

  template <typename Component>
  [[nodiscard]] const Component* component(const PrimId& prim) const noexcept {
    return components_.get<Component>(prim);
  }

  [[nodiscard]] std::size_t componentCount() const noexcept {
    return components_.componentCount();
  }

private:
  std::unordered_set<PrimId> prims_;
  ComponentRegistry components_;
};

} // namespace usd_stage_runner::runtime
