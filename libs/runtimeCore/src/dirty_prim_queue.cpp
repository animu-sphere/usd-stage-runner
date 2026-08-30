#include "usd_stage_runner/runtime/dirty_prim_queue.h"

#include <algorithm>
#include <utility>

namespace usd_stage_runner::runtime {

bool DirtyPrimQueue::mark(const PrimId& prim) {
  if (!queued_.insert(prim).second) {
    return false;
  }
  order_.push_back(prim);
  return true;
}

bool DirtyPrimQueue::erase(const PrimId& prim) noexcept {
  if (queued_.erase(prim) == 0) {
    return false;
  }
  order_.erase(std::remove(order_.begin(), order_.end(), prim), order_.end());
  return true;
}

std::vector<PrimId> DirtyPrimQueue::take() {
  queued_.clear();
  return std::exchange(order_, {});
}

} // namespace usd_stage_runner::runtime
