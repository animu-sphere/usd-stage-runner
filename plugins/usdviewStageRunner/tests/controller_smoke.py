"""Dependency-light lifecycle smoke test for the usdview Qt controller."""

import importlib.util
from pathlib import Path
import sys
import types


class Signal:
    def __init__(self):
        self._callbacks = []

    def connect(self, callback):
        self._callbacks.append(callback)

    def emit(self):
        for callback in self._callbacks:
            callback()


class Timer:
    def __init__(self, parent):
        self.parent = parent
        self.interval = None
        self.active = False
        self.timeout = Signal()

    def setInterval(self, interval):
        self.interval = interval

    def start(self):
        self.active = True

    def stop(self):
        self.active = False


class ElapsedTimer:
    def __init__(self):
        self.started = False

    def start(self):
        self.started = True

    def restart(self):
        if not self.started:
            raise RuntimeError("elapsed timer was not started")
        return 20


class Session:
    def __init__(self):
        self.calls = []
        self.state = "paused"
        self.failAdvance = False

    def play(self):
        self.calls.append("play")
        self.state = "playing"

    def pause(self):
        self.calls.append("pause")
        self.state = "paused"

    def stop(self):
        self.calls.append("stop")
        self.state = "paused"

    def singleStep(self):
        if self.state != "paused":
            raise RuntimeError("single-step requires a paused play session")
        self.calls.append("singleStep")

    def reset(self):
        self.calls.append("reset")
        self.state = "paused"

    def advance(self, frameTime):
        self.calls.append(("advance", frameTime))
        if self.failAdvance:
            raise RuntimeError("advance failed")


class DataModel:
    def __init__(self):
        self.signalStageReplaced = Signal()
        self.clearCount = 0

    def _clearCaches(self):
        self.clearCount += 1


class Api:
    def __init__(self):
        self.stage = object()
        self.qMainWindow = object()
        self.dataModel = DataModel()
        self.statuses = []
        self.viewportUpdates = 0

    def PrintStatus(self, status):
        self.statuses.append(status)

    def UpdateViewport(self):
        self.viewportUpdates += 1


def loadController():
    packageDir = Path(__file__).resolve().parents[1] / "python" / "usdviewStageRunner"

    pxr = types.ModuleType("pxr")
    pxr.__path__ = []
    pxr.Tf = types.SimpleNamespace(Warn=lambda message: None)
    usdviewq = types.ModuleType("pxr.Usdviewq")
    usdviewq.__path__ = []
    qt = types.ModuleType("pxr.Usdviewq.qt")
    qt.QtCore = types.SimpleNamespace(QTimer=Timer, QElapsedTimer=ElapsedTimer)

    package = types.ModuleType("usdviewStageRunner")
    package.__path__ = [str(packageDir)]
    native = types.ModuleType("usdviewStageRunner._usdviewStageRunner")
    sessions = []

    def createSession(stage):
        if stage is None:
            raise RuntimeError("missing Stage")
        session = Session()
        sessions.append(session)
        return session

    native.createSession = createSession
    sys.modules.update(
        {
            "pxr": pxr,
            "pxr.Usdviewq": usdviewq,
            "pxr.Usdviewq.qt": qt,
            "usdviewStageRunner": package,
            "usdviewStageRunner._usdviewStageRunner": native,
        }
    )

    spec = importlib.util.spec_from_file_location(
        "usdviewStageRunner.controller", packageDir / "controller.py"
    )
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module, sessions


def main():
    controllerModule, sessions = loadController()
    api = Api()
    controller = controllerModule.StageRunnerController(api)

    controller.play()
    session = sessions[-1]
    if not controller._timer.active or session.calls != ["play"]:
        raise RuntimeError("play did not start both the session and timer")

    controller._timer.timeout.emit()
    if session.calls[-1] != ("advance", 0.02) or api.viewportUpdates != 1:
        raise RuntimeError("timer tick did not advance and refresh the Stage")

    controller.singleStep()
    if controller._timer.active or session.calls[-2:] != ["pause", "singleStep"]:
        raise RuntimeError("single-step did not pause a playing session before advancing")

    controller.reset()
    controller.play()
    session.failAdvance = True
    controller._timer.timeout.emit()
    if controller._timer.active or session.state != "paused":
        raise RuntimeError("an update error did not pause the controller")
    if not api.statuses[-1].startswith("Stage Runner update failed:"):
        raise RuntimeError("an update error did not reach usdview status output")

    api.dataModel.signalStageReplaced.emit()
    if controller._session is not None or session.calls[-1] != "stop":
        raise RuntimeError("Stage replacement did not dispose of the current session")


if __name__ == "__main__":
    main()
