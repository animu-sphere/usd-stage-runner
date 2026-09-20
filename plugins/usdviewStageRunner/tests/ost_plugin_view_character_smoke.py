"""Exercise a Jolt character and its follow camera from an OST-staged bundle."""

import sys
from pathlib import Path

from pxr import Plug, Usd


def main():
    plugin_root = str(Path(sys.argv[1]).resolve())
    stage_path = str(Path(sys.argv[2]).resolve())
    Plug.Registry().RegisterPlugins(plugin_root)

    from usdviewStageRunner import _usdviewStageRunner

    stage = Usd.Stage.Open(stage_path)
    if stage is None:
        raise RuntimeError("could not open the character follow-camera Stage")
    root_before = stage.GetRootLayer().ExportToString()
    player = stage.GetPrimAtPath("/World/PlayerCube").GetAttribute("xformOp:translate")
    camera = stage.GetPrimAtPath("/World/ThirdPersonCamera").GetAttribute("xformOp:translate")
    player_before = player.Get()
    camera_before = camera.Get()

    session = _usdviewStageRunner.createSession(stage, 1.0 / 60.0, 8)
    session.setActions(1.0, 0.0, True)
    session.play()
    session.advance(session.fixedStep)
    player_after = player.Get()
    camera_after = camera.Get()
    stats = session.stats
    if stats["characterControllerCount"] != 1 or stats["cameraRigCount"] != 1:
        raise RuntimeError("character or follow camera was not imported")
    if player_after[0] <= player_before[0] or player_after[1] <= player_before[1]:
        raise RuntimeError("right movement or jump did not move the character")
    if camera_after[0] <= camera_before[0] or stats["cameraRigUpdates"] < 1:
        raise RuntimeError("third-person camera did not follow the character")

    session.setActions(-1.0, 0.0, False)
    session.advance(session.fixedStep)
    if player.Get()[0] >= player_after[0]:
        raise RuntimeError("left movement did not move the character")

    session.stop()
    if player.Get() != player_before or camera.Get() != camera_before:
        raise RuntimeError("Stop did not discard runtime character and camera transforms")
    if stage.GetRootLayer().ExportToString() != root_before:
        raise RuntimeError("the persistent root layer changed")


if __name__ == "__main__":
    main()
