#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace usd_stage_runner::input {

namespace actions {
inline constexpr std::string_view moveX{"move.x"};
inline constexpr std::string_view moveY{"move.y"};
inline constexpr std::string_view jump{"jump"};
} // namespace actions

class ActionState {
public:
  void set(std::string action, double value);
  [[nodiscard]] double value(std::string_view action) const;
  void clear() noexcept;

private:
  std::unordered_map<std::string, double> values_;
};

} // namespace usd_stage_runner::input
