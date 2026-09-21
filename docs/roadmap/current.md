# Current Milestone: Physics Boundary Freeze

Status: in progress

Stage Runner currently owns `physicsCore`, `physicsJolt`, physics-specific Stage
import, and Runner physics schemas. The next milestone freezes and inventories
those contracts before extracting the reusable kernel to
`usd-physics-plugins`. This work takes priority over vehicle physics so the
vehicle integration is built once against the intended shared substrate.

The governing proposal is
[0002: Physics Repository Boundary](../design/proposed/0002-physics-repository-boundary.md).
Architecture pages continue to describe the repository-local implementation
until the extraction actually lands.

## Outcome

```text
OpenUSD Stage
    -> stageRuntime orchestration
    -> backend-neutral physics capabilities
    -> usd-physics-plugins
    -> selected backend such as Jolt
    -> changed runtime state
    -> discardable USD runtime layer
```

Stage Runner remains responsible for Runtime World construction, fixed-step
order, play-session lifecycle, character/camera/vehicle gameplay policy, host
adapters, and incremental synchronization. The extracted package owns reusable
physics contracts, the Jolt backend, and physics-specific USD interpretation.

## Phase A scope

### Freeze the current contract

- Document the public surface of `physicsCore`: handles, descriptors,
  commands, state extraction, optional queries, and `PhysicsRuntime` mapping.
- Preserve deterministic contract tests before files or packages move.
- Do not add new Runner-specific physics schemas or Jolt-specific public
  concepts.

### Inventory ownership and consumers

- Identify every direct physics responsibility in `stageRuntime`, standalone,
  usdview, fixtures, CMake, and OpenStrata packaging.
- Record the minimum ground, body-state, velocity, and collision capabilities
  required by Character and Camera.
- Record the additional rigid-body, constraint, force, torque, contact, wheel,
  and suspension capabilities that Vehicle may require.
- Separate reusable physics USD interpretation from Stage Runner-specific
  character, camera, and vehicle import.

### Define migration seams

- Define the CMake package boundary used by Stage Runner.
- Define OpenStrata composition for development, packaging, tests, and hosts.
- Plan temporary compatibility for `RunnerPhysicsBodyAPI` and
  `RunnerColliderAPI` while standard `UsdPhysics` import is introduced.
- Keep standalone and usdview hosts on the same `StageSession` API.

## Vehicle work during the freeze

The implemented `vehicleCore` contract remains in Stage Runner and is not
discarded. Freeze and retain:

- normalized throttle, brake, steering, and handbrake `VehicleIntent`;
- explicit chassis identity;
- independent steering, powertrain, service-brake, and handbrake roles;
- deterministic per-wheel command generation; and
- arbitrary wheel counts and backend-neutral tests.

Do not add vehicle physics, Jolt-specific vehicle types, or Runner vehicle
schemas until the shared physics boundary is stable. Input mapping and Stage
integration resume with the later vehicle phase.

## Completion criteria

- The reusable and Stage Runner-specific parts of `physicsCore`,
  `physicsJolt`, physics import, schemas, tests, and packaging are explicitly
  inventoried.
- Character, Camera, and Vehicle capability requirements are documented
  without Jolt types.
- The extraction package and OpenStrata composition seams are agreed and
  testable.
- Existing physics, character, camera, host, and vehicle-core behavior remains
  covered by deterministic tests.
- No new Stage Runner-specific physics schema or backend coupling is added.

After these criteria are met, continue with Phase B in
[the planned delivery phases](milestones.md): extract the minimum reusable
physics kernel and Jolt backend before redesigning or extending features.
