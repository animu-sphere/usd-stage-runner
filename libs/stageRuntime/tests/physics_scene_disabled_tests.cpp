#include "usd_stage_runner/stage/physics_scene_runtime.h"

#include <string>

int main() {
  using usd_stage_runner::stage::PhysicsSceneRuntime;
  if (PhysicsSceneRuntime::available())
    return 1;
  int created = 0;
  PhysicsSceneRuntime runtime([&](auto) {
    ++created;
    return std::unique_ptr<usd_physics::core::PhysicsWorld>{};
  });
  try {
    runtime.rebuild({});
  } catch (const std::runtime_error& error) {
    return created == 0 && !runtime.world() &&
                   std::string(error.what()).find("USD_STAGE_RUNNER_ENABLE_PHYSICS_SCENE") != std::string::npos
               ? 0 : 2;
  }
  return 3;
}
