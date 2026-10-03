# Current Work

## Phase D: Standard `UsdPhysics` Declarations

### Status

In progress: the optional standard Box slice is implemented. Phase C's
installed-package migration is complete. The
[hosted consumer report](../reports/ost/04-2026-09-23-phase-c-hosted-physics-consumer.md)
records Windows and Linux CMake and OpenStrata results, including a successful
run with artifact caches disabled.

The [current architecture](../architecture/overview.md#standard-box-physics-import)
records the optional installed `physicsUsd` path. The default build retains
Runner compatibility until the new package enters the pinned artifact graph.

The pre-publication CI gate now installs a pinned parser and checks CMake and
OpenStrata on both OSes. The [local Phase D report](../reports/ost/05-2026-10-04-phase-d-standard-physics.md)
records Windows results; hosted execution and artifact publication remain open.

### Next slice

- Publish and pin Windows/Linux `physicsUsd` artifacts, add the package to
  Stage Runner's OpenStrata requirements, then enable it by default.
- Execute the standard/compatibility CI gate in hosted CMake/OpenStrata on both
  platforms, then repeat against published artifact pins. Local Windows
  coverage includes falling bodies, Character, Camera, standalone, native
  usdview, and the OpenStrata-staged adapter.
- Switch default host examples to standard fixtures after that delivery gate;
  retain compatibility fixtures until Runner physics schemas can be removed.
- Extend the sibling parser through working fixtures for gravity, additional
  shapes, mass inference, joints, drives, and limits.
- Keep generic physics contracts and backend implementation in the sibling
  repository; Stage Runner retains Stage/session orchestration and gameplay
  policy.

The intended ownership and parity requirements are in the
[physics extraction contract](../design/physics-extraction.md) and
[Proposed Design 0002](../design/proposed/0002-physics-repository-boundary.md).
