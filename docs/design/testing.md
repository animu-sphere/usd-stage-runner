# Testing Strategy

Status: runtime, input, physics-core contracts, SDL mapping, and input-to-USD
integration coverage implemented

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

## Representative fixtures

Fixtures are added with their implementation milestone rather than committed as
empty promises.

| Capability | Planned fixture |
| --- | --- |
| Physics | `falling_cube.usda` |
| Character | `character_walk.usda` |
| Camera | `third_person_camera.usda` |
| Vehicle | `four_wheel_vehicle.usda` |
| Behavior | `behavior_chase.usda` |

Each fixture should remain small, reproducible, and useful from both automated
tests and an interactive host.

## Per-milestone verification

Before a milestone is complete:

1. core contracts have isolated tests;
2. concrete adapter behavior has focused coverage;
3. the full Stage-to-runtime-to-Stage path is exercised where applicable;
4. fixed-step results are repeatable under a controlled clock;
5. dirty synchronization is verified rather than inferred; and
6. both plain CMake and OpenStrata build paths remain valid for their supported
   environments.
