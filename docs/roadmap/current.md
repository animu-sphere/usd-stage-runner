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

The implemented extraction baseline is frozen in the
[physics extraction inventory](../architecture/physics-extraction-inventory.md)
at source revision `12324992c7ddd0b016ace780acadc5f07903390c`. That inventory
records the public contract, ownership classification, consumers,
compatibility surface, and deterministic evidence. The
[extraction contract](../design/physics-extraction.md) records Character,
Camera, and Vehicle capability requirements plus the package and migration
seams. Phase A remains in progress while the receiving package resolves the
open public namespace, neutral math, error, collision-filter, and
changed-state-ordering decisions.

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

## Remaining Phase A scope

- Resolve the receiving package's public namespace and include root without
  retaining Stage Runner ownership names.
- Replace `runtimeCore` math dependencies with receiving-package neutral math
  while keeping `PrimId <-> BodyHandle` mapping and dirty synchronization in
  `stageRuntime`.
- Settle error transport, semantic collision filtering, stale-handle behavior,
  and changed-state ordering before accepting external public headers.
- Coordinate whether Stage Runner consumes the installed core/backend before
  or after the first `physicsUsd` slice; the two proposed roadmaps currently
  order those validation steps differently.
- Prove installed-package consumption in plain CMake and equivalent
  `physicsCore`/`physicsJolt` OpenStrata composition for tests, packaging,
  standalone, and usdview.
- Preserve the inventoried deterministic tests before files or packages move.
- Do not add new Runner-specific physics schemas or Jolt-specific public
  concepts.

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
