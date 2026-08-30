#pragma once

#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/runtime/runtime_world.h"

namespace usd_stage_runner::input {

struct MovementIntent {
  double x{0.0};
  double y{0.0};
};

void updateMovementIntent(runtime::RuntimeWorld& world, const runtime::PrimId& prim,
                          const ActionState& actions);

bool applyMovementIntent(runtime::RuntimeWorld& world, const runtime::PrimId& prim,
                         double speed, double seconds);

} // namespace usd_stage_runner::input
