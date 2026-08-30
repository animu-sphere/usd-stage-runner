# Architecture Overview

## Current state

The repository implements the input-to-physics-to-USD vertical slice and its
formal authored-data contract. It contains runtime, input, and physics core
libraries, SDL and Jolt adapters, a codeless OpenUSD physics-schema plugin, an
OpenUSD-aware host executable, core/adapter/integration tests, and dual
CMake/OpenStrata build configuration. Character, OpenExec, camera, behavior,
and vehicle targets do not exist yet.

## Implemented targets

| Target | Path | Responsibility | Dependencies |
| --- | --- | --- | --- |
| `runtimeCore` | `libs/runtimeCore` | Frame timing, bounded fixed stepping, prim identity, runtime components, runtime transforms, dirty transform queue, and Runtime World lifetime. | C++ standard library only. |
| `inputCore` | `libs/inputCore` | Named action state, movement intent, and deterministic movement integration. | `runtimeCore`. |
| `physicsCore` | `libs/physicsCore` | Typed resource handles; box, body, and fixed-constraint descriptors; force, velocity, fixed-step, state-query, and changed-body extraction contracts; prim/body mapping and Runtime transform synchronization. | `runtimeCore`. |
| `inputSdl` | `backends/inputSdl` | Map WASD, arrow keys, and the first gamepad's left stick to `move.x` and `move.y`; own SDL window, controller, and subsystem lifetime. | `inputCore`; SDL3 or SDL2 when available. |
| `physicsJolt` | `backends/physicsJolt` | Own Jolt initialization and shutdown, box shapes, static and dynamic bodies, fixed constraints, the initial moving/non-moving layers, fixed stepping, and changed-body extraction behind `physicsCore`. | `physicsCore`; Jolt when available. |
| `runnerSchema` | `plugins/runnerSchema` | Register the codeless single-apply `RunnerPhysicsBodyAPI` and `RunnerColliderAPI` authored-data contracts. | OpenUSD resource-plugin discovery; no C++ ABI. |
| `stage_runner` | `apps/stage_runner` | Parse host options, open an OpenUSD Stage, import applied physics schemas, poll input, set desired body motion, step physics, and synchronize dirty translations to USD. | `runtimeCore`, `inputCore`, `physicsCore`, `inputSdl`, `physicsJolt`; OpenUSD `plug`, `usd`, and `usdGeom` when available. |

The CTest suite covers clocks, registry and dirty-queue behavior, action and
movement logic, physics-core resource, deterministic-step, prim/body mapping,
and changed-transform synchronization contracts, physical-control mapping,
host option validation, Stage loading, and the complete injected-input-to-USD
synchronization path. `ost plugin test plugins/runnerSchema` additionally
verifies schema registration and authored-attribute flatten round-tripping.

## Physics boundary

`physicsCore` defines distinct `ShapeHandle`, `BodyHandle`, and
`ConstraintHandle` types so backend resources cannot be accidentally mixed.
Descriptors currently cover box half extents, static or dynamic bodies, mass,
collision layers, initial transforms, and fixed constraints. Shared validation
rejects invalid dimensions, transforms, masses, handles, forces, velocities,
and timesteps before they reach an SDK adapter.

`PhysicsWorld` owns the backend-neutral lifetime and command boundary. A
backend creates and destroys shapes, bodies, and constraints; accepts force and
velocity commands; advances a positive fixed duration; and returns a draining
list of changed `BodyState` values. `PhysicsBody` is a small handle component
that can be attached to a prim in `RuntimeWorld`. The contract test uses a
deterministic mock with gravity to verify this boundary without Jolt.

`PhysicsRuntime` owns the one-to-one mapping between runtime prims and backend
bodies. Its fixed-step boundary drains changed body states, updates only mapped
`RuntimeTransform` values that actually changed, and marks only those prims
dirty for USD synchronization. Missing or removed prims are safely discarded
from extraction, and binding never exposes a backend-specific type.

`physicsJolt` implements that contract behind a factory boundary: its public
header exposes only `physicsCore` and standard-library types. The adapter maps
static bodies to collision layer 0 (`nonMovingCollisionLayer`) and dynamic
bodies to collision layer 1 (`movingCollisionLayer`), rejecting mismatched
descriptors. It owns Jolt's process-wide type registration while adapter worlds
exist and drains changed dynamic body state after fixed steps. Jolt update
capacity failures are surfaced rather than silently accepting dropped
contacts. Its focused adapter test drops a cube onto a static floor and verifies
settling and explicit resource cleanup. When no Jolt CMake package is available,
a small unavailable implementation preserves backend-neutral builds; requiring
Jolt is an explicit configure option.

## Runtime World and transforms

`RuntimeWorld` stores absolute USD prim paths as `PrimId` values and owns a
`ComponentRegistry`. A `RuntimeTransform` is an ordinary prim-indexed runtime
component; it does not introduce another entity or scene hierarchy.

Transform mutations are added to a duplicate-free dirty queue. Taking the queue
drains it, so the host writes only transforms changed since the previous
synchronization point. Removing a prim removes its components and any pending
dirty entry.

When a Stage opens, the host traverses its prims and imports local translate ops
for xformable prims. A physics prim applies both `RunnerPhysicsBodyAPI` and
`RunnerColliderAPI`. Motion type, mass, shape, and local-space box half extents
come from their declared `runner:physics:*` attributes; ordered scale ops
multiply the half extents. The importer currently accepts `static` or `dynamic`
motion and `box` shapes. It requires a Y-up meter Stage, one translate op
followed by scale ops, and identity ancestor transforms unless the prim resets
its transform stack. These restrictions keep local USD translation identical
to Jolt world position until composed transform support lands. The importer
creates and binds backend bodies through `PhysicsRuntime`; a physics-declaring
Stage is rejected when Jolt is unavailable. After each host frame, the host
sets the USD translate op only for dirty runtime transforms. Stage writes remain
outside the core libraries. Authored body or collider attributes without their
owning API schema are rejected instead of being silently interpreted through
the removed temporary convention.

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

The implemented physics frame order is:

```text
poll SDL or inject physical axes
    -> normalize move.x and move.y
    -> update movement intent
    -> advance zero or more bounded fixed steps
        -> set desired horizontal body velocity
        -> step Jolt
        -> extract changed body transforms
        -> update and dirty runtime transforms
    -> synchronize dirty translations to USD
```

The default fixed interval is 1/60 second, the default frame bound is 300, and
catch-up is limited to eight fixed updates per frame. `--deterministic` supplies
exactly one fixed interval per host frame without sleeping.

## Build and verification

The root CMake tree builds the five compiled libraries, codeless schema plugin,
host, and CTest suite. Each library installs headers and an exported CMake
package. OpenUSD, SDL, and Jolt discovery remain isolated to schema,
adapter, and host directories.

OpenStrata owns the pinned `cy2026`/`usd` environment. That profile supplies
OpenUSD but not SDL or Jolt; interactive or Jolt-backed builds therefore need
those packages on `CMAKE_PREFIX_PATH`. Backend-neutral deterministic tests do
not require either SDK or physical devices.

The committed `tests/fixtures/minimal.usda` Stage contains `/World/Ground`,
`/World/PlayerCube`, and `/World/Camera`. The synchronization integration test
injects `move.x = 1` for four 1/60-second frames at speed 3, verifies four dirty
writes, and observes `/World/PlayerCube` move from X=0 to X=0.2. A second Stage
fixture verifies the same path with a float-precision translate op.
`falling_cube.usda` declares a static floor and dynamic player Cube exclusively
through the two applied physics APIs. When OpenUSD and Jolt are both available,
its integration test verifies imported body counts, changed-body extraction,
dirty USD writes, gravity, collision, and injected horizontal movement through
one host path.

## Dependency direction

The realized graph is:

```text
runnerSchema -----> OpenUSD resource-plugin registry
      ^
      |
stage_runner -----> OpenUSD plug + usd + usdGeom
      |  \
      |   `-------> inputSdl -----> SDL2 or SDL3 (optional at configure time)
      |                 |
      v                 v
 runtimeCore <----- inputCore
      ^
      |
 physicsCore <----- physics contract tests
      ^                 ^
      |                 |
 physicsJolt <----- stage_runner
      |
      `------------> Jolt (optional at configure time)
```

The core targets include no OpenUSD, SDL, Jolt, or OpenExec headers. The
complete intended model and forbidden edges remain in the
[design specification](../design/spec.md). Delivery order is tracked in the
[roadmap](../roadmap/).
