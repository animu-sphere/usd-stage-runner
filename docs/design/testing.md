# Testing Strategy

Status: runtime, play-session lifecycle, input, physics-core, character-core,
camera-core, vehicle intent and wheel-command composition, Stage-session
rebuild and discardable-layer contracts, usdview
native-binding lifecycle and OpenStrata-staged bundle smoke coverage,
runtime-integration contracts, SDL mapping, Jolt bootstrap,
ground queries, and
collision segment queries,
physics, character, and camera schema registration, round-trip and import
validation, input-to-USD, obstructed camera synchronization, and conditional
full physics, character, and camera vertical-slice coverage implemented

Deterministic testing is a design requirement. Each vertical slice must be
testable with controlled time and input and should add a runnable Stage fixture.

## Test layers

| Layer | Coverage |
| --- | --- |
| Unit | `runtimeCore`, `inputCore`, `physicsCore`, `characterCore`, `cameraCore`, `behaviorCore`, and `vehicleCore` logic without backend SDKs where practical. |
| Adapter | SDL mapping, Jolt creation and stepping, schema import, and host-specific boundaries. |
| Integration | Stage import through input, simulation, runtime transforms, and incremental USD synchronization. |
| Golden scenario | A minimal Stage that demonstrates the completed milestone end to end. |

Physics tests use fixed steps and an injectable clock. A deterministic mock
backend should validate `physicsCore` contracts before or alongside the Jolt
adapter. Wall-clock sleeps and physical devices are not required for core or
integration coverage.

Under the proposed physics extraction, the same test intent is split by
ownership:

- `usd-physics-plugins` owns backend-neutral contract tests, Jolt adapter tests,
  and standard `UsdPhysics` interpretation tests;
- Stage Runner owns tests for subsystem composition, Character and Camera
  capability use, fixed-step order, lifecycle, and runtime-layer
  synchronization; and
- cross-repository OpenStrata tests prove package discovery, host composition,
  and representative Stage execution without introducing a direct Jolt
  dependency in `stageRuntime`.

During migration, representative physics fixtures should gain standard
`UsdPhysics` variants before Runner physics declarations are removed. Contract
tests must continue to run without a concrete backend, and extraction is not
complete until at least one non-Stage-Runner consumer exercises the shared
package.

## Representative fixtures

Fixtures are added with their implementation milestone rather than committed as
empty promises.

| Capability | Planned fixture |
| --- | --- |
| Physics | `falling_cube.usda` (implemented) |
| Character | `character_walk.usda` (implemented) |
| Camera | `third_person_camera.usda` (implemented) |
| Host play session | `character_walk.usda` and `third_person_camera.usda` (reused) |
| Vehicle | `four_wheel_vehicle.usda` |
| Behavior | `behavior_chase.usda` |

Each fixture should remain small, reproducible, and useful from both automated
tests and an interactive host. Host tests should reuse the representative
character and camera Stages and verify session-layer discard and reset
semantics instead of introducing a host-specific scene.

## Per-milestone verification

Before a milestone is complete:

1. core contracts have isolated tests;
2. concrete adapter behavior has focused coverage;
3. the full Stage-to-runtime-to-Stage path is exercised where applicable;
4. fixed-step results are repeatable under a controlled clock;
5. dirty synchronization is verified rather than inferred; and
6. both plain CMake and OpenStrata build paths remain valid for their supported
   environments.
