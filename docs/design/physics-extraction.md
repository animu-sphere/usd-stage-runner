# Physics Extraction Contract

Status: intended migration contract; extraction not implemented

The [current inventory](../architecture/physics-extraction-inventory.md)
freezes the Stage Runner source boundary at revision
`12324992c7ddd0b016ace780acadc5f07903390c`. This page defines how that boundary
is split between Stage Runner and `usd-physics-plugins` without widening the
physics API during the move.

## Ownership split

| Current responsibility | Destination |
| --- | --- |
| Handles, descriptors, validation, world lifecycle, commands, state, and optional queries from `physicsCore` | External `physicsCore`. Preserve behavior while replacing Stage Runner namespace, include paths, and math types. |
| `PhysicsRuntime` prim/body mapping, dirty synchronization, and fixed-step orchestration | Stage Runner's `stageRuntime`; do not pull `PrimId` or `RuntimeWorld` into the external core. |
| Jolt initialization, resources, layers, stepping, queries, and native-ID maps | External `physicsJolt`; all Jolt types and configuration remain private. |
| Runner physics schema interpretation and Stage-specific validation | Temporary Stage Runner compatibility importer. |
| Standard `UsdPhysics` interpretation and reusable scene/resource mappings | External `physicsUsd` after core/backend extraction; its order relative to Stage Runner package migration remains open below. |
| Character, Camera, and Vehicle policy | Stage Runner core libraries. They consume only narrow physics capabilities. |

The existing `physicsCore -> runtimeCore` edge is removed during extraction.
The receiving package owns neutral vector and transform values, while Stage
Runner performs explicit conversions at the composition boundary.

## Capability contracts

### Character minimum

Character requires no backend-native character object. The preserved minimum
is:

- a stable body handle;
- direct linear velocity and body transform state;
- a linear-velocity command;
- a ground query for a maximum distance; and
- support-body identity, contact normal, and contact distance.

The normal must support slope classification and projecting desired motion
onto a walkable surface. Grounded/rising/falling state, facing, and jump-edge
policy remain in `characterCore`.

### Camera minimum

`cameraCore` retains no physics dependency. `stageRuntime` adapts its callback
to:

- a finite world-space segment query;
- the closest normalized hit fraction; and
- an ignored body so the followed target does not obstruct its own camera.

Target `PrimId` to ignored `BodyHandle` conversion remains Stage Runner bridge
behavior.

### Vehicle hold point

The implemented Vehicle contract requires only chassis identity and
deterministic per-wheel commands. A later physics slice must prove the smallest
composable set drawn from:

- rigid chassis and wheel/body state, including angular velocity;
- force and torque application;
- constraints;
- contact or shape-cast queries with body, point, normal, distance, and surface
  friction; and
- suspension-force application.

These are validation requirements, not a frozen API. The migration must not
introduce a Jolt vehicle object or a four-wheel-only public abstraction.

## Installed package seam

The receiving workspace reserves these surfaces:

| Package | Imported target | Stage Runner consumer |
| --- | --- | --- |
| `physicsCore` | `physicsCore::physicsCore` | `characterCore`, `vehicleCore`, `stageRuntime`, and their tests. |
| `physicsJolt` | `physicsJolt::physicsJolt` | `stage_runner`, `usdviewStageRunner`, and Jolt-backed integration tests. |
| `physicsUsd` | `physicsUsd::physicsUsd` | Later standard physics import; it is not required for the initial core/backend extraction. |

Plain CMake consumes installed packages. OpenStrata uses the same
`physicsCore` and `physicsJolt` identities rather than a private sibling source
edge. Development, CI, packaging, the standalone host, and usdview Plugin View
must resolve the same graph.

The first Stage Runner consumer migration is bounded to:

- remove repository-local `libs/physicsCore` and `backends/physicsJolt` only
  after external package parity is proven;
- replace local subdirectories with package discovery and matching OpenStrata
  requirements;
- adapt includes, namespace, and math conversions to the accepted external
  contract;
- relocate `PhysicsRuntime` mapping and dirty synchronization into
  `stageRuntime`; and
- retain `StageSession::PhysicsWorldFactory` so standalone and usdview select
  the same neutral world construction seam.

## Authored compatibility seam

During the bounded compatibility period:

1. `RunnerPhysicsBodyAPI` and `RunnerColliderAPI` fixtures continue to work;
2. their importer stays in Stage Runner and produces external neutral
   descriptors;
3. no new `runner:physics:*` property is added;
4. `physicsUsd` later introduces the standard `UsdPhysics` path; and
5. Runner physics APIs are removed only after falling-body, Character, Camera,
   standalone, and usdview scenarios pass through the standard form.

The two repositories must settle whether installed core/backend consumer
migration precedes the first `physicsUsd` slice. Stage Runner's current roadmap
places `UsdPhysics` canonicalization before final consumer migration, while the
receiving repository's proposed roadmap uses Stage Runner package consumption
to validate the extracted core and backend before `physicsUsd`. Either order
must keep the compatibility importer local and must preserve the same parity
gate; Phase A does not silently choose between them.

## Unresolved receiving-package decisions

The extraction does not pre-decide the receiving repository's open contracts:

- public namespace and include root;
- neutral math ownership;
- validation and backend error transport;
- stale and cross-world handle behavior;
- semantic collision categories and masks;
- changed-body extraction ordering across create, sleep, wake, teleport, and
  destroy; and
- delivery order between Stage Runner package migration and the first
  `physicsUsd` slice.

Stage Runner may add a temporary local adapter after those decisions are made;
it must not make its old namespace, numeric Jolt layers, or `runtimeCore` types
the permanent external surface.

## Parity gate

The external packages are not ready for Stage Runner migration until the
inventory's neutral, Jolt, Character, Camera, Stage-session, standalone,
usdview, plain-CMake, and OpenStrata evidence passes against installed
packages. Mapping and dirty-synchronization assertions remain in Stage Runner
when the neutral contract tests move.
