#pragma once

#include "usd_stage_runner/runtime/component_registry.h"

#include <cstddef>
#include <unordered_set>
#include <vector>

namespace usd_stage_runner::runtime {

class DirtyPrimQueue {
public:
  bool mark(const PrimId& prim);
  bool erase(const PrimId& prim) noexcept;
  [[nodiscard]] std::size_t size() const noexcept {
    return queued_.size();
  }
  [[nodiscard]] bool empty() const noexcept {
    return queued_.empty();
  }
  std::vector<PrimId> take();

private:
  std::vector<PrimId> order_;
  std::unordered_set<PrimId> queued_;
};

} // namespace usd_stage_runner::runtime
