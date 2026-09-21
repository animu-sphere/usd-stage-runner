# Planned Delivery Phases

Status: Phase A in progress; later phases not started

Character, camera, host integration, and the backend-neutral `vehicleCore`
composition slice are implemented and recorded in the
[current architecture](../architecture/overview.md). The next delivery sequence
establishes a reusable physics repository boundary before vehicle physics or
new runtime systems expand the current coupling.

## Delivery order

| Phase | Capability | Proof |
| --- | --- | --- |
| A | Physics boundary freeze | Current ownership and the minimum Character, Camera, and Vehicle capability contracts are documented and protected by deterministic tests. |
| B | `usd-physics-plugins` extraction | The backend-neutral kernel and Jolt backend build and test independently of Stage Runner. |
| C | `UsdPhysics` canonicalization | Standard OpenUSD physics declarations construct backend-neutral runtime state, with bounded compatibility for existing Runner declarations. |
| D | Stage Runner consumer migration | CMake and OpenStrata compose the external package; Stage Runner no longer owns or directly depends on Jolt. |
| E | Shared consumer validation | Stage Runner plus MMD and VRM requirements validate the reusable contracts. |
| F | Vehicle physics | A USD-composed vehicle is drivable through composable physics capabilities without a four-wheel-only runtime contract. |
| G | Behavior and richer runtime systems | Behavior, thin OpenExec adapters, animation integration, and runtime tooling grow on the stable substrate. |

## Phase A: Physics boundary freeze

Audit `physicsCore`, `physicsJolt`, Stage physics import, Runner physics schemas,
Character and Camera queries, Vehicle requirements, tests, builds, and host
composition. Freeze the reusable public contract and stop adding
Runner-specific physics schemas. Detailed work and completion criteria are in
[the current milestone](current.md).

## Phase B: `usd-physics-plugins` extraction

Move the minimum reusable kernel first:

- backend-neutral worlds, bodies, shapes, constraints, handles, commands, and
  changed-state extraction;
- optional ground and collision queries already proven by Character and
  Camera;
- the Jolt backend and its lifecycle, stepping, layers, and queries; and
- deterministic contract and focused adapter tests.

Do not redesign every physics feature during the move. Preserve behavior and
make ownership clear before adding capabilities.

Success: the physics package builds and tests without Stage Runner, and its
public API exposes no Jolt types.

## Phase C: `UsdPhysics` canonicalization

Teach the physics package to interpret standard declarations including rigid
bodies, collisions, mass, joints, drives, and limits as demanded by working
slices. Retain temporary compatibility with `RunnerPhysicsBodyAPI` and
`RunnerColliderAPI` only as needed for migration.

Success: standard `UsdPhysics` is the primary authored representation and no
new Runner-specific physics schema is required.

## Phase D: Stage Runner consumer migration

- Consume `usd-physics-plugins` through exported CMake packages.
- Compose the repositories through OpenStrata for build, test, packaging, and
  host workflows.
- Remove repository-local Jolt ownership and direct Jolt selection from
  `stageRuntime`.
- Keep standalone and usdview on the same `StageSession`, fixed-step order, and
  discardable runtime-layer policy.

Success: the Stage Runner repository contains orchestration and gameplay policy
but no physics backend, while existing Character and Camera scenarios retain
their behavior.

## Phase E: Shared consumer validation

Validate abstractions against real requirements from:

- `usd-stage-runner` for rigid bodies, Character, Camera, and later Vehicle;
- `usd-mmd-plugins` for its physics-driven use cases; and
- `usd-vrm-plugins` for secondary motion and related runtime needs.

Retain only abstractions that survive multiple consumers. Add narrow optional
capabilities rather than widening the base interface speculatively.

Success: at least one non-Stage-Runner consumer uses the extracted package and
the contracts cover the documented MMD and VRM requirements without format
logic entering Stage Runner.

## Phase F: Vehicle physics

Resume from the implemented, frozen `vehicleCore` contract:

```text
named actions or behavior
    -> VehicleIntent
    -> vehicleCore wheel-command composition
    -> physics capabilities
    -> usd-physics-plugins
    -> runtime transform changes
    -> incremental USD synchronization
```

Prefer rigid bodies, constraints, forces, torques, contact or friction data,
and wheel or suspension queries over a monolithic backend vehicle object.
Introduce `RunnerVehicleAPI`, wheel declarations, and narrower gameplay schemas
only with the importer that consumes them. Preserve independent wheel roles and
support for non-four-wheel layouts.

Success: a USD-composed representative vehicle is drivable with deterministic
input and tests, while its physics path remains reusable by other vehicle
forms.

## Phase G: Behavior and richer runtime systems

### Behavior

Add `behaviorCore`, blackboards, stateful composites, decorators, conditions,
tasks, and events. Behavior produces the same Character and Vehicle intent
contracts as player input and does not manipulate backend simulation objects.

### OpenExec

Add thin `execCharacter`, `execRunner`, `execVehicle`, and `execBehavior`
wrappers only for proven use cases. Core algorithms remain callable without
OpenExec, and OpenExec never owns the host loop or domain state.

### Animation and tooling

Connect runtime motion to USD Skeleton and animation state without making an
avatar format the runtime identity. Add host-rendered inspection, debug
primitives, profiling, explicit bake, and selected-property commit workflows.

## Cross-cutting follow-up

- Incremental USD-to-runtime updates using Stage change notices.
- CI checks for forbidden repository and target dependency edges.
- Additional input and physics backends.
- Richer camera collision, rig blending, and cinematic modes.
- Dedicated SDL/GLFW viewport, editor integration, headless simulation, and
  remote or application hosts.
- Packaging, compatibility policy, release records, task guides, and generated
  API/schema reference once real usage requires them.

The initial non-goals in [concepts/overview.md](../concepts/overview.md) remain
out of scope until a completed vertical slice demonstrates a concrete need.
