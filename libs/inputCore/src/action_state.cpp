#include "usd_stage_runner/input/action_state.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace usd_stage_runner::input {

void ActionState::set(std::string action, double value) {
  if (action.empty()) {
    return;
  }
  const double normalized = std::isfinite(value) ? std::clamp(value, -1.0, 1.0) : 0.0;
  values_.insert_or_assign(std::move(action), normalized);
}

double ActionState::value(std::string_view action) const {
  const auto found = values_.find(std::string(action));
  return found == values_.end() ? 0.0 : found->second;
}

void ActionState::clear() noexcept {
  values_.clear();
}

} // namespace usd_stage_runner::input
