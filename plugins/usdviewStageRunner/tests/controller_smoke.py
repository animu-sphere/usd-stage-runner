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

    @staticmethod
    def singleShot(delay, callback):
        assert delay == 0
        callback()


class ElapsedTimer:
    def __init__(self):
        self.started = False

    def start(self):
        self.started = True

    def restart(self):
        if not self.started:
            raise RuntimeError("elapsed timer was not started")
        return 20


class Object:
    def __init__(self, parent=None):
        self.parent = parent


class Widget(Object):
    def isAncestorOf(self, widget):
        return widget.parent is self

    def window(self):
        widget = self
        while isinstance(widget.parent, Widget):
            widget = widget.parent
        return widget


class TextWidget(Widget):
    pass


class ButtonWidget(Widget):
    pass


class CameraPrim:
    def __init__(self, mode="thirdPerson"):
        self.mode = mode

    def IsA(self, cameraType):
        return cameraType is Camera

    def GetAppliedSchemas(self):
        return ["RunnerCameraRigAPI"]

    def GetAttribute(self, name):
        assert name == "runner:camera:mode"
        return types.SimpleNamespace(Get=lambda: self.mode)


class Camera:
    pass


class Stage:
    def __init__(self):
        self.prims = []

    def Traverse(self):
        return iter(self.prims)


class ViewSettings:
    def __init__(self):
        self._cameraPrim = None

    @property
    def cameraPrim(self):
        return self._cameraPrim

    @cameraPrim.setter
    def cameraPrim(self, value):
        if value is not None and not value.IsA(Camera):
            raise TypeError("usdview expects a camera Usd.Prim")
        self._cameraPrim = value


class Application:
    _instance = None

    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def installEventFilter(self, eventFilter):
        self.eventFilter = eventFilter


class Event:
    def __init__(self, eventType, key=0, modifiers=0, repeat=False):
        self._type = eventType
        self._key = key
        self._modifiers = modifiers
        self._repeat = repeat
        self.accepted = False

    def type(self):
        return self._type

    def key(self):
        return self._key

    def modifiers(self):
        return self._modifiers

    def isAutoRepeat(self):
        return self._repeat

    def accept(self):
        self.accepted = True


class StageView(Widget):
    def setFocus(self):
        self.focused = True

    def send(self, event):
        return Application.instance().eventFilter.eventFilter(self, event)


class MainWindow(Widget):
    def __init__(self):
        super().__init__()
        self.stageView = StageView(self)

    def findChild(self, childType):
        assert childType is StageView
        return self.stageView


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

    def setActions(self, moveX, moveY, jump):
        self.calls.append(("actions", moveX, moveY, jump))


class DataModel:
    def __init__(self):
        self.signalStageReplaced = Signal()
        self.clearCount = 0
        self.viewSettings = ViewSettings()

    def _clearCaches(self):
        self.clearCount += 1


class Api:
    def __init__(self):
        self.stage = Stage()
        self.qMainWindow = MainWindow()
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
    pxr.UsdGeom = types.SimpleNamespace(Camera=Camera)
    usdviewq = types.ModuleType("pxr.Usdviewq")
    usdviewq.__path__ = []
    qt = types.ModuleType("pxr.Usdviewq.qt")
    keys = types.SimpleNamespace(
        Key_A=65, Key_D=68, Key_W=87, Key_S=83,
        Key_Left=16777234, Key_Right=16777236,
        Key_Up=16777235, Key_Down=16777237, Key_Space=32,
        ControlModifier=1, AltModifier=2, MetaModifier=4,
    )
    eventTypes = types.SimpleNamespace(
        KeyPress=1, KeyRelease=2, FocusOut=3,
        ApplicationDeactivate=4, WindowDeactivate=5,
        ShortcutOverride=6,
    )
    qt.QtCore = types.SimpleNamespace(
        QObject=Object, QTimer=Timer, QElapsedTimer=ElapsedTimer,
        Qt=keys, QEvent=eventTypes,
    )
    qt.QtWidgets = types.SimpleNamespace(
        QApplication=Application, QWidget=Widget,
        QLineEdit=TextWidget, QTextEdit=TextWidget,
        QPlainTextEdit=TextWidget, QAbstractSpinBox=TextWidget,
        QComboBox=TextWidget, QMenu=TextWidget,
        QAbstractButton=ButtonWidget,
    )
    stageView = types.ModuleType("pxr.Usdviewq.stageView")
    stageView.StageView = StageView

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
            "pxr.Usdviewq.stageView": stageView,
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
    rig = CameraPrim()
    api.stage.prims.append(rig)
    controller = controllerModule.StageRunnerController(api)

    controller.play()
    session = sessions[-1]
    if not controller._timer.active or session.calls != [("actions", 0, 0, False), "play"]:
        raise RuntimeError("play did not start both the session and timer")
    if api.dataModel.viewSettings.cameraPrim is not rig:
        raise RuntimeError("Play did not select the third-person follow camera")

    view = api.qMainWindow.stageView
    if not view.focused:
        raise RuntimeError("play did not focus the viewport")
    if not view.send(Event(1, 87)) or session.calls[-1] != ("actions", 0, 1, False):
        raise RuntimeError("W did not set forward movement")
    view.send(Event(1, 68))
    view.send(Event(1, 32))
    if session.calls[-1] != ("actions", 1, 1, True):
        raise RuntimeError("diagonal movement and jump did not reach the session")
    view.send(Event(2, 87))
    if session.calls[-1] != ("actions", 1, 0, True):
        raise RuntimeError("key release did not clear forward movement")
    tree = Widget(api.qMainWindow)
    Application.instance().eventFilter.eventFilter(tree, Event(1, 87))
    if session.calls[-1] != ("actions", 1, 1, True):
        raise RuntimeError("W stopped working when focus moved within usdview")
    Application.instance().eventFilter.eventFilter(tree, Event(2, 87))
    if session.calls[-1] != ("actions", 1, 0, True):
        raise RuntimeError("key release outside the viewport left movement held")
    text = TextWidget(api.qMainWindow)
    if Application.instance().eventFilter.eventFilter(text, Event(1, 87)):
        raise RuntimeError("typing in an editor was consumed as movement")
    override = Event(6, 32)
    if not Application.instance().eventFilter.eventFilter(tree, override) or not override.accepted:
        raise RuntimeError("usdview's Space playback shortcut was not overridden")
    override = Event(6, 16777234)
    if not Application.instance().eventFilter.eventFilter(tree, override) or not override.accepted:
        raise RuntimeError("usdview's Left arrow navigation was not overridden")
    if Application.instance().eventFilter.eventFilter(text, Event(6, 32)):
        raise RuntimeError("Space shortcut was overridden in a text editor")
    view.send(Event(2, 68))
    view.send(Event(2, 32))
    if not Application.instance().eventFilter.eventFilter(tree, Event(1, 16777234)) or session.calls[-1] != ("actions", -1, 0, False):
        raise RuntimeError("Left arrow did not move from a usdview panel")
    Application.instance().eventFilter.eventFilter(tree, Event(2, 16777234))
    if not Application.instance().eventFilter.eventFilter(tree, Event(1, 16777236)) or session.calls[-1] != ("actions", 1, 0, False):
        raise RuntimeError("Right arrow did not move from a usdview panel")
    Application.instance().eventFilter.eventFilter(tree, Event(2, 16777236))
    if not Application.instance().eventFilter.eventFilter(tree, Event(1, 32)) or session.calls[-1] != ("actions", 0, 0, True):
        raise RuntimeError("Space did not request a jump from a usdview panel")
    Application.instance().eventFilter.eventFilter(tree, Event(2, 32))
    if Application.instance().eventFilter.eventFilter(ButtonWidget(api.qMainWindow), Event(1, 32)):
        raise RuntimeError("Space on a button was consumed as jump")
    if Application.instance().eventFilter.eventFilter(text, Event(1, 32)):
        raise RuntimeError("Space in a text editor was consumed as jump")
    view.send(Event(4))
    if session.calls[-1] != ("actions", 0, 0, False):
        raise RuntimeError("focus loss left movement held")
    if view.send(Event(1, 87, modifiers=1)):
        raise RuntimeError("modified shortcuts should remain available to usdview")

    controller._timer.timeout.emit()
    if session.calls[-1] != ("advance", 0.02) or api.viewportUpdates != 1:
        raise RuntimeError("timer tick did not advance and refresh the Stage")

    view.send(Event(1, 87))
    controller.pause()
    if session.calls[-2:] != [("actions", 0, 0, False), "pause"]:
        raise RuntimeError("pause left a movement key held")
    if view.send(Event(1, 87)):
        raise RuntimeError("movement keys were consumed while paused")
    if view.send(Event(6, 32)):
        raise RuntimeError("Space shortcut was overridden while paused")
    selectedCamera = CameraPrim()
    api.dataModel.viewSettings.cameraPrim = selectedCamera
    controller.play()
    if session.calls[-2:] != [("actions", 0, 0, False), "play"]:
        raise RuntimeError("resuming replayed stale movement")
    if api.dataModel.viewSettings.cameraPrim is not selectedCamera:
        raise RuntimeError("Play replaced a camera selected by the user")

    controller.singleStep()
    if controller._timer.active or session.calls[-2:] != ["pause", "singleStep"]:
        raise RuntimeError("single-step did not pause a playing session before advancing")

    controller.stop()
    if view.send(Event(1, 87)):
        raise RuntimeError("movement keys were consumed while stopped")
    controller.reset()
    controller.play()
    view.send(Event(1, 87))
    session.failAdvance = True
    controller._timer.timeout.emit()
    if (controller._timer.active or session.state != "paused" or
            session.calls[-2:] != [("actions", 0, 0, False), "pause"]):
        raise RuntimeError("an update error did not pause the controller")
    if not api.statuses[-1].startswith("Stage Runner update failed:"):
        raise RuntimeError("an update error did not reach usdview status output")

    api.dataModel.signalStageReplaced.emit()
    if controller._session is not None or session.calls[-1] != "stop":
        raise RuntimeError("Stage replacement did not dispose of the current session")


if __name__ == "__main__":
    main()
