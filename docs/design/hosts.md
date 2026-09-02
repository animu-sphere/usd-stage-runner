# Host Integration

Status: reusable Stage play session, discardable runtime layer,
`stage_runner`, and usdview adapters implemented; OST Plugin View planned

Runtime libraries remain host-independent so the same world and systems can be
used from three hosts:

```text
runtime libraries
    +--> apps/stage_runner
    +--> plugins/usdviewStageRunner
    `--> OST Plugin View
```

## Standalone host

`stage_runner` registers schemas, opens a Stage, selects input and physics
adapters, polls input, and runs the frame loop. `stageRuntime::StageSession`
builds the Runtime World, imports systems, executes fixed updates, rebuilds on
reset, and coordinates incremental synchronization. Command-line parsing and
the host loop do not own domain behavior.

## usdview plugin

The implemented usdview plugin obtains the current Stage and provides:

- play, pause, and stop;
- single-step and reset;
- fixed 1/60-second frame ticking through a Qt timer; and
- automatic session disposal when usdview replaces the current Stage.

Camera selection, editable runtime settings, and debug visualization toggles
remain later host-tooling work.

Physics, behavior, camera, and vehicle logic remain in shared libraries. The
plugin supplies usdview lifecycle, UI, and rendering integration only.

Host integration is its own delivery milestone after the initial character and
camera slices. A Stage that runs in `stage_runner` must use the same Runtime
World, subsystem implementations, and fixed-step semantics when played in
usdview. The plugin remains a thin lifecycle and UI adapter.

The first usable controls—play, pause, stop, single-step, and reset—are exposed
from a `Stage Runner` menu. A small native Python binding passes usdview's
existing `Usd.Stage` directly into `StageSession`; the Python layer owns only
commands, elapsed host time, redraw requests, and Stage replacement handling.
The milestone is not complete until OST Plugin View also reuses the discardable
[play-session layer](usd-integration.md#play-session-layer).

The implemented `runtimeCore::PlaySession` defines lifecycle and timing
semantics without depending on OpenUSD or a host SDK. The implemented
`stageRuntime::StageSession` binds its callbacks to Stage import, state rebuild,
one fixed update, and dirty USD synchronization while accepting a
backend-neutral physics-world factory. `stage_runner` drives this public
boundary. It writes simulation results to its own anonymous sublayer beneath
the Stage session layer, discards live results on reset or stop, and detaches
only that owned layer when destroyed. Editor adapters can consume this boundary
without redirecting the host's persistent edit target.

## OST Plugin View and OpenStrata

OpenStrata owns reproducible environment setup, dependency pinning, build,
test, packaging, CI, and host integration. It does not become a runtime API
dependency.

```text
OpenStrata      -> build / host / package
Stage Runner    -> ordinary C++ runtime libraries
```

The existing `openstrata.toml` and `openstrata.ci.yaml` direction remains in
place, while direct CMake builds continue to work. A later OST Plugin View
integration should load the same runtime libraries rather than fork their
implementation.
