#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace usd_stage_runner::physics {

template <typename Tag> class PhysicsHandle {
public:
  using ValueType = std::uint64_t;

  constexpr PhysicsHandle() noexcept = default;
  explicit constexpr PhysicsHandle(ValueType value) noexcept : value_(value) {}

  [[nodiscard]] constexpr ValueType value() const noexcept {
    return value_;
  }

  [[nodiscard]] explicit constexpr operator bool() const noexcept {
    return value_ != 0;
  }

  friend constexpr bool operator==(PhysicsHandle left, PhysicsHandle right) noexcept {
    return left.value_ == right.value_;
  }

  friend constexpr bool operator!=(PhysicsHandle left, PhysicsHandle right) noexcept {
    return !(left == right);
  }

private:
  ValueType value_{0};
};

struct BodyHandleTag;
struct ShapeHandleTag;
struct ConstraintHandleTag;

using BodyHandle = PhysicsHandle<BodyHandleTag>;
using ShapeHandle = PhysicsHandle<ShapeHandleTag>;
using ConstraintHandle = PhysicsHandle<ConstraintHandleTag>;

template <typename Handle> struct PhysicsHandleHash {
  std::size_t operator()(Handle handle) const noexcept {
    return std::hash<typename Handle::ValueType>{}(handle.value());
  }
};

} // namespace usd_stage_runner::physics
