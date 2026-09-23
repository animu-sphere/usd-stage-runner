# Current Work

## External Physics Package Migration

### Status

In progress.

The Stage Runner no longer owns or builds the physics implementation. Its
runtime libraries and hosts now consume the installed `physicsCore` and
`physicsJolt` packages produced by the sibling `usd-physics-plugins`
repository.

### Completed Locally

- removed the repository-local `libs/physicsCore` and
  `backends/physicsJolt` source trees;
- changed the root build to require installed `physicsCore` and
  `physicsJolt` CMake packages;
- migrated character, vehicle, Stage runtime, standalone host, usdview host,
  and tests to the external `usd_physics::core` and `usd_physics::jolt`
  APIs;
- moved USD-Stage-specific prim/body ownership and dirty synchronization into
  the Stage-owned `PhysicsRuntime` bridge;
- kept runner-schema import and host integration in this repository;
- replaced raw collision-layer coupling with semantic category/mask filters;
- pinned external physics artifacts in the affected OpenStrata manifests;
- verified the Windows CMake build and the artifact-backed
  `plugin-view-jolt` intent;
- passed all 48 tests, including the direct Stage bridge test, standalone
  Jolt scenarios, and usdview plugin scenarios.

### Remaining

- produce and pin equivalent Linux artifacts;
- run hosted Windows and Linux Stage Runner CI against those published
  artifacts;
- record the hosted evidence before declaring the migration complete;
- continue toward the broader repository-boundary acceptance criteria in
  [Proposed Design 0002](../design/proposed/0002-physics-repository-boundary.md),
  including an additional external consumer and the planned USD-facing
  adapter work.

### Guardrails

- Stage Runner may own Stage/session integration, host wiring, and schema
  import code, but not generic physics contracts or backend implementations.
- No new repository-local physics backend or duplicate public physics type is
  added.
- External artifact identities stay explicit and immutable.
- A Jolt-required build must fail at configure time when the installed
  `physicsJolt` package reports that its backend is unavailable.
