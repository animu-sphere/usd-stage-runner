# Physics Extraction Contract

Status: extraction and Stage Runner consumer migration implemented; hosted
Windows/Linux artifact evidence recorded

The [current inventory](../architecture/physics-extraction-inventory.md)
freezes the Stage Runner source boundary at revision
`12324992c7ddd0b016ace780acadc5f07903390c`. This page defines how that boundary
was split between Stage Runner and `usd-physics-plugins` without widening the
physics API during the move. The inventory remains the revision-specific
pre-extraction record; the current implementation consumes installed packages.

## Ownership split

| Current responsibility | Destination |
| --- | --- |
| Handles, descriptors, validation, world lifecycle, commands, state, and optional queries from `physicsCore` | External `physicsCore`. Preserve behavior while replacing Stage Runner namespace, include paths, and math types. |
| `PhysicsRuntime` prim/body mapping, dirty synchronization, and fixed-step orchestration | Stage Runner's `stageRuntime`; do not pull `PrimId` or `RuntimeWorld` into the external core. |
| Jolt initialization, resources, layers, stepping, queries, and native-ID maps | External `physicsJolt`; all Jolt types and configuration remain private. |
| Runner physics schema interpretation and Stage-specific validation | Temporary Stage Runner compatibility importer. |
| Standard `UsdPhysics` interpretation and reusable scene/resource mappings | External `physicsUsd` after core/backend extraction and Stage Runner package migration. |
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

The first Stage Runner consumer migration implemented this bounded scope:

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

The receiving repository completed extraction and hosted backend verification
before the first `physicsUsd` slice. Stage Runner keeps the compatibility
importer local while consuming the installed core and backend packages. The
later `physicsUsd` phase introduces the standard authored path and preserves
the same parity gate before Runner physics APIs are removed.

## Accepted receiving-package decisions

The extracted packages resolved the migration-facing contracts:

- public headers use `usd_physics/` and public names use
  `usd_physics::core` or `usd_physics::jolt`;
- neutral vector, quaternion, and transform values belong to `physicsCore`;
- validation and backend failures use the package's typed error surface;
- descriptors use semantic collision category/mask filters rather than Jolt
  layer numbers; and
- the Stage-owned bridge converts values explicitly and keeps `PrimId` and
  `RuntimeWorld` out of the package.

The receiving package remains authoritative for detailed handle lifecycle and
changed-body ordering contracts.

## Parity gate

Local Windows parity passes for the neutral, Jolt, Character, Camera,
Stage-session, standalone, usdview, plain-CMake, and OpenStrata paths against
installed packages. Mapping and dirty-synchronization assertions remain in
Stage Runner. Immutable Windows and Linux package artifacts are public and
verified from fresh caches. The
[hosted consumer report](../reports/ost/04-2026-09-23-phase-c-hosted-physics-consumer.md)
records 46 passing OpenStrata tests on each OS, the usdview and Jolt scenarios,
and successful plain-CMake jobs. Standard `UsdPhysics` interpretation remains
the next authored-data migration.
