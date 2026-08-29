#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace usd_stage_runner::runtime {

using PrimId = std::string;

class ComponentRegistry {
public:
  template <typename Component, typename... Args>
  Component& emplace(const PrimId& prim, Args&&... args) {
    static_assert(!std::is_reference_v<Component>, "Component must be an object type");
    auto holder = std::make_unique<Holder<Component>>(std::forward<Args>(args)...);
    Component& component = holder->value;
    components_[prim].insert_or_assign(std::type_index(typeid(Component)), std::move(holder));
    return component;
  }

  template <typename Component> [[nodiscard]] Component* get(const PrimId& prim) noexcept {
    const auto primIt = components_.find(prim);
    if (primIt == components_.end()) {
      return nullptr;
    }
    const auto componentIt = primIt->second.find(std::type_index(typeid(Component)));
    if (componentIt == primIt->second.end()) {
      return nullptr;
    }
    return &static_cast<Holder<Component>&>(*componentIt->second).value;
  }

  template <typename Component>
  [[nodiscard]] const Component* get(const PrimId& prim) const noexcept {
    const auto primIt = components_.find(prim);
    if (primIt == components_.end()) {
      return nullptr;
    }
    const auto componentIt = primIt->second.find(std::type_index(typeid(Component)));
    if (componentIt == primIt->second.end()) {
      return nullptr;
    }
    return &static_cast<const Holder<Component>&>(*componentIt->second).value;
  }

  template <typename Component> [[nodiscard]] bool contains(const PrimId& prim) const noexcept {
    return get<Component>(prim) != nullptr;
  }

  template <typename Component> bool remove(const PrimId& prim) noexcept {
    const auto primIt = components_.find(prim);
    if (primIt == components_.end()) {
      return false;
    }
    const bool removed = primIt->second.erase(std::type_index(typeid(Component))) != 0;
    if (primIt->second.empty()) {
      components_.erase(primIt);
    }
    return removed;
  }

  std::size_t removeAll(const PrimId& prim) noexcept {
    const auto found = components_.find(prim);
    if (found == components_.end()) {
      return 0;
    }
    const auto count = found->second.size();
    components_.erase(found);
    return count;
  }

  [[nodiscard]] std::size_t componentCount() const noexcept {
    std::size_t count = 0;
    for (const auto& entry : components_) {
      count += entry.second.size();
    }
    return count;
  }

private:
  struct HolderBase {
    virtual ~HolderBase() = default;
  };

  template <typename Component> struct Holder final : HolderBase {
    template <typename... Args>
    explicit Holder(Args&&... args) : value(std::forward<Args>(args)...) {}
    Component value;
  };

  using ComponentsByType = std::unordered_map<std::type_index, std::unique_ptr<HolderBase>>;
  std::unordered_map<PrimId, ComponentsByType> components_;
};

} // namespace usd_stage_runner::runtime
