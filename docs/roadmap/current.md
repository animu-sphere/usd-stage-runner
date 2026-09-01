# Milestone 5: Host Integration

Status: in progress

The standalone `stage_runner` path now implements physics, character control,
camera following, collision avoidance, and incremental USD synchronization.
This milestone makes that same play session reusable from usdview and OST
Plugin View without duplicating simulation logic or authoring live values into
the Stage's persistent layers.

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

- Extract the implemented Stage import, fixed-step execution, reset, and
  synchronization path from standalone-only orchestration into a reusable
  host-facing boundary.
- Add play, pause, single-step, and reset semantics under a controlled clock.
- Author simulated values into a discardable session/runtime layer.
- Prove that reset or stop discards runtime values without changing the root
  layer.

### Host adapters

- Add a thin `plugins/usdviewStageRunner` adapter over the shared session.
- Connect the same session boundary to OST Plugin View.
- Reuse `character_walk.usda` and `third_person_camera.usda` to verify
  equivalent fixed-step and synchronization results in each host.

## Recommended PR sequence

The next PR introduces the reusable play-session boundary and deterministic
play/pause/single-step/reset coverage while keeping `stage_runner` behavior
unchanged. A following PR adds the discardable runtime layer, then the usdview
and OST Plugin View adapters consume that boundary.

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
