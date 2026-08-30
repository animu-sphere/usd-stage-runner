# Host Integration

Status: `stage_runner` implemented; usdview and OST Plugin View planned

Runtime libraries remain host-independent so the same world and systems can be
used from three hosts:

```text
runtime libraries
    +--> apps/stage_runner
    +--> plugins/usdviewStageRunner
    `--> OST Plugin View
```

## Standalone host

`stage_runner` opens a Stage, builds the Runtime World, selects adapters, runs
the frame loop, and coordinates synchronization. Command-line parsing and the
host loop do not own domain behavior.

## usdview plugin

The planned usdview plugin obtains the current Stage and provides:

- play, pause, and stop;
- frame ticking and fixed-step settings;
- camera selection;
- runtime settings; and
- debug visualization toggles.

Physics, behavior, camera, and vehicle logic remain in shared libraries. The
plugin supplies usdview lifecycle, UI, and rendering integration only.

A small usdview prototype may be delivered before all domain milestones to
prove host independence, but full editor integration must respect the
[play-session layer](usd-integration.md#play-session-layer).

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
