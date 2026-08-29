#include "usd_stage_runner/runtime/component_registry.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

struct Transform {
  double x;
};

struct Label {
  std::string value;
};

struct MoveOnly {
  explicit MoveOnly(int value) : value(std::make_unique<int>(value)) {}
  std::unique_ptr<int> value;
};

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

} // namespace

int main() {
  using usd_stage_runner::runtime::ComponentRegistry;
  using usd_stage_runner::runtime::RuntimeWorld;

  ComponentRegistry registry;
  registry.emplace<Transform>("/World/Cube", Transform{1.0});
  registry.emplace<Label>("/World/Cube", Label{"player"});
  registry.emplace<MoveOnly>("/World/Cube", 7);
  if (registry.componentCount() != 3 || !registry.contains<Transform>("/World/Cube") ||
      registry.get<Label>("/World/Cube")->value != "player" ||
      *registry.get<MoveOnly>("/World/Cube")->value != 7) {
    return fail("the registry must store independently typed components per prim");
  }

  registry.emplace<Transform>("/World/Cube", Transform{2.0});
  if (registry.componentCount() != 3 || registry.get<Transform>("/World/Cube")->x != 2.0) {
    return fail("emplacing the same component type must replace it");
  }
  if (!registry.remove<Label>("/World/Cube") || registry.contains<Label>("/World/Cube")) {
    return fail("typed component removal failed");
  }

  RuntimeWorld world;
  world.addPrim("/World/Cube");
  world.emplaceComponent<Transform>("/World/Cube", Transform{3.0});
  if (world.primCount() != 1 || world.component<Transform>("/World/Cube")->x != 3.0) {
    return fail("the Runtime World must index components by prim identity");
  }
  if (!world.removePrim("/World/Cube") || world.componentCount() != 0) {
    return fail("removing a prim must remove its runtime components");
  }

  try {
    world.emplaceComponent<Transform>("/Missing", Transform{});
    return fail("components must not attach to an unknown prim");
  } catch (const std::out_of_range&) {
  }

  return 0;
}
