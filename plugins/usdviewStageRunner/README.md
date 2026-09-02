# usdview Stage Runner

This plugin adds a **Stage Runner** menu to usdview. Its Play, Pause, Stop,
Single Step, and Reset commands drive the same `stageRuntime::StageSession`
used by the standalone `stage_runner` executable. Live transform values are
written only to the session's owned anonymous runtime layer; Stop and Reset
discard those values without changing the Stage's persistent root layer.

The build stages a loadable package under
`build/.../plugins/usdviewStageRunner/python`. To run it from a build tree, add
that `python` directory to `PYTHONPATH` and the contained
`usdviewStageRunner` directory to `PXR_PLUGINPATH_NAME`, then launch usdview in
the matching OpenUSD/Python runtime. Installed files live under
`lib/usd-stage-runner/usdview` and use the same two environment entries.
The package includes and registers the codeless Runner schema resources before
usdview opens a Stage.

The adapter currently uses a 1/60-second fixed step, an eight-step catch-up
bound, `/World/PlayerCube` as the player prim, and the Jolt backend when a
Stage declares physics bodies. It intentionally owns host lifecycle, timing,
and UI only; import, simulation, camera evaluation, synchronization, and layer
discard remain in shared libraries.
