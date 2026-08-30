# usd-stage-runner

`usd-stage-runner` is an experimental C++ runtime that opens an OpenUSD Stage,
derives a transient Runtime World from its prims, and advances that world in a
bounded real-time update loop. The project is being delivered as small vertical
slices; the first input-to-Jolt-to-USD physics slice is implemented.

The intended architecture and the distinction between implemented and planned
behavior are documented in [docs/README.md](docs/README.md).

## Current capabilities

- `runtimeCore`, with an injectable frame clock, bounded fixed-step accumulator,
  prim-indexed component registry, Runtime World, runtime transforms, and a dirty
  synchronization queue;
- `inputCore`, with named action state and backend-neutral movement intent;
- `physicsCore`, with typed backend-neutral resource handles, descriptors for
  boxes, bodies, and fixed constraints, fixed-step commands, changed-body
  extraction contracts, and prim/body runtime synchronization;
- `physicsJolt`, which owns Jolt initialization and resource lifetime, creates
  box shapes and static or dynamic bodies, advances fixed simulation steps, and
  extracts changed body state without exposing Jolt types publicly;
- `inputSdl`, which maps WASD, arrow keys, and the first gamepad's left stick to
  `move.x` and `move.y` without exposing SDL types to core consumers;
- `stage_runner`, which uses OpenUSD when an SDK is available and has an explicit
  frame bound, imports the temporary physics convention, maps movement intent to
  desired body velocity, advances Jolt, and synchronizes only dirty runtime
  transforms to USD;
- minimal transform and falling-cube USDA fixtures plus dependency-free unit
  tests; and
- dual build paths through plain CMake and OpenStrata.

Formal physics API schemas are the next roadmap milestone. Character, camera,
behavior, and OpenExec integration are later slices.

## Build with OpenStrata

[OpenStrata](https://github.com/animu-sphere/open-strata) supplies the pinned
OpenUSD runtime and compiler environment. With `ost` installed:

```powershell
ost runtime pull cy2026 --profile usd
ost build
ost test
```

The reusable core can also be built and tested as an isolated OpenStrata
library member:

```powershell
ost library build libs\runtimeCore
ost library test libs\runtimeCore
```

To run the staged executable directly, first enter the runtime-activated shell:

```powershell
ost devshell cy2026 --profile usd
```

Then run the deterministic smoke path inside that shell:

```powershell
.\apps\stage_runner\bin\stage_runner.exe tests\fixtures\minimal.usda --frames 4 --deterministic
```

## Build with plain CMake

A C++17 compiler is sufficient for `runtimeCore`. Point `CMAKE_PREFIX_PATH` at
an OpenUSD installation to enable real Stage loading in `stage_runner`:

```powershell
cmake --preset dev -DCMAKE_PREFIX_PATH=C:\path\to\openusd
cmake --build --preset dev
ctest --preset dev
```

Without OpenUSD, the host still compiles but reports that Stage loading is
unavailable; the backend-neutral unit tests remain buildable. Set
`USD_STAGE_RUNNER_REQUIRE_OPENUSD=ON` when a missing SDK should be a configure
error. Interactive input is enabled when CMake can find `SDL3::SDL3` or
`SDL2::SDL2`; set `USD_STAGE_RUNNER_REQUIRE_SDL=ON` to require a real SDL-backed
demo build. The OpenStrata `usd` profile does not currently bundle SDL, so pass
an SDL package through `CMAKE_PREFIX_PATH` for interactive builds. The Jolt
adapter similarly uses a `Jolt::Jolt` or `Jolt` CMake package when available;
set `USD_STAGE_RUNNER_REQUIRE_JOLT=ON` to require it. Without that package, the
adapter remains buildable but reports that world creation is unavailable.

## Host usage

```text
stage_runner <scene.usd[a|c]> [--frames N] [--fixed-dt SECONDS]
             [--max-fixed-steps N] [--deterministic]
             [--move-x VALUE] [--move-y VALUE]
```

The default loop runs 300 frames at a 60 Hz target. `--deterministic` injects
one fixed interval per frame and does not sleep, making integration tests fast
and repeatable. `--move-x` and `--move-y` accept normalized values from -1 to 1
for deterministic adapter-to-Stage tests and require `--deterministic`.
Interactive runs use SDL keyboard and gamepad input.

The temporary physics convention applies `runner:physics:motionType = "static"`
or `"dynamic"` to a `UsdGeomCube`; dynamic bodies may also author
`runner:physics:mass`. Cube size and scale determine box half extents. A Stage
using these attributes must be Y-up with `metersPerUnit = 1`. Physics Cubes
require one translate op followed only by scale ops, plus identity ancestor
transforms unless they set `resetXformStack`. A build with both OpenUSD and Jolt
is required. The next schema milestone will replace this convention with applied
API schemas.

## License

Project code is licensed under Apache-2.0.
