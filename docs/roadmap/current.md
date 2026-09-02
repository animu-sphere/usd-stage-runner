# Milestone 5: Host Integration

Status: in progress

The standalone `stage_runner` path now implements physics, character control,
camera following, collision avoidance, and incremental USD synchronization.
Its frame execution now passes through a host-neutral play-session controller
with deterministic play, pause, stop, single-step, and reset semantics. Stage
import, state rebuild, fixed updates, and incremental synchronization now live in the
reusable `stageRuntime::StageSession` boundary rather than the standalone host.
Simulation writes now use an owned anonymous runtime layer that reset, stop, and
destruction can discard without changing persistent authored layers. The
usdview adapter now drives that same session through a native Python binding
and thin menu/timer layer. This milestone next connects the safe play session
to OST Plugin View without duplicating simulation logic.

## Outcome

```text
OpenUSD Stage
    -> shared play-session runtime
        -> Runtime World and imported systems
        -> fixed-step execution
        -> discardable runtime layer writes
    -> stage_runner / usdview / OST Plugin View adapters
```

Each host owns lifecycle, UI, Stage access, and rendering hooks only. Physics,
character, camera, timing, and synchronization behavior remains shared.

## Scope

### Shared play session

The reusable host-facing boundary, controlled lifecycle, anonymous runtime
layer, and root-layer preservation tests are implemented.

### Host adapters

- The thin `plugins/usdviewStageRunner` adapter over the shared session is
  implemented.
- Connect the same session boundary to OST Plugin View.
- Reuse `character_walk.usda` and `third_person_camera.usda` to verify
  equivalent fixed-step and synchronization results in each host.

## Recommended PR sequence

The host-neutral lifecycle, reusable Stage session, discardable runtime layer,
standalone adapter, and usdview adapter are implemented. The next PR connects
OST Plugin View to the same boundary.

## Completion criteria

- All three hosts execute the same Runtime World and subsystem libraries.
- Live simulation values are isolated in a discardable layer.
- Reset and stop leave persistent authored layers unchanged.
- Plain CMake and OpenStrata build paths remain valid in supported
  environments.

The relevant contracts are documented in the
[host integration design](../design/hosts.md),
[runtime model](../design/runtime-model.md),
[USD integration](../design/usd-integration.md), and
[testing strategy](../design/testing.md).
