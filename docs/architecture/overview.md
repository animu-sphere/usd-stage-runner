# Architecture Overview

## Current state

The repository implements the Phase 0 runtime skeleton. It contains one
backend-neutral library, one OpenUSD-aware host executable, unit and integration
tests, and dual CMake/OpenStrata build configuration. Input, physics, OpenExec,
camera, behavior, and vehicle targets do not exist yet.

## Implemented targets

| Target | Path | Responsibility | Dependencies |
| --- | --- | --- | --- |
| `runtimeCore` | `libs/runtimeCore` | Frame timing, bounded fixed stepping, prim identity, runtime components, and Runtime World lifetime. | C++ standard library only. |
| `stage_runner` | `apps/stage_runner` | Parse host options, open an OpenUSD Stage, derive a Runtime World from traversed prim paths, and run a bounded update loop. | `runtimeCore`; OpenUSD `usd` when available. |
| `runtime_clock_tests` | `libs/runtimeCore/tests` | Verify injected clock time, negative-time protection, accumulation, and bounded catch-up. | `runtimeCore`. |
| `runtime_registry_tests` | `libs/runtimeCore/tests` | Verify typed components, move-only values, prim ownership, replacement, and cleanup. | `runtimeCore`. |

`runtimeCore` does not include or link OpenUSD. OpenUSD discovery is isolated to
the `stage_runner` CMake directory. A plain CMake build can disable the host with
`USD_STAGE_RUNNER_BUILD_APP=OFF`; if the host is enabled without OpenUSD it
builds a diagnostic-only executable. Setting
`USD_STAGE_RUNNER_REQUIRE_OPENUSD=ON` makes the missing dependency a configure
error instead.

## Runtime World

`RuntimeWorld` stores absolute USD prim paths as `PrimId` values and owns a
`ComponentRegistry`. The registry stores at most one component of each C++ type
per prim, supports move-only components, and removes all attached components
when a prim leaves the world. The first host build derives these identities by
traversing the opened Stage; it does not yet scan API schemas or create domain
components.

There is no independently parented entity hierarchy. The prim path remains the
runtime identity, matching the design invariant.

## Frame execution

The implemented host loop is the Phase 0 subset of the intended frame order:

```text
open Stage
    -> traverse prims into Runtime World
    -> sample variable frame time
    -> accumulate zero or more fixed updates
    -> stop at the configured frame bound
```

The default fixed interval is 1/60 second, the default frame bound is 300, and
catch-up is limited to eight fixed updates per frame. Excess whole steps are
dropped while the fractional remainder is retained. `--deterministic` supplies
exactly one fixed interval per host frame without sleeping, which is the path
used by the Stage integration test.

Polling, input actions, physics, USD transform synchronization, cameras, and
rendering remain unimplemented.

## Build and verification

The root CMake tree builds the library, host, and CTest suite. `runtimeCore`
installs headers and an exported `runtimeCore::runtimeCore` CMake package.

OpenStrata owns the pinned `cy2026`/`usd` environment. `openstrata.toml`
declares the library and tool workspace members, and `openstrata.ci.yaml` is the
source for the generated Windows pull-request workflow. OpenStrata 0.22.x emits
a plugin-only graph command for every workspace cell, so the generated workflow
temporarily guards that step until the repository contains a plugin member. CI
also contains a plain CMake job that builds the backend-neutral core on Windows
and Linux.

The committed `tests/fixtures/minimal.usda` Stage contains `/World/Ground`,
`/World/PlayerCube`, and `/World/Camera`. When OpenUSD is available, CTest opens
that Stage, derives four prim identities including `/World`, advances four
deterministic frames, and verifies four fixed updates with no dropped steps.

## Dependency direction

The currently realized graph is:

```text
stage_runner -----> OpenUSD usd
      |
      v
 runtimeCore -----> C++ standard library
      ^
      |
 unit tests
```

The complete intended model and later forbidden edges remain in the
[design specification](../design/spec.md). Delivery order is tracked in the
[roadmap](../roadmap/).
