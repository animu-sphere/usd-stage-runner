# Physics Extraction Inventory

> Historical baseline: this inventory records the pre-extraction source tree.
> See [Architecture](overview.md) for the current external-package topology.

This page freezes the implemented physics boundary that will be handed to
`usd-physics-plugins`. It describes the tree at source revision
`12324992c7ddd0b016ace780acadc5f07903390c`; later implementation changes must
update this inventory or identify the newer extraction revision.

The target repository owns the future package contracts. This page owns only
the current Stage Runner implementation, its consumers, and its evidence. The
[extraction contract](../design/physics-extraction.md) owns the planned split
and migration seam.

## Classification

The current implementation separates into four ownership classes.

| Class | Current source | Current responsibility |
| --- | --- | --- |
| Neutral physics behavior | `libs/physicsCore`, except `PhysicsRuntime` and Stage Runner value types | Define and validate handles, descriptors, lifecycle, commands, state, and optional queries. |
| Stage Runner bridge behavior | `PhysicsRuntime` and physics orchestration in `stageRuntime` | Map prim identity to physics handles, schedule steps, and write changed transforms into `RuntimeWorld`. |
| Backend implementation | `backends/physicsJolt` | Own every Jolt type, allocator, job-system object, object layer, and native resource identifier. |
| Authored-data compatibility | physics declarations in `runnerSchema` and their importer in `stageRuntime` | Interpret the implemented Runner schemas and enforce the current Stage restrictions. |

The current `physicsCore -> runtimeCore` dependency supplies
`runtime::Vec3d` and `runtime::RuntimeTransform` to descriptors, state,
commands, and queries. `runtime::PrimId` does not appear in the solver API; it
enters through `PhysicsRuntime`.

## Public `physicsCore` surface

All current public headers are under
`libs/physicsCore/include/usd_stage_runner/physics/`.

| Header | Frozen behavior | Current coupling |
| --- | --- | --- |
| `handles.h` | Typed 64-bit `ShapeHandle`, `BodyHandle`, and `ConstraintHandle`; zero is invalid; equality and hashing are available without exposing backend identifiers. | Standard library only. |
| `physics_shape.h` | `ShapeType::box`, positive finite half extents, `ShapeDescriptor`, and the lightweight `PhysicsShape` handle component. | `runtime::Vec3d`. |
| `physics_body.h` | Static and dynamic motion, collision-layer value, initial transform, positive dynamic mass, body state with transform and linear velocity, and the `PhysicsBody` handle component. | `runtime::RuntimeTransform` and `runtime::Vec3d`. |
| `physics_constraint.h` | Fixed constraints between two distinct valid body handles and the `PhysicsConstraint` handle component. | Handles only. |
| `physics_world.h` | Explicit create/destroy for shapes, bodies, and constraints; force and linear-velocity commands; direct body state; caller-supplied positive fixed step; draining changed-body extraction. | Descriptor and Stage Runner vector types. |
| `ground_query.h` | Optional query returning support body, finite non-zero normal, and non-negative distance for a body and maximum probe distance. | `runtime::Vec3d`. |
| `collision_query.h` | Optional first-hit query over a non-zero world-space segment, normalized hit fraction in `[0, 1]`, and one optional ignored body. | `runtime::Vec3d`. |
| `physics_runtime.h` | One-to-one `PrimId <-> BodyHandle` mapping, fixed-step delegation, changed-state extraction, `RuntimeTransform` updates, and dirty-prim enqueueing. | `runtime::RuntimeWorld`, `runtime::PrimId`, and the dirty queue. |

The contract is single-owner and called from one scheduling context. It does
not own a wall clock, fixed-step accumulation, a host loop, a USD Stage, or
runtime-to-USD synchronization.

## `physicsJolt` surface and private state

The installed public header currently exposes only:

- `isJoltPhysicsAvailable()`;
- `createJoltPhysicsWorld()` returning `std::unique_ptr<PhysicsWorld>`; and
- numeric moving and non-moving collision-layer constants.

The numeric layer constants are adapter policy and are passed through
`StageSessionConfig`; they are not authored schema values.

`jolt_physics_world.cpp` privately owns Jolt type registration, allocator and
job-system lifetime, object and broad-phase layer filters, native shapes,
bodies and fixed constraints, handle/native-ID maps, stepping, changed-body
collection, ground shape casts, and segment ray casts. Its implemented
observable behavior is:

- box, static/dynamic body, and fixed-constraint creation;
- explicit dependent-resource cleanup;
- force and linear-velocity commands on dynamic bodies;
- fixed stepping and draining changed-body state;
- closest-hit segment queries with an ignored body;
- walkable-support candidates returned as ground contacts; and
- explicit errors for invalid descriptors, unknown handles, unavailable Jolt,
  resource exhaustion, or failed Jolt updates.

No Jolt header or type appears in the public `physicsCore`, Character, Camera,
Vehicle, or `stageRuntime` APIs.

## Current consumers

| Consumer | Physics contract used today | Responsibility that stays in Stage Runner |
| --- | --- | --- |
| `characterCore` | `BodyHandle`, `PhysicsWorld::bodyState`, `setLinearVelocity`, and `GroundQuery::groundContact`. | Grounded/rising/falling state, slope acceptance, planar desired velocity, facing, and jump-edge policy. Character import requires the selected world to provide `GroundQuery`. |
| `cameraCore` | No physics header or handle. It accepts a prim-aware collision callback returning an optional normalized hit fraction. | Target/anchor resolution, camera modes, framing, smoothing, and collision-clearance policy. `stageRuntime` adapts this callback to `CollisionQuery` and ignores the target's mapped body. |
| `vehicleCore` | `BodyHandle` identifies the chassis. The current controller emits per-wheel steering, drive-torque, and brake-torque commands but does not apply them to physics. | Intent normalization and arbitrary-wheel role composition remain here. No Jolt vehicle type is required or admitted. |
| `stageRuntime` | `PhysicsWorld`, optional query capabilities, `PhysicsRuntime`, descriptors, body state, velocity commands, stepping, and changed-state extraction. | Stage traversal, compatibility import, Runtime World construction, fixed-step order, lifecycle/reset, prim/body mapping, and incremental runtime-layer writeback. |
| `stage_runner` | Jolt availability and world factory. | Select the concrete backend and pass a neutral factory to `StageSession`. |
| `usdviewStageRunner` | The same Jolt availability and world factory. | Reuse `StageSession`; no host-specific physics path. |
| `runnerSchema` | No runtime C++ dependency. It declares the current physics compatibility APIs. | Register gameplay schemas and retain compatibility resources while fixtures migrate. |

`StageSession` currently exposes the backend seam as
`std::function<std::unique_ptr<PhysicsWorld>()>`. A session constructs physics
only when the Stage declares bodies. The standalone and usdview adapters both
provide the same Jolt factory, so host behavior does not require separate
simulation APIs.

## Fixed-step and lifetime order

The current order that migration must preserve is:

```text
actions
  -> Character intent/controller or direct movement velocity
  -> PhysicsWorld::step
  -> changed body state into RuntimeWorld + dirty queue
  -> camera rig evaluation and optional collision segment query
  -> incremental USD runtime-layer synchronization
```

`StageSession` owns the physics world. Character components may retain pointers
to that world and its ground-query capability, so Runtime World components are
destroyed before the physics world. Reset and stop rebuild or discard transient
state without changing persistent authored layers.

## Authored compatibility inventory

`RunnerPhysicsBodyAPI` currently owns `runner:physics:motionType` and
`runner:physics:mass`. `RunnerColliderAPI` owns `runner:physics:shape` and
`runner:physics:halfExtents`. A physics prim must apply both APIs.

`stageRuntime` interprets those declarations into a box shape and static or
dynamic body. It also enforces the current Y-up, meter-scale, transform-stack,
and identity-ancestor restrictions. This importer is application compatibility
code, not part of Jolt. No standard `UsdPhysics` importer exists in this tree.

Physics-positive fixtures are `falling_cube.usda`, `character_import.usda`,
`character_walk.usda`, `character_follow_camera.usda`, and
`third_person_camera.usda`. Focused negative fixtures cover legacy or
incomplete schema application, static Character bodies, non-meter stages, and
inherited physics transforms. The `runnerSchema` smoke fixture exercises
schema registration and authored-property flattening.

## Current build and composition

The root CMake project adds `libs/physicsCore` and `backends/physicsJolt` as
repository-local subdirectories. `physicsCore::physicsCore` publishes
`runtimeCore::runtimeCore`; `physicsJolt::physicsJolt` publishes `physicsCore`
and keeps the discovered `Jolt::Jolt` or `Jolt` target private.
`USD_STAGE_RUNNER_REQUIRE_JOLT` changes a missing backend from the buildable
unavailable implementation to a configure error.

`stageRuntime::stageRuntime`, `characterCore::characterCore`, and
`vehicleCore::vehicleCore` publish their current `physicsCore` dependency. The
standalone executable and usdview native module link `physicsJolt` directly so
each host can supply the same neutral world factory to `StageSession`.

OpenStrata currently mirrors that source-owned graph:

- `physicsCore` and `physicsJolt` are workspace and release members;
- their library manifests declare `physicsJolt -> physicsCore -> runtimeCore`;
- Character, Vehicle, Stage Runtime, and the standalone tool declare their
  direct physics library requirements;
- `plugin-view-jolt` sets `USD_STAGE_RUNNER_REQUIRE_JOLT` and stages the
  usdview adapter into the `runnerSchema` bundle; and
- `runnerSchema` provides both compatibility physics schemas alongside the
  Character and Camera schemas.

The plain-CMake physics CI and the OpenStrata source CI both check out the
Jolt 5.5.0 source at commit
`23dadd0e603f1b321142d4c74df07fce85064989`, build it separately, and configure
Stage Runner with Jolt required. The OpenStrata job also verifies the workspace
graph, builds and tests the full workspace, and confirms that the real
`stage_runner.physics_falling_cube` test is present.

## Verification inventory

| Boundary | Deterministic evidence |
| --- | --- |
| Neutral core | `physicsCore.contracts` covers typed handles, descriptor validation, lifecycle, force and velocity commands, direct and changed state, fixed stepping, draining extraction, prim/body mapping, and dirty synchronization with a mock world. |
| Jolt adapter | `physicsJolt.bootstrap` covers availability behavior, collision-layer rejection, closest segment hits, ignored bodies, ground distance and normal, falling/settling, draining changes, constraints, and cleanup. |
| Character capability | `characterCore.contracts`, `stage_runner.character_walk_and_ground`, and `stage_runner.character_jump` cover the narrow query/state/velocity path. |
| Camera capability | `cameraCore.rigs` covers callback semantics; `stage_runner.camera_collision_and_sync` covers the Jolt query adapter and incremental camera synchronization. |
| Vehicle hold point | `vehicleCore.contracts` covers normalized intent, independent wheel roles, torque distribution, passive wheels, and non-four-wheel layouts without a backend. |
| Stage/host composition | `stage_runtime.session`, `stage_runner.physics_falling_cube`, `stage_runner.physics_vertical_slice`, usdview native-session smoke tests, and OpenStrata Plugin View smoke tests cover shared lifecycle and discardable-layer behavior. |
| Authored compatibility | schema smoke and round-trip tests plus rejection fixtures cover complete API application, legacy properties, static Character rejection, units, and transform restrictions. |
