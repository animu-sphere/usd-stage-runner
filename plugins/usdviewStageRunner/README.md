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

Choose **Stage Runner > Play** to focus the viewport. Use **WASD** to move
`/World/PlayerCube`; the arrow keys also work when the viewport has focus.
**Space** requests a jump for a character Stage while the viewport has focus.
Releasing a key or deactivating usdview clears that input. `minimal.usda`
demonstrates movement without Jolt; physics and jumping in
`character_walk.usda` require a build with Jolt.

For OpenStrata Plugin View, the root `plugin-view` intent stages this package
inside the `runnerSchema` bundle, whose schema `plugInfo.json` includes the
Python plugin registration:

```powershell
ost build --intent plugin-view
$fixture = (Resolve-Path .\tests\fixtures\minimal.usda).Path
ost plugin view plugins/runnerSchema $fixture --profile lookdev
```

The repository [quick start](../../README.md#quick-start-usdview-via-openstrata)
shows how to adopt a usdview-capable OpenUSD runtime before these commands.

OpenStrata 0.23.0 supports a dedicated `usdview-plugin` bundle. This repository
continues staging the adapter into `runnerSchema` until its pinned runtime and
CI lane provide usdview. The current `usd` profile cannot promise an
independent host add-on with the required `usdview` capability.

This path uses the same package and native module as the ordinary usdview
adapter; it does not introduce another host implementation.
