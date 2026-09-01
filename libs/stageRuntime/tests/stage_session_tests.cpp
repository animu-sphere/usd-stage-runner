#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/stage/stage_session.h"

#include <cmath>
#include <iostream>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <string>

namespace {

int fail(const std::string& message) {
  std::cerr << message << '\n';
  return 1;
}

bool close(double left, double right) {
  return std::abs(left - right) < 1.0e-9;
}

} // namespace

int main() {
  const auto stage = pxr::UsdStage::CreateInMemory();
  const auto player = pxr::UsdGeomXform::Define(stage, pxr::SdfPath{"/World/PlayerCube"});
  const auto translate = player.AddTranslateOp();
  if (!translate.Set(pxr::GfVec3d{0.0, 1.0, 0.0})) {
    return fail("could not author the Stage fixture translation");
  }

  usd_stage_runner::stage::StageSessionConfig config;
  config.fixedStep = usd_stage_runner::stage::StageSession::Duration{0.01};
  config.playerSpeed = 2.0;
  usd_stage_runner::stage::StageSession session(stage, config);
  if (session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
      session.world().primCount() != 2) {
    return fail("a Stage session must import the Stage and begin paused");
  }

  usd_stage_runner::input::ActionState actions;
  actions.set(std::string{usd_stage_runner::input::actions::moveX}, 1.0);
  session.setActions(actions);
  session.play();
  (void)session.advance(session.fixedStep());
  const auto* moved = session.world().transform("/World/PlayerCube");
  pxr::GfVec3d authored;
  if (moved == nullptr || !close(moved->translation.x, 0.02) || !translate.Get(&authored) ||
      !close(authored[0], 0.02) || session.stats().fixedSteps != 1 ||
      session.stats().synchronizedTransforms != 1) {
    return fail("StageSession must execute and synchronize the shared fixed-step path");
  }

  session.reset();
  const auto* restored = session.world().transform("/World/PlayerCube");
  if (session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
      restored == nullptr || !close(restored->translation.x, 0.0) || !translate.Get(&authored) ||
      !close(authored[0], 0.0) || session.stats().fixedSteps != 0 ||
      session.stats().synchronizedTransforms != 1) {
    return fail("reset must rebuild and synchronize the captured initial Stage state");
  }

  session.singleStep();
  const auto* stepped = session.world().transform("/World/PlayerCube");
  if (stepped == nullptr || !close(stepped->translation.x, 0.02) ||
      session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
      session.stats().fixedSteps != 1) {
    return fail("single-step must use the rebuilt Stage session and remain paused");
  }

  return 0;
}
