# 0002: Physics Repository Boundary

Status: proposed

## Context

`usd-stage-runner` currently owns both the orchestration layer that advances an
OpenUSD Stage and the reusable physics contracts and Jolt adapter used by that
layer. The appearance of a separate `usd-physics-plugins` repository creates a
clearer ownership boundary: physics contracts, backend lifetime, and physics
USD interpretation can serve Stage Runner, MMD, VRM, mobility, robotics, and
other OpenUSD consumers without making Stage Runner their dependency.

Keeping those responsibilities together would also encourage character,
camera, and vehicle code to depend on Jolt-specific concepts and would make
Runner-specific physics schemas compete with the standard `UsdPhysics`
authored model.

## Proposal

Define `usd-stage-runner` as a lightweight real-time runtime and orchestration
layer that turns an OpenUSD Stage into an interactive world. It owns the
Runtime World, fixed-step scheduling, play-session lifecycle, subsystem
coordination, transient USD synchronization, and host adapters. It composes
reusable subsystems through CMake packages and OpenStrata rather than owning
every implementation.

Move the reusable physics kernel to `usd-physics-plugins`:

- backend-neutral worlds, bodies, shapes, constraints, handles, commands, and
  state extraction;
- optional ground, collision, and future vehicle-oriented queries;
- Jolt initialization, lifetime, layers, stepping, and query implementation;
- deterministic physics contract and adapter tests; and
- interpretation of standard OpenUSD physics declarations into
  backend-neutral descriptors.

Keep the following responsibilities in Stage Runner:

- `runtimeCore` and the prim-indexed Runtime World;
- `stageRuntime` traversal, subsystem composition, fixed-step ordering,
  lifecycle, and runtime-layer synchronization;
- `inputCore` and `inputSdl`;
- character, camera, vehicle, and behavior gameplay policy; and
- standalone, usdview, and future host adapters.

Character, camera, and vehicle code consume narrow backend-neutral physics
capabilities. No Jolt types may cross their public boundaries, and
`stageRuntime` must not depend on Jolt directly. OpenExec remains an optional
thin execution adapter over ordinary runtime operations.

The canonical authored physics representation should become standard
`UsdPhysics` schemas. `RunnerPhysicsBodyAPI` and `RunnerColliderAPI` remain a
temporary compatibility surface during migration, then are deprecated or
removed. Runner-specific schemas remain appropriate for gameplay semantics,
including character, camera, and future vehicle declarations, that standard
OpenUSD schemas cannot express.

## Target dependency direction

```text
runtimeCore

inputCore       -> runtimeCore
characterCore   -> runtimeCore + physics contracts
cameraCore      -> runtimeCore
vehicleCore     -> runtimeCore + physics contracts
behaviorCore    -> runtimeCore

stageRuntime    -> runtimeCore
                + inputCore
                + characterCore
                + cameraCore
                + vehicleCore
                + usd-physics-plugins
                + OpenUSD

inputSdl        -> inputCore + SDL
host adapters   -> stageRuntime + selected adapters
```

Forbidden dependencies include:

```text
runtimeCore    -/-> Jolt, SDL, or OpenExec
characterCore  -/-> Jolt
cameraCore     -/-> Jolt
vehicleCore    -/-> Jolt
stageRuntime   -/-> Jolt directly
```

## Delivery sequence

1. Freeze and inventory the current physics contracts and direct ownership.
2. Extract the minimum reusable physics kernel and Jolt backend.
3. Add standard `UsdPhysics` interpretation with temporary Runner-schema
   compatibility where needed.
4. Migrate Stage Runner to consume `usd-physics-plugins` through CMake and
   OpenStrata composition.
5. Validate the contracts against Stage Runner, MMD, and VRM requirements.
6. Resume vehicle physics using composable capabilities rather than a
   monolithic four-wheel abstraction.
7. Continue behavior, richer runtime systems, and thin OpenExec adapters.

The implemented `vehicleCore` intent, arbitrary wheel-role composition, and
deterministic command generation are preserved while vehicle physics waits for
the shared boundary to stabilize.

## Consequences

- Stage Runner becomes a consumer of physics rather than the owner of a
  physics backend.
- Physics APIs must survive more than one real OpenUSD consumer.
- Standard `UsdPhysics` declarations become portable between runtime clients.
- Extraction requires a compatibility period while current fixtures and
  Runner physics schemas still exist.
- Host and Stage-session construction must receive external physics
  capabilities without selecting or exposing Jolt in core code.
- Vehicle delivery pauses after its backend-neutral core slice, avoiding an
  integration that would immediately need to be moved.

## Acceptance criteria

Move this decision to `accepted/` when:

- `physicsCore` and `physicsJolt` no longer live in this repository;
- Stage Runner consumes the extracted package without public Jolt types or a
  direct `stageRuntime`-to-Jolt dependency;
- standard `UsdPhysics` is the primary authored physics representation;
- character and camera scenarios pass through narrow physics capabilities;
- the extracted contracts are validated by at least one additional consumer;
- standalone and usdview hosts still share `StageSession`; and
- runtime results remain incremental and confined to the discardable session
  layer.
