import sys

from pxr import Usd
from usdviewStageRunner import _usdviewStageRunner


def main():
    stage = Usd.Stage.Open(sys.argv[1])
    if stage is None:
        raise RuntimeError("could not open smoke-test Stage")

    root_before = stage.GetRootLayer().ExportToString()
    session = _usdviewStageRunner.createSession(stage, 1.0 / 60.0, 8)
    session.setActions(1.0, 0.0, False)
    session.play()
    result = session.advance(session.fixedStep)
    if result["steps"] != 1 or session.stats["fixedSteps"] != 1:
        raise RuntimeError("native adapter did not advance exactly one fixed step")
    session.pause()
    session.singleStep()
    if session.state != "paused" or session.stats["fixedSteps"] != 2:
        raise RuntimeError("native adapter did not preserve single-step semantics")
    session.stop()
    if stage.GetRootLayer().ExportToString() != root_before:
        raise RuntimeError("native adapter changed the persistent root layer")


if __name__ == "__main__":
    main()
