# Architecture Overview

## Current state

The repository implements the input-to-physics-to-USD vertical slice and its
formal authored-data contract, the complete character-control slice, and the
camera-rig slice with collision avoidance. It contains the character controller,
Jolt ground and collision-query adapters, a reusable Stage play session,
keyboard/gamepad/injected action mapping, runnable walking, grounding, and
jumping scenario, plus camera targeting, mode, collision probes, smoothing,
fixed-step host evaluation, and incremental USD Camera pose synchronization.
Runtime, input, physics,
character, and camera core libraries, SDL and Jolt adapters, a codeless OpenUSD
runtime-schema plugin, thin standalone and usdview host adapters,
core/adapter/integration tests, and dual CMake/OpenStrata build configuration
are present. OST Plugin View integration, OpenExec, behavior, and vehicle
targets do not exist yet.

## Implemented targets

| Target | Path | Responsibility | Dependencies |
| --- | --- | --- | --- |
| `runtimeCore` | `libs/runtimeCore` | Frame timing, bounded fixed stepping, host-facing play-session lifecycle, prim identity, runtime components, runtime transforms, dirty transform queue, and Runtime World lifetime. | C++ standard library only. |
| `inputCore` | `libs/inputCore` | Named action state, movement intent, and deterministic movement integration. | `runtimeCore`. |
| `physicsCore` | `libs/physicsCore` | Typed resource handles; box, body, and fixed-constraint descriptors; force, velocity, fixed-step, state-query, changed-body extraction, character ground-query, and collision-segment query contracts; prim/body mapping and Runtime transform synchronization. | `runtimeCore`. |
| `characterCore` | `libs/characterCore` | Character intent, controller configuration and live state, walkable-ground and slope evaluation, desired velocity, facing, jump-edge handling, and rising/falling transitions. | `runtimeCore`, `physicsCore`. |
| `cameraCore` | `libs/cameraCore` | Prim-indexed target and optional anchor resolution; free, first-person, third-person, and orbit poses; optional collision-probe callbacks and clearance; configuration validation; live desired/current pose state; deterministic exponential smoothing; and dirty camera pose updates. | `runtimeCore`. |
| `inputSdl` | `backends/inputSdl` | Map WASD, arrow keys, and the first gamepad's left stick to `move.x` and `move.y`; map Space and the gamepad south button to `jump`; own SDL window, controller, and subsystem lifetime. | `inputCore`; SDL3 or SDL2 when available. |
| `physicsJolt` | `backends/physicsJolt` | Own Jolt initialization and shutdown, box shapes, static and dynamic bodies, fixed constraints, the initial moving/non-moving layers, fixed stepping, changed-body extraction, character ground shape casts, and first-hit segment ray casts behind `physicsCore`. | `physicsCore`; Jolt when available. |
| `runnerSchema` | `plugins/runnerSchema` | Register the codeless single-apply `RunnerPhysicsBodyAPI`, `RunnerColliderAPI`, `RunnerCharacterAPI`, and `RunnerCameraRigAPI` authored-data contracts. | OpenUSD resource-plugin discovery; no C++ ABI. |
| `stageRuntime` | `libs/stageRuntime` | Import an open Stage into a Runtime World, create physics through an injected factory, import character and camera systems, drive the shared play-session lifecycle, rebuild initial state on reset, and synchronize dirty translations and camera orientations. | `runtimeCore`, `inputCore`, `physicsCore`, `characterCore`, `cameraCore`; OpenUSD `usd` and `usdGeom`. |
| `stage_runner` | `apps/stage_runner` | Parse host options, register schemas, open a Stage, select SDL and Jolt adapters, poll or inject input, drive `StageSession`, and report results. | `stageRuntime`, `inputSdl`, `physicsJolt`; OpenUSD `plug` when available. |
| `usdviewStageRunner` | `plugins/usdviewStageRunner` | Register Runner schemas, bind usdview's current Stage to `StageSession`, expose play/pause/stop/single-step/reset commands, drive elapsed host time, refresh the viewport, and dispose the session when the Stage changes. | `stageRuntime`, `physicsJolt`, OpenUSD Python bindings, and usdview Qt APIs. |

The CTest suite covers clocks, play/pause/stop/single-step/reset lifecycle,
registry and dirty-queue behavior, action and movement logic, physics-core
resource, deterministic-step, prim/body mapping,
changed-transform synchronization, isolated character controller and camera
rig contracts, camera target following, collision adjustment, and smoothing,
physical-control mapping, Jolt ground and segment queries, host option validation,
Stage-session import, reset/rebuild and synchronization, Stage loading, and
complete injected movement and jump paths through character control to USD
synchronization, plus camera schema import, invalid-declaration rejection, and
the deterministic first-/third-person follow and obstructed-camera paths
through incremental USD translation and orientation writes.
`ost plugin test plugins/runnerSchema` additionally verifies schema
registration and authored-property flatten round-tripping.

`RunnerCharacterAPI` is valid only on a prim that also applies both physics APIs
and declares dynamic motion. Its ground-probe distance, maximum slope angle in
radians, and jump speed are read into a `CharacterControllerConfig`. The host
attaches the resulting controller to the same Runtime World prim and binds it
to the already imported body and Jolt ground-query capability. Character
attributes without their owning API and incomplete or static character
declarations are rejected. At each fixed step `StageSession` converts normalized
movement and jump actions into `CharacterIntent` and updates the imported
player controller before advancing physics.

`RunnerCameraRigAPI` is valid only on a `UsdGeomCamera`. Its target and optional
anchor relationships resolve to Runtime World prim identities with transforms;
non-free modes require exactly one target. Mode, offset, distance, pitch, yaw,
damping, collision enablement, and collision clearance are imported into a
prim-indexed `CameraRig`, and invalid values or
legacy properties without the API are rejected before the frame loop starts.
Camera declarations require a Y-up Stage. Camera, target, and anchor
translations affected by ancestor transforms are rejected unless the prim
resets its xform stack, keeping the current local `RuntimeTransform` identical
to its world translation until composed camera transforms are implemented. A
rig Camera accepts an empty transform stack or one translate op optionally
followed by the reserved double-precision runtime orient op, rejects other
authored transform ops, and cannot target or anchor itself.
After each fixed physics extraction, `StageSession` evaluates imported rigs with the
same controlled step. A changed position or forward direction marks that camera
dirty; synchronization updates its translate op and the dedicated
`xformOp:orient:runnerCamera` quaternion op. Unchanged cameras do not enter the
USD write path.
For collision-enabled third-person rigs, `StageSession` connects `cameraCore`'s
prim-aware callback to the optional `physicsCore` segment query and ignores the
followed target's bound body. A first hit shortens the desired pose by the
authored clearance before exponential smoothing, without exposing Jolt types
to `cameraCore`.

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

`GroundQuery` is a separate optional physics capability introduced by the
character slice. It reports a support body, contact normal, and distance
without adding a backend type to the public contract. `characterCore` combines
that query with `PhysicsWorld` state and velocity commands, so a deterministic
test double can exercise grounding, slope projection, facing, and edge-triggered
jumping. The Jolt world implements the capability with a downward shape cast
that excludes the queried body and translates the hit back to stable runtime
body handles.

`CollisionQuery` is a second optional capability. It reports the first tracked
body and normalized hit fraction along a finite world-space segment, with an
optional ignored body handle. The Jolt implementation uses a narrow-phase ray
cast and translates its result back to backend-neutral handles.

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
contacts. Its focused adapter test probes an elevated body, drops a cube onto a
static floor, verifies settled ground contact, and checks explicit resource
cleanup. When no Jolt CMake package is available,
a small unavailable implementation preserves backend-neutral builds; requiring
Jolt is an explicit configure option.

## Runtime World and transforms

`RuntimeWorld` stores absolute USD prim paths as `PrimId` values and owns a
`ComponentRegistry`. A `RuntimeTransform` is an ordinary prim-indexed runtime
component; it does not introduce another entity or scene hierarchy.

Transform mutations are added to a duplicate-free dirty queue. Taking the queue
drains it, so `StageSession` writes only transforms changed since the previous
synchronization point. Removing a prim removes its components and any pending
dirty entry.

When a Stage session starts, `stageRuntime` traverses its prims and imports local translate ops
for xformable prims. A physics prim applies both `RunnerPhysicsBodyAPI` and
`RunnerColliderAPI`. Motion type, mass, shape, and local-space box half extents
come from their declared `runner:physics:*` attributes; ordered scale ops
multiply the half extents. The importer currently accepts `static` or `dynamic`
motion and `box` shapes. It requires a Y-up meter Stage, one translate op
followed by scale ops, and identity ancestor transforms unless the prim resets
its transform stack. These restrictions keep local USD translation identical
to Jolt world position until composed transform support lands. The importer
creates and binds backend bodies through `PhysicsRuntime`; a physics-declaring
Stage is rejected when Jolt is unavailable. After each host frame, `StageSession`
sets the USD translate op only for dirty runtime transforms. Dirty camera rigs
also write their runtime orientation through a dedicated orient op. These live
values are authored in a scoped edit context to an owned anonymous layer placed
first among the existing Stage session layer's sublayers; the host edit target,
root layer, and unrelated session sublayers remain unchanged. Stage writes
remain in `stageRuntime` and outside the backend-neutral core libraries. Authored
body or collider attributes without their
owning API schema are rejected instead of being silently interpreted through
the removed temporary convention.

## Input boundary

`ActionState` stores normalized values keyed by names. Missing actions read as
zero, non-finite values normalize to zero, and finite values clamp to `[-1, 1]`.
The first actions are `move.x`, `move.y`, and `jump`.

`SdlInputSource` implements the core `InputSource` interface through a PImpl, so
SDL types do not appear in public core APIs. SDL3 is preferred and SDL2 is used
when available. Without either SDK, the adapter target still builds in an
unavailable state so deterministic core and Stage tests remain usable. Setting
`USD_STAGE_RUNNER_REQUIRE_SDL=ON` converts a missing SDL package into a configure
error for interactive demo builds.

Deterministic host runs accept `--move-x`, `--move-y`, and `--jump`. These
values pass through the adapter's backend-neutral physical-state mapper,
allowing the same normalization and controller path to be tested without a
window or device.

## Frame execution

The implemented physics frame order is coordinated through the host-neutral
`PlaySession` boundary:

```text
poll SDL or inject physical axes and jump button
    -> normalize move.x, move.y, and jump
    -> update movement and character intent
    -> advance zero or more bounded fixed steps
        -> update the character controller
        -> set desired planar or jump body velocity
        -> step Jolt
        -> extract changed body transforms
        -> update and dirty runtime transforms
        -> probe collision-enabled third-person rigs
        -> evaluate camera rigs and dirty changed poses
    -> synchronize dirty translations and camera orientations to USD
```

`PlaySession` owns play and pause state plus the bounded accumulator, while
`StageSession` supplies its rebuild, fixed-update, and synchronization
callbacks. A playing
host frame advances zero or more fixed steps and then invokes one synchronization
callback. While paused, host time is ignored. Single-step clears any partial
remainder, advances exactly one fixed interval, synchronizes once, and remains
paused. Reset also clears the remainder, invokes the host-supplied state rebuild
callback, synchronizes the restored state, and remains paused. Stop pauses and
clears the remainder without invoking lifecycle callbacks, leaving the host to
discard its state. `StageSession`
captures initial transforms, reconstructs the Runtime World and imported
systems on reset, and restores those values through the same dirty write path.
Before reset it clears the runtime layer, so prior simulation opinions are
discarded. Stop clears the layer and rebuilds from persistent composed state;
destruction detaches only the sublayer owned by that session.
The standalone host only supplies time, actions, and the selected physics
factory. The usdview host supplies its current Stage, elapsed Qt timer duration,
lifecycle commands, and viewport redraw requests through the same boundary.

The default fixed interval is 1/60 second, the default frame bound is 300, and
catch-up is limited to eight fixed updates per frame. `--deterministic` supplies
exactly one fixed interval per host frame without sleeping.

## usdview adapter

`usdviewStageRunner` consists of a small OpenUSD Boost.Python module and a
Python `PluginContainer`. The native module accepts usdview's existing
`Usd.Stage`, constructs the same `StageSession` and physics-world factory used
by the standalone host, and exposes only lifecycle, action, timing, and stats
operations. The Python controller owns a 16 ms Qt host timer, measures elapsed
frame time, requests viewport redraws, and releases the old session on
`signalStageReplaced`. Its Stop and Reset commands therefore retain the shared
discardable-layer semantics.

The plugin package carries the codeless Runner schema resources and registers
them at import time. A native binding smoke test opens the shared minimal Stage,
advances play and single-step paths, stops the session, and confirms the root
layer text is unchanged. Python sources are also compiled in CTest. A full
interactive usdview launch remains conditional on a runtime with usdview, Qt,
and a display.

## Build and verification

The root CMake tree builds the eight compiled libraries, codeless schema plugin,
standalone host, optional usdview adapter, and CTest suite. Each library
installs headers and an exported CMake package. The usdview adapter is enabled
only when the selected OpenUSD SDK supplies Python targets. OpenUSD, SDL, and
Jolt discovery remain isolated to schema, Stage-integration, adapter, and host
directories.

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
one host path. `character_import.usda` adds `RunnerCharacterAPI` to a dynamic
body and verifies that the host constructs one runtime controller.
`character_walk.usda` adds a floor and grounded character; deterministic tests
verify walking, grounding, an edge-triggered jump, physics updates, and dirty
USD synchronization through the complete host path. `third_person_camera.usda`
moves a non-physics target through the same controlled clock, evaluates first-
and third-person rigs, places a static obstruction behind the target, and
verifies that the third-person pose stops at its authored clearance while both
moving poses use incremental USD writes. The multi-frame
`camera_import.usda` test verifies that an unchanged rig writes only once.

## Dependency direction

The realized graph is:

```text
runnerSchema -----> OpenUSD resource-plugin registry
      ^
      |
stage_runner -----> OpenUSD plug + usd + usdGeom
      |  |  |  \
      |  |  |   `----> inputSdl -----> SDL2 or SDL3 (optional at configure time)
      |  |  `--------> inputCore
      |  `-----------> characterCore
      v
 runtimeCore
      ^  ^
      |  `---------------- cameraCore
      |
 physicsCore <----- characterCore
      ^
      |
 physicsJolt <----- stage_runner
      |
      `------------> Jolt (optional at configure time)

usdviewStageRunner -> stageRuntime + physicsJolt + OpenUSD Python + usdview Qt
```

The core targets include no OpenUSD, SDL, Jolt, or OpenExec headers. The
complete intended model and forbidden edges remain in the
[design specification](../design/spec.md). Delivery order is tracked in the
[roadmap](../roadmap/).
