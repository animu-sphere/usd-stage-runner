"""usdview plugin container for the shared Stage Runner runtime."""

from pathlib import Path

from pxr import Plug, Usd, UsdGeom

# Import both binding modules before loading the native adapter. On Windows,
# Python's extension loader does not search PATH for every transitive DLL; the
# imports also register the Usd.Stage converter consumed by createSession().
del Usd, UsdGeom

_schemaResources = Path(__file__).resolve().parent / "resources" / "runnerSchema"
if (_schemaResources / "plugInfo.json").is_file():
    Plug.Registry().RegisterPlugins(str(_schemaResources))
del Path, Plug, _schemaResources

try:
    from pxr import Tf
    from pxr.Usdviewq.plugin import PluginContainer
except ModuleNotFoundError as error:
    if error.name is None or not error.name.startswith("pxr.Usdviewq"):
        raise
else:

    class StageRunnerPluginContainer(PluginContainer):
        def registerPlugins(self, plugRegistry, plugCtx):
            del plugCtx
            controller = self.deferredImport(".controller")
            self._play = plugRegistry.registerCommandPlugin(
                "StageRunnerPluginContainer.play",
                "Play",
                controller.play,
                description="Play the current Stage through the shared runtime session.",
            )
            self._pause = plugRegistry.registerCommandPlugin(
                "StageRunnerPluginContainer.pause",
                "Pause",
                controller.pause,
                description="Pause without discarding live runtime values.",
            )
            self._stop = plugRegistry.registerCommandPlugin(
                "StageRunnerPluginContainer.stop",
                "Stop",
                controller.stop,
                description="Stop and discard the runtime layer.",
            )
            self._singleStep = plugRegistry.registerCommandPlugin(
                "StageRunnerPluginContainer.singleStep",
                "Single Step",
                controller.singleStep,
                description="Advance exactly one fixed simulation step while paused.",
            )
            self._reset = plugRegistry.registerCommandPlugin(
                "StageRunnerPluginContainer.reset",
                "Reset",
                controller.reset,
                description="Rebuild the runtime session from persistent Stage values.",
            )

        def configureView(self, plugRegistry, plugUIBuilder):
            del plugRegistry
            menu = plugUIBuilder.findOrCreateMenu("Stage Runner")
            menu.addItem(self._play)
            menu.addItem(self._pause)
            menu.addItem(self._stop)
            menu.addSeparator()
            menu.addItem(self._singleStep)
            menu.addItem(self._reset)


    Tf.Type.Define(StageRunnerPluginContainer)
