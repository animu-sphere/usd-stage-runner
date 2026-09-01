# Host Integration

Status: reusable Stage play session and `stage_runner` adapter implemented;
discardable layer, usdview, and OST Plugin View planned

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

The planned usdview plugin obtains the current Stage and provides:

- play, pause, and stop;
- frame ticking and fixed-step settings;
- camera selection;
- runtime settings; and
- debug visualization toggles.

Physics, behavior, camera, and vehicle logic remain in shared libraries. The
plugin supplies usdview lifecycle, UI, and rendering integration only.

Host integration is its own delivery milestone after the initial character and
camera slices. A Stage that runs in `stage_runner` must use the same Runtime
World, subsystem implementations, and fixed-step semantics when played in
usdview. The plugin remains a thin lifecycle and UI adapter.

The first usable controls are play, pause, single-step, and reset. A small
prototype may land earlier to prove host independence, but the milestone is not
complete until simulation writes use the discardable
[play-session layer](usd-integration.md#play-session-layer).

The implemented `runtimeCore::PlaySession` defines lifecycle and timing
semantics without depending on OpenUSD or a host SDK. The implemented
`stageRuntime::StageSession` binds its callbacks to Stage import, state rebuild,
one fixed update, and dirty USD synchronization while accepting a
backend-neutral physics-world factory. `stage_runner` drives this public
boundary. The remaining safety work is the discardable layer required before
editor adapters consume it.

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
