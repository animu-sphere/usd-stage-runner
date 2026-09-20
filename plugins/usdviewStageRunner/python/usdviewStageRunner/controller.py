"""Qt lifecycle adapter for the shared C++ Stage Runner session."""

from pxr import Tf, UsdGeom
from pxr.Usdviewq.qt import QtCore, QtWidgets
from pxr.Usdviewq.stageView import StageView

from . import _usdviewStageRunner as _native


_LEFT_KEYS = {QtCore.Qt.Key_A, QtCore.Qt.Key_Left}
_RIGHT_KEYS = {QtCore.Qt.Key_D, QtCore.Qt.Key_Right}
_FORWARD_KEYS = {QtCore.Qt.Key_W, QtCore.Qt.Key_Up}
_BACKWARD_KEYS = {QtCore.Qt.Key_S, QtCore.Qt.Key_Down}
_MOVEMENT_KEYS = _LEFT_KEYS | _RIGHT_KEYS | _FORWARD_KEYS | _BACKWARD_KEYS | {
    QtCore.Qt.Key_Space
}


class StageRunnerController(QtCore.QObject):
    """Own one StageSession for usdview's current Stage."""

    def __init__(self, usdviewApi):
        super().__init__(usdviewApi.qMainWindow)
        self._api = usdviewApi
        self._session = None
        self._pressedKeys = set()
        self._stageView = usdviewApi.qMainWindow.findChild(StageView)
        if self._stageView is None:
            raise RuntimeError("Stage Runner requires the usdview viewport")
        self._application = QtWidgets.QApplication.instance()
        self._application.installEventFilter(self)
        self._timer = QtCore.QTimer(usdviewApi.qMainWindow)
        self._timer.setInterval(16)
        self._timer.timeout.connect(self._tick)
        self._elapsed = QtCore.QElapsedTimer()
        self._advancing = False
        usdviewApi.dataModel.signalStageReplaced.connect(self._stageReplaced)

    def play(self):
        session = self._ensureSession()
        self._sendActions()
        session.play()
        self._selectFollowCamera()
        self._elapsed.start()
        self._timer.start()
        QtCore.QTimer.singleShot(0, self._stageView.setFocus)
        self._api.PrintStatus("Stage Runner: playing (WASD/arrows; Space to jump)")

    def pause(self):
        self._timer.stop()
        self._clearKeys()
        if self._session is not None:
            self._session.pause()
        self._api.PrintStatus("Stage Runner: paused")

    def stop(self):
        self._timer.stop()
        self._clearKeys()
        if self._session is not None:
            self._session.stop()
        self._refreshView()
        self._api.PrintStatus("Stage Runner: stopped and discarded runtime values")

    def singleStep(self):
        self._timer.stop()
        session = self._ensureSession()
        session.pause()
        session.singleStep()
        self._refreshView()
        self._api.PrintStatus("Stage Runner: advanced one fixed step")

    def reset(self):
        self._timer.stop()
        self._clearKeys()
        self._ensureSession().reset()
        self._refreshView()
        self._api.PrintStatus("Stage Runner: reset")

    def _ensureSession(self):
        if self._session is None:
            stage = self._api.stage
            if stage is None:
                raise RuntimeError("Stage Runner requires an open Stage")
            self._session = _native.createSession(stage)
        return self._session

    def eventFilter(self, watched, event):
        if event.type() in (QtCore.QEvent.ApplicationDeactivate, QtCore.QEvent.WindowDeactivate):
            self._clearKeys()
            return False
        if event.type() not in (
            QtCore.QEvent.ShortcutOverride, QtCore.QEvent.KeyPress,
            QtCore.QEvent.KeyRelease,
        ):
            return False
        key = event.key()
        if key not in _MOVEMENT_KEYS:
            return False
        if event.type() != QtCore.QEvent.KeyRelease and (
            self._session is None or self._session.state != "playing"
        ):
            return False
        if not self._isInputWidget(watched, key, event.type()):
            return False
        if event.type() != QtCore.QEvent.KeyRelease and event.modifiers() & (
            QtCore.Qt.ControlModifier | QtCore.Qt.AltModifier | QtCore.Qt.MetaModifier
        ):
            return False
        if event.type() == QtCore.QEvent.ShortcutOverride:
            event.accept()
            return True
        if not event.isAutoRepeat():
            if event.type() == QtCore.QEvent.KeyPress:
                self._pressedKeys.add(key)
            else:
                self._pressedKeys.discard(key)
            self._sendActions()
        return True

    def _isInputWidget(self, widget, key, eventType):
        if not isinstance(widget, QtWidgets.QWidget) or widget.window() != self._api.qMainWindow:
            return False
        if eventType == QtCore.QEvent.KeyRelease and key in self._pressedKeys:
            return True
        if isinstance(widget, (
            QtWidgets.QLineEdit, QtWidgets.QTextEdit, QtWidgets.QPlainTextEdit,
            QtWidgets.QAbstractSpinBox, QtWidgets.QComboBox, QtWidgets.QMenu,
            QtWidgets.QAbstractButton,
        )):
            return False
        return True

    def _selectFollowCamera(self):
        viewSettings = self._api.dataModel.viewSettings
        if viewSettings.cameraPrim:
            return
        for prim in self._api.stage.Traverse():
            if (prim.IsA(UsdGeom.Camera)
                    and "RunnerCameraRigAPI" in prim.GetAppliedSchemas()
                    and prim.GetAttribute("runner:camera:mode").Get() == "thirdPerson"):
                viewSettings.cameraPrim = prim
                return

    def _clearKeys(self):
        if self._pressedKeys:
            self._pressedKeys.clear()
            self._sendActions()

    def _sendActions(self):
        if self._session is None:
            return
        keys = self._pressedKeys
        moveX = int(bool(keys & _RIGHT_KEYS)) - int(bool(keys & _LEFT_KEYS))
        moveY = int(bool(keys & _FORWARD_KEYS)) - int(bool(keys & _BACKWARD_KEYS))
        self._session.setActions(moveX, moveY, QtCore.Qt.Key_Space in keys)

    def _tick(self):
        if self._advancing or self._session is None:
            return
        self._advancing = True
        try:
            elapsedSeconds = max(self._elapsed.restart(), 0) / 1000.0
            self._session.advance(elapsedSeconds)
            self._refreshView()
        except Exception as error:
            self._timer.stop()
            self._clearKeys()
            if self._session is not None:
                self._session.pause()
            Tf.Warn("Stage Runner paused after an update error: {}".format(error))
            self._api.PrintStatus("Stage Runner update failed: {}".format(error))
        finally:
            self._advancing = False

    def _stageReplaced(self):
        self._timer.stop()
        self._clearKeys()
        if self._session is not None:
            try:
                self._session.stop()
            except Exception as error:
                Tf.Warn("Stage Runner could not stop the replaced Stage: {}".format(error))
        self._session = None

    def _refreshView(self):
        self._api.dataModel._clearCaches()
        self._api.UpdateViewport()


_controller = None


def _getController(usdviewApi):
    global _controller
    if _controller is None or _controller._api is not usdviewApi:
        _controller = StageRunnerController(usdviewApi)
    return _controller


def _invoke(usdviewApi, methodName):
    try:
        getattr(_getController(usdviewApi), methodName)()
    except Exception as error:
        Tf.Warn("Stage Runner command failed: {}".format(error))
        usdviewApi.PrintStatus("Stage Runner command failed: {}".format(error))


def play(usdviewApi):
    _invoke(usdviewApi, "play")


def pause(usdviewApi):
    _invoke(usdviewApi, "pause")


def stop(usdviewApi):
    _invoke(usdviewApi, "stop")


def singleStep(usdviewApi):
    _invoke(usdviewApi, "singleStep")


def reset(usdviewApi):
    _invoke(usdviewApi, "reset")
