# Current Work

## Phase D: Standard `UsdPhysics` Declarations

### Status

Not started. Phase C's installed-package migration is complete. The
[hosted consumer report](../reports/ost/04-2026-09-23-phase-c-hosted-physics-consumer.md)
records Windows and Linux CMake and OpenStrata results, including a successful
run with artifact caches disabled.

Stage Runner still imports `RunnerPhysicsBodyAPI` and `RunnerColliderAPI` as a
compatibility path. The next slice makes standard `UsdPhysics` declarations the
primary source for runtime physics without changing the Stage-owned
`PhysicsRuntime` bridge or the shared `StageSession` host path.

### Next slice

- Implement the reusable standard-declaration interpretation in the sibling
  `usd-physics-plugins` package, starting with the rigid bodies and colliders
  needed by existing Stage Runner scenarios.
- Compose that package at the Stage boundary and keep the current Runner-schema
  importer temporarily for existing fixtures.
- Migrate representative falling-body, Character, Camera, standalone, and
  usdview fixtures to the standard path and compare their behavior with the
  compatibility path before removing Runner physics schemas.
- Keep generic physics contracts and backend implementation in the sibling
  repository; Stage Runner retains Stage/session orchestration and gameplay
  policy.

The intended ownership and parity requirements are in the
[physics extraction contract](../design/physics-extraction.md) and
[Proposed Design 0002](../design/proposed/0002-physics-repository-boundary.md).
