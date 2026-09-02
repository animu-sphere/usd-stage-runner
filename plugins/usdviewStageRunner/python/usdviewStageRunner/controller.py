"""Qt lifecycle adapter for the shared C++ Stage Runner session."""

from pxr import Tf
from pxr.Usdviewq.qt import QtCore

from . import _usdviewStageRunner as _native


class StageRunnerController:
    """Own one StageSession for usdview's current Stage."""

    def __init__(self, usdviewApi):
        self._api = usdviewApi
        self._session = None
        self._timer = QtCore.QTimer(usdviewApi.qMainWindow)
        self._timer.setInterval(16)
        self._timer.timeout.connect(self._tick)
        self._elapsed = QtCore.QElapsedTimer()
        self._advancing = False
        usdviewApi.dataModel.signalStageReplaced.connect(self._stageReplaced)

    def play(self):
        session = self._ensureSession()
        session.play()
        self._elapsed.start()
        self._timer.start()
        self._api.PrintStatus("Stage Runner: playing")

    def pause(self):
        self._timer.stop()
        if self._session is not None:
            self._session.pause()
        self._api.PrintStatus("Stage Runner: paused")

    def stop(self):
        self._timer.stop()
        if self._session is not None:
            self._session.stop()
        self._refreshView()
        self._api.PrintStatus("Stage Runner: stopped and discarded runtime values")

    def singleStep(self):
        self._timer.stop()
        self._ensureSession().singleStep()
        self._refreshView()
        self._api.PrintStatus("Stage Runner: advanced one fixed step")

    def reset(self):
        self._timer.stop()
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
            if self._session is not None:
                self._session.pause()
            Tf.Warn("Stage Runner paused after an update error: {}".format(error))
            self._api.PrintStatus("Stage Runner update failed: {}".format(error))
        finally:
            self._advancing = False

    def _stageReplaced(self):
        self._timer.stop()
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
