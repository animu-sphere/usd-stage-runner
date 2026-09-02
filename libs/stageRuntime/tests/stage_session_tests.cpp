#include "usd_stage_runner/input/action_state.h"
#include "usd_stage_runner/stage/stage_session.h"

#include <cmath>
#include <iostream>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/sdf/layer.h>
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
  std::string persistentLayerBeforePlay;
  if (!stage->GetRootLayer()->ExportToString(&persistentLayerBeforePlay)) {
    return fail("could not inspect the persistent Stage fixture layer");
  }
  const auto hostSessionLayer = pxr::SdfLayer::CreateAnonymous("host-session.usda");
  if (!hostSessionLayer) {
    return fail("could not create the host session layer fixture");
  }
  stage->GetSessionLayer()->SetSubLayerPaths({hostSessionLayer->GetIdentifier()});

  {
    usd_stage_runner::stage::StageSessionConfig config;
    config.fixedStep = usd_stage_runner::stage::StageSession::Duration{0.01};
    config.playerSpeed = 2.0;
    usd_stage_runner::stage::StageSession session(stage, config);
    if (session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
        session.world().primCount() != 2) {
      return fail("a Stage session must import the Stage and begin paused");
    }
    const auto runtimeSublayers = stage->GetSessionLayer()->GetSubLayerPaths();
    if (runtimeSublayers.size() != 2 ||
        runtimeSublayers.back() != hostSessionLayer->GetIdentifier()) {
      return fail("a Stage session must attach one runtime layer and preserve host sublayers");
    }
    const auto runtimeLayer = pxr::SdfLayer::Find(runtimeSublayers.front());
    if (!runtimeLayer || !runtimeLayer->IsEmpty()) {
      return fail("the attached runtime layer must begin empty");
    }

    usd_stage_runner::input::ActionState actions;
    actions.set(std::string{usd_stage_runner::input::actions::moveX}, 1.0);
    session.setActions(actions);
    session.play();
    (void)session.advance(session.fixedStep());
    const auto* moved = session.world().transform("/World/PlayerCube");
    pxr::GfVec3d authored;
    std::string persistentLayerDuringPlay;
    if (moved == nullptr || !close(moved->translation.x, 0.02) || !translate.Get(&authored) ||
        !close(authored[0], 0.02) || session.stats().fixedSteps != 1 ||
        session.stats().synchronizedTransforms != 1 || runtimeLayer->IsEmpty() ||
        !stage->GetRootLayer()->ExportToString(&persistentLayerDuringPlay) ||
        persistentLayerDuringPlay != persistentLayerBeforePlay) {
      return fail("StageSession must synchronize through the runtime layer only");
    }

    session.reset();
    const auto* restored = session.world().transform("/World/PlayerCube");
    if (session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
        restored == nullptr || !close(restored->translation.x, 0.0) || !translate.Get(&authored) ||
        !close(authored[0], 0.0) || session.stats().fixedSteps != 0 ||
        session.stats().synchronizedTransforms != 0 || !runtimeLayer->IsEmpty()) {
      return fail("reset must discard runtime values and rebuild the captured initial state");
    }

    if (!translate.Set(pxr::GfVec3d{4.0, 1.0, 0.0})) {
      return fail("failed to author an external Stage transform edit");
    }
    std::string persistentLayerAfterExternalEdit;
    if (!stage->GetRootLayer()->ExportToString(&persistentLayerAfterExternalEdit) ||
        persistentLayerAfterExternalEdit == persistentLayerBeforePlay) {
      return fail("the external Stage edit must reach the persistent layer");
    }
    session.reset();
    const auto* externallyRestored = session.world().transform("/World/PlayerCube");
    std::string persistentLayerAfterReset;
    if (externallyRestored == nullptr || !close(externallyRestored->translation.x, 0.0) ||
        !translate.Get(&authored) || !close(authored[0], 0.0) ||
        session.stats().synchronizedTransforms != 1 ||
        !stage->GetRootLayer()->ExportToString(&persistentLayerAfterReset) ||
        persistentLayerAfterReset != persistentLayerAfterExternalEdit) {
      return fail("reset must restore through the runtime layer without changing persistent edits");
    }

    session.singleStep();
    const auto* stepped = session.world().transform("/World/PlayerCube");
    if (stepped == nullptr || !close(stepped->translation.x, 0.02) ||
        session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
        session.stats().fixedSteps != 1) {
      return fail("single-step must use the rebuilt Stage session and remain paused");
    }

    session.stop();
    const auto* stopped = session.world().transform("/World/PlayerCube");
    if (session.state() != usd_stage_runner::runtime::PlaySession::State::paused ||
        stopped == nullptr || !close(stopped->translation.x, 4.0) || !translate.Get(&authored) ||
        !close(authored[0], 4.0) || !runtimeLayer->IsEmpty()) {
      return fail("stop must discard runtime values and expose the persistent Stage state");
    }
  }

  const auto remainingSublayers = stage->GetSessionLayer()->GetSubLayerPaths();
  if (remainingSublayers.size() != 1 ||
      remainingSublayers.front() != hostSessionLayer->GetIdentifier()) {
    return fail("destroying a Stage session must detach only its own runtime layer");
  }

  return 0;
}
