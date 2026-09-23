# Architecture Overview

## Current state

The repository implements the input-to-physics-to-USD vertical slice and its
formal authored-data contract, the complete character-control slice, and the
camera-rig slice with collision avoidance. The reusable physics contract and
Jolt adapter are installed packages from `usd-physics-plugins`; this repository
owns gameplay policy, Stage import, prim/body synchronization, and host
composition. It contains a reusable Stage play session,
keyboard/gamepad/injected action mapping, runnable walking, grounding, and
jumping scenarios, plus camera targeting, mode, collision probes, smoothing,
fixed-step host evaluation, and incremental USD Camera pose synchronization.
Runtime, input, character, camera, and initial vehicle core libraries, an SDL
adapter, a codeless OpenUSD runtime-schema plugin, thin standalone and usdview
host adapters, an OpenStrata Plugin View deployment path, tests, and dual
CMake/OpenStrata build configuration are present. `vehicleCore` currently maps
normalized vehicle intent into deterministic per-wheel steering, drive,
service-brake, and handbrake commands for arbitrary wheel layouts. Vehicle
physics application, USD declarations, Stage integration, OpenExec, and
behavior targets do not exist yet.

## Implemented targets

| Target | Path | Responsibility | Dependencies |
| --- | --- | --- | --- |
| `runtimeCore` | `libs/runtimeCore` | Frame timing, bounded fixed stepping, host-facing play-session lifecycle, prim identity, runtime components, runtime transforms, dirty transform queue, and Runtime World lifetime. | C++ standard library only. |
| `inputCore` | `libs/inputCore` | Named action state, movement intent, and deterministic movement integration. | `runtimeCore`. |
| `physicsCore::physicsCore` | installed `usd-physics-plugins` package | Typed resource handles; box, body, and fixed-constraint descriptors; force, velocity, fixed-step, state-query, changed-body extraction, ground-query, and segment-query contracts. | C++ standard library only. |
| `characterCore` | `libs/characterCore` | Character intent, controller configuration and live state, walkable-ground and slope evaluation, desired velocity, facing, jump-edge handling, and rising/falling transitions. | `runtimeCore`, `physicsCore`. |
| `cameraCore` | `libs/cameraCore` | Prim-indexed target and optional anchor resolution; free, first-person, third-person, and orbit poses; optional collision-probe callbacks and clearance; configuration validation; live desired/current pose state; deterministic exponential smoothing; and dirty camera pose updates. | `runtimeCore`. |
| `vehicleCore` | `libs/vehicleCore` | Normalized vehicle intent, chassis and wheel composition, independent steering/powertrain/brake configuration, validation, and deterministic per-wheel steering and torque command distribution without a four-wheel-only contract. | `runtimeCore`, `physicsCore`. |
| `inputSdl` | `backends/inputSdl` | Map WASD, arrow keys, and the first gamepad's left stick to `move.x` and `move.y`; map Space and the gamepad south button to `jump`; own SDL window, controller, and subsystem lifetime. | `inputCore`; SDL3 or SDL2 when available. |
| `physicsJolt::physicsJolt` | installed `usd-physics-plugins` package | Own Jolt initialization and shutdown, shapes, bodies, constraints, semantic collision filtering, fixed stepping, changed-body extraction, ground shape casts, and first-hit segment ray casts behind `physicsCore`. | `physicsCore`; Jolt when the package is backend-enabled. |
| `runnerSchema` | `plugins/runnerSchema` | Register the codeless single-apply `RunnerPhysicsBodyAPI`, `RunnerColliderAPI`, `RunnerCharacterAPI`, and `RunnerCameraRigAPI` authored-data contracts. | OpenUSD resource-plugin discovery; no C++ ABI. |
| `stageRuntime` | `libs/stageRuntime` | Import an open Stage into a Runtime World, own the prim/body bridge, create physics through an injected factory, import character and camera systems, drive the shared play-session lifecycle, rebuild initial state on reset, and synchronize dirty translations and camera orientations. | `runtimeCore`, `inputCore`, external `physicsCore`, `characterCore`, `cameraCore`; OpenUSD `usd` and `usdGeom`. |
| `stage_runner` | `apps/stage_runner` | Parse host options, register schemas, open a Stage, select SDL and external Jolt adapters, poll or inject input, drive `StageSession`, and report results. | `stageRuntime`, `inputSdl`, external `physicsJolt`; OpenUSD `plug` when available. |
| `usdviewStageRunner` | `plugins/usdviewStageRunner` | Register Runner schemas, bind usdview's current Stage to `StageSession`, expose play/pause/stop/single-step/reset commands, drive elapsed host time, refresh the viewport, and dispose the session when the Stage changes. | `stageRuntime`, external `physicsJolt`, OpenUSD Python bindings, and usdview Qt APIs. |

The OpenStrata `plugin-view` build intent stages `usdviewStageRunner` into the
`runnerSchema` bundle's conventional `python/` root. An `Includes` entry in the
schema `plugInfo.json` registers the Python `PluginContainer`; `ost plugin view`
then activates the bundle paths and launches usdview. This is deployment
composition only—the native module and Python controller are the same files
used by the ordinary usdview path.

The CTest suite covers clocks, play/pause/stop/single-step/reset lifecycle,
registry and dirty-queue behavior, action and movement logic, the Stage-owned
prim/body bridge and changed-transform synchronization, isolated character
controller and camera
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

The source-revision-specific public surface, consumers, and deterministic
evidence are frozen in the
[physics extraction inventory](physics-extraction-inventory.md).

The installed `physicsCore` package defines distinct `ShapeHandle`, `BodyHandle`, and
`ConstraintHandle` types so backend resources cannot be accidentally mixed.
Descriptors currently cover box half extents, static or dynamic bodies, mass,
semantic collision filters, initial transforms, and fixed constraints. Shared validation
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

`SegmentQuery` is a second optional capability. It reports the first tracked
body and normalized hit fraction along a finite world-space segment, with an
optional ignored body handle. The Jolt implementation uses a narrow-phase ray
cast and translates its result back to backend-neutral handles.

Stage Runner's `stageRuntime::PhysicsRuntime` owns the one-to-one mapping
between runtime prims and backend
bodies. Its fixed-step boundary drains changed body states, updates only mapped
`RuntimeTransform` values that actually changed, and marks only those prims
dirty for USD synchronization. Missing or removed prims are safely discarded
from extraction, and binding never exposes a backend-specific type.

The installed `physicsJolt` package implements that contract behind a factory
boundary: its public header exposes only `physicsCore` and standard-library
types. Semantic category/mask filters replace knowledge of backend collision
layer numbers in Stage Runner. The adapter owns Jolt's process-wide type
registration while worlds exist and drains changed dynamic body state after
fixed steps. Jolt update capacity failures are surfaced rather than silently
accepting dropped contacts. Its package tests probe an elevated body, drop a
cube onto a static floor, verify settled ground contact, and check explicit
resource cleanup. The package exports `physicsJolt_BACKEND_AVAILABLE`; setting
`USD_STAGE_RUNNER_REQUIRE_JOLT=ON` rejects a package built without backend
support at configure time.

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
layer text is unchanged. A separate OST-bundle smoke test resolves the Python
plugin through the schema `plugInfo.json`, imports the staged native binding,
advances one fixed step, and verifies the same root-layer invariant. Python
sources are also compiled in CTest. A full interactive usdview launch remains
conditional on a runtime with usdview, Qt, and a display.

## Build and verification

The root CMake tree builds seven repository-owned compiled libraries, the
codeless schema plugin, standalone host, optional usdview adapter, and CTest
suite. `physicsCore` and `physicsJolt` are required installed CMake packages;
they are not built from repository source. The usdview adapter is enabled only
when the selected OpenUSD SDK supplies Python targets. OpenUSD and SDL
discovery remain isolated to schema, Stage-integration, adapter, and host
directories.

OpenStrata owns the pinned `cy2026`/`usd` environment and composes immutable
external `physicsCore` and `physicsJolt` artifacts into affected members. The
profile supplies OpenUSD but not SDL or the Jolt SDK dependency. The migration
branch pins publicly available Windows artifact sources and has validated them
from a fresh cache; Linux artifacts and hosted Stage Runner evidence remain.
Backend-neutral deterministic tests do not require SDL or physical devices.

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
runnerSchema --------> OpenUSD resource-plugin registry

stage_runner --------> stageRuntime + inputSdl + external physicsJolt
usdviewStageRunner --> stageRuntime + external physicsJolt
                       + OpenUSD Python + usdview Qt

stageRuntime --------> runtimeCore + inputCore + characterCore + cameraCore
                       + external physicsCore + OpenUSD usd/usdGeom
characterCore -------> runtimeCore + external physicsCore
vehicleCore ---------> runtimeCore + external physicsCore
cameraCore ----------> runtimeCore
inputSdl ------------> inputCore + SDL2 or SDL3 (optional)

external physicsJolt -> external physicsCore + Jolt
```

The core targets include no OpenUSD, SDL, Jolt, or OpenExec headers. The
complete intended model and forbidden edges remain in the
[design specification](../design/spec.md). Delivery order is tracked in the
[roadmap](../roadmap/).
