#include "usd_stage_runner/stage/stage_session.h"
#include "usd_physics/jolt/availability.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/plug/registry.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usdGeom/xformable.h>

using namespace usd_stage_runner;

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

void compare(const stage::StageSession& a, const stage::StageSession& b) {
  for (const char* path : {"/World/PlayerCube", "/World/FirstPersonCamera", "/World/ThirdPersonCamera"}) {
    const auto* x = a.world().transform(path);
    const auto* y = b.world().transform(path);
    require((x == nullptr) == (y == nullptr), "prim presence differs");
    if (x) require(std::abs(x->translation.x - y->translation.x) < 1e-6 &&
                   std::abs(x->translation.y - y->translation.y) < 1e-6 &&
                   std::abs(x->translation.z - y->translation.z) < 1e-6, "physics/camera trajectory differs");
  }
  require(a.stats().physicsBodyCount == b.stats().physicsBodyCount &&
          a.stats().characterControllerCount == b.stats().characterControllerCount &&
          a.stats().cameraRigCount == b.stats().cameraRigCount &&
          a.stats().synchronizedTransforms == b.stats().synchronizedTransforms &&
          a.stats().physicsBodyUpdates == b.stats().physicsBodyUpdates, "import or synchronization differs");
}

int main() {
  try {
    pxr::PlugRegistry::GetInstance().RegisterPlugins(TEST_SCHEMA_PATH);
    auto factory = [] { return usd_physics::jolt::createWorld(); };
    for (const char* fixture : {"falling_cube", "character_walk", "third_person_camera", "character_follow_camera"}) {
      auto legacy = pxr::UsdStage::Open(std::string(TEST_FIXTURE_PATH) + "/" + fixture + ".usda");
      auto standard = pxr::UsdStage::Open(std::string(TEST_FIXTURE_PATH) + "/standard_" + fixture + ".usda");
      require(legacy && standard, "could not open parity fixtures");
      std::string before, after;
      standard->GetRootLayer()->ExportToString(&before);
      {
        stage::StageSession a(legacy, {}, factory), b(standard, {}, factory);
        compare(a, b);
        a.play(); b.play();
        for (int i = 0; i < 180; ++i) {
          input::ActionState actions;
          actions.set(std::string(input::actions::moveX), i < 60 ? 1.0 : 0.0);
          actions.set(std::string(input::actions::jump), i >= 30 && i < 35 ? 1.0 : 0.0);
          a.setActions(actions); b.setActions(actions);
          (void)a.advance(a.fixedStep()); (void)b.advance(b.fixedStep());
          compare(a, b);
        }
        a.reset(); b.reset(); compare(a, b);
        standard->GetRootLayer()->ExportToString(&after);
        require(before == after, "simulation and Reset must preserve persistent opinions");
        standard->GetRootLayer()->ExportToString(&after);
        require(before == after, "simulation and Reset must preserve persistent opinions");
        // Reset restores captured position even after an external persistent edit.
        const pxr::TfToken translate("xformOp:translate");
        const pxr::SdfPath player("/World/PlayerCube");
        legacy->GetPrimAtPath(player).GetAttribute(translate).Set(pxr::GfVec3d(7, 2, 0));
        standard->GetPrimAtPath(player).GetAttribute(translate).Set(pxr::GfVec3d(7, 2, 0));
        standard->GetRootLayer()->ExportToString(&before);
        a.reset(); b.reset(); compare(a, b);
        a.singleStep(); b.singleStep(); compare(a, b);
        a.stop(); b.stop(); compare(a, b);
        require(b.world().transform("/World/PlayerCube")->translation.x == 7,
                "Stop must rebuild persistent position");
      }
      standard->GetRootLayer()->ExportToString(&after);
      require(before == after && standard->GetSessionLayer()->GetSubLayerPaths().empty(),
              "runtime must preserve root and detach its layer");
    }
    auto invalid = pxr::UsdStage::Open(std::string(TEST_FIXTURE_PATH) + "/standard_falling_cube.usda");
    invalid->GetPrimAtPath(pxr::SdfPath("/World/PlayerCube")).AddAppliedSchema(pxr::TfToken("RunnerPhysicsBodyAPI"));
    int created = 0;
    try {
      stage::StageSession session(invalid, {}, [&] { ++created; return factory(); });
      throw std::logic_error("mixed physics APIs accepted");
    } catch (const std::runtime_error& error) {
      require(std::string(error.what()).find("must not share a prim") != std::string::npos && created == 0,
              "mixed declarations must fail before creating a world");
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
