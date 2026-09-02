import sys
from pathlib import Path

from pxr import Plug, Usd


def main():
    plugin_root = str(Path(sys.argv[1]).resolve())
    stage_path = str(Path(sys.argv[2]).resolve())

    registry = Plug.Registry()
    registry.RegisterPlugins(plugin_root)
    if registry.GetPluginWithName("runnerSchema") is None:
        raise RuntimeError("OST bundle did not register runnerSchema")
    if registry.GetPluginWithName("usdviewStageRunner") is None:
        raise RuntimeError("runnerSchema plugInfo did not include usdviewStageRunner")

    from usdviewStageRunner import _usdviewStageRunner

    stage = Usd.Stage.Open(stage_path)
    if stage is None:
        raise RuntimeError("could not open OST Plugin View smoke-test Stage")

    root_before = stage.GetRootLayer().ExportToString()
    session = _usdviewStageRunner.createSession(stage, 1.0 / 60.0, 8)
    session.play()
    result = session.advance(session.fixedStep)
    if result["steps"] != 1 or session.stats["fixedSteps"] != 1:
        raise RuntimeError("OST-staged adapter did not advance one fixed step")
    session.stop()
    if stage.GetRootLayer().ExportToString() != root_before:
        raise RuntimeError("OST-staged adapter changed the persistent root layer")


if __name__ == "__main__":
    main()
