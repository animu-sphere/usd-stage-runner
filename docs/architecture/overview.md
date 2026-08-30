# Architecture Overview

## Current state

The repository implements the input-and-transform vertical slice. It contains
backend-neutral runtime and input libraries, an SDL adapter, an OpenUSD-aware
host executable, core/adapter/integration tests, and dual CMake/OpenStrata build
configuration. Physics, OpenExec, camera, behavior, and vehicle targets do not
exist yet.

## Implemented targets

| Target | Path | Responsibility | Dependencies |
| --- | --- | --- | --- |
| `runtimeCore` | `libs/runtimeCore` | Frame timing, bounded fixed stepping, prim identity, runtime components, runtime transforms, dirty transform queue, and Runtime World lifetime. | C++ standard library only. |
| `inputCore` | `libs/inputCore` | Named action state, movement intent, and deterministic movement integration. | `runtimeCore`. |
| `inputSdl` | `backends/inputSdl` | Map WASD, arrow keys, and the first gamepad's left stick to `move.x` and `move.y`; own SDL window, controller, and subsystem lifetime. | `inputCore`; SDL3 or SDL2 when available. |
| `stage_runner` | `apps/stage_runner` | Parse host options, open an OpenUSD Stage, build runtime transforms, poll input, advance movement, and synchronize dirty translations to USD. | `runtimeCore`, `inputCore`, `inputSdl`; OpenUSD `usd` and `usdGeom` when available. |

The CTest suite covers clocks, registry and dirty-queue behavior, action and
movement logic, physical-control mapping, host option validation, Stage loading,
and the complete injected-input-to-USD synchronization path.

## Runtime World and transforms

`RuntimeWorld` stores absolute USD prim paths as `PrimId` values and owns a
`ComponentRegistry`. A `RuntimeTransform` is an ordinary prim-indexed runtime
component; it does not introduce another entity or scene hierarchy.

Transform mutations are added to a duplicate-free dirty queue. Taking the queue
drains it, so the host writes only transforms changed since the previous
synchronization point. Removing a prim removes its components and any pending
dirty entry.

When a Stage opens, the host traverses its prims and imports local translate ops
for xformable prims. The current movement controller maps the two-dimensional
intent to X/Z translation at a fixed speed. After each host frame, the host sets
the USD translate op only for dirty runtime transforms. Stage writes remain
outside `runtimeCore` and `inputCore`.

## Input boundary

`ActionState` stores normalized values keyed by names. Missing actions read as
zero, non-finite values normalize to zero, and finite values clamp to `[-1, 1]`.
The first actions are `move.x` and `move.y`.

`SdlInputSource` implements the core `InputSource` interface through a PImpl, so
SDL types do not appear in public core APIs. SDL3 is preferred and SDL2 is used
when available. Without either SDK, the adapter target still builds in an
unavailable state so deterministic core and Stage tests remain usable. Setting
`USD_STAGE_RUNNER_REQUIRE_SDL=ON` converts a missing SDL package into a configure
error for interactive demo builds.

Deterministic host runs accept `--move-x` and `--move-y`. These values pass
through the adapter's backend-neutral physical-state mapper, allowing the same
normalization and controller path to be tested without a window or device.

## Frame execution

The implemented frame order is:

```text
poll SDL or inject physical axes
    -> normalize move.x and move.y
    -> update movement intent
    -> advance zero or more bounded fixed steps
    -> update and dirty the runtime transform
    -> synchronize dirty translations to USD
```

The default fixed interval is 1/60 second, the default frame bound is 300, and
catch-up is limited to eight fixed updates per frame. `--deterministic` supplies
exactly one fixed interval per host frame without sleeping.

## Build and verification

The root CMake tree builds the three libraries, host, and CTest suite. Each
library installs headers and an exported CMake package. OpenUSD and SDL discovery
remain isolated to their adapter/host directories.

OpenStrata owns the pinned `cy2026`/`usd` environment. That profile supplies
OpenUSD but not SDL; interactive builds therefore need an SDL2 or SDL3 package
on `CMAKE_PREFIX_PATH`. Dependency-free and deterministic tests do not require
SDL or physical devices.

The committed `tests/fixtures/minimal.usda` Stage contains `/World/Ground`,
`/World/PlayerCube`, and `/World/Camera`. The synchronization integration test
injects `move.x = 1` for four 1/60-second frames at speed 3, verifies four dirty
writes, and observes `/World/PlayerCube` move from X=0 to X=0.2.

## Dependency direction

The realized graph is:

```text
stage_runner -----> OpenUSD usd + usdGeom
      |  \
      |   `-------> inputSdl -----> SDL2 or SDL3 (optional at configure time)
      |                 |
      v                 v
 runtimeCore <----- inputCore
      ^                 ^
      |                 |
      `------ tests ----'
```

Neither `runtimeCore` nor `inputCore` includes OpenUSD or SDL. The complete
intended model and forbidden edges remain in the
[design specification](../design/spec.md). Delivery order is tracked in the
[roadmap](../roadmap/).
