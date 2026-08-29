# Current Milestone: Runtime Skeleton

Status: 🚧 in progress

The immediate objective is to establish a buildable repository skeleton and run
a controlled update loop after opening a USD Stage. This is Phase 0 of the
canonical design and deliberately excludes input, physics, camera behavior, and
substantial OpenExec integration.

## Scope

- root `CMakeLists.txt` and `CMakePresets.json`;
- OpenStrata workspace and CI configuration;
- `libs/runtimeCore` with a frame clock, fixed-step accumulator, Runtime World,
  and minimal component registry;
- `apps/stage_runner` with Stage loading and a bounded main loop;
- unit tests for the clock and component registry;
- one minimal Stage fixture; and
- CI that configures, builds, and tests the skeleton.

## Required boundaries

- `runtimeCore` does not depend on OpenUSD or OpenExec.
- Stage loading stays in the host or an adapter layer.
- Clock tests use injected time rather than wall-clock sleeps.
- The main loop can terminate deterministically for tests.

## Completion criteria

```text
stage_runner opens a minimal USD Stage
    -> builds a Runtime World
    -> advances update frames at an approximately 60 Hz target
    -> exits cleanly under a test-controlled condition
```

The build must pass through direct CMake and the initial OpenStrata workflow.
After completion, update [architecture/overview.md](../architecture/overview.md)
with the real targets and dependencies before advancing the next slice.
