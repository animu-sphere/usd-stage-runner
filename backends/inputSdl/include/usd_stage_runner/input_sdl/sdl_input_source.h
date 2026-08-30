#pragma once

#include "usd_stage_runner/input/input_source.h"

#include <memory>
#include <string_view>

namespace usd_stage_runner::input_sdl {

class SdlInputSource final : public input::InputSource {
public:
  SdlInputSource();
  ~SdlInputSource() override;

  SdlInputSource(SdlInputSource&&) noexcept;
  SdlInputSource& operator=(SdlInputSource&&) noexcept;
  SdlInputSource(const SdlInputSource&) = delete;
  SdlInputSource& operator=(const SdlInputSource&) = delete;

  [[nodiscard]] bool available() const noexcept;
  [[nodiscard]] std::string_view error() const noexcept;
  bool poll(input::ActionState& actions) override;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace usd_stage_runner::input_sdl
