# usd-stage-runner

`usd-stage-runner` is an experimental C++ runtime that opens an OpenUSD Stage,
derives a transient Runtime World from its prims, and advances that world in a
bounded real-time update loop. The project is being delivered as small vertical
slices; physics, character control, and first- and third-person camera following
with collision avoidance are implemented.

The intended architecture and the distinction between implemented and planned
behavior are documented in [docs/README.md](docs/README.md).

## Current capabilities

- `runtimeCore`, with an injectable frame clock, bounded fixed-step accumulator,
  a host-facing play-session controller for play, pause, stop, single-step, and
  reset, a prim-indexed component registry, Runtime World, runtime transforms,
  and a dirty synchronization queue;
- `inputCore`, with named action state and backend-neutral movement intent;
- `physicsCore`, with typed backend-neutral resource handles, descriptors for
  boxes, bodies, and fixed constraints, fixed-step commands, changed-body
  extraction contracts, character ground and collision-segment query
  extensions, and prim/body runtime synchronization;
- `characterCore`, with backend-neutral character intent and controller state,
  walkable-ground and slope evaluation, desired motion, facing, jump-edge
  handling, and deterministic tests against a physics test double;
- `cameraCore`, with prim-indexed target and optional anchor identities, free,
  first-person, third-person, and orbit modes, deterministic target following,
  optional third-person collision probes, live pose state, exponential
  smoothing, and dirty Runtime World translation updates without OpenUSD, Jolt,
  or a renderer;
- `vehicleCore`, with normalized throttle, brake, steering, and handbrake
  intent; explicit chassis identity; independently composed steering,
  powertrain, service-brake, and handbrake configuration; and deterministic
  per-wheel command distribution that does not assume four wheels;
- `physicsJolt`, which owns Jolt initialization and resource lifetime, creates
  box shapes and static or dynamic bodies, advances fixed simulation steps,
  extracts changed body state, and implements character ground shape casts and
  camera collision ray casts without exposing Jolt types publicly;
- `runnerSchema`, a codeless OpenUSD plugin defining the single-apply
  `RunnerPhysicsBodyAPI`, `RunnerColliderAPI`, `RunnerCharacterAPI`, and
  `RunnerCameraRigAPI` declaration contracts;
- `stageRuntime`, a reusable OpenUSD-facing play session that imports the
  Runtime World and physics, character, and camera systems, owns fixed-step
  execution and reset/rebuild semantics, and incrementally synchronizes dirty
  transforms into a discardable anonymous runtime layer for any host without
  changing persistent authored layers;
- `inputSdl`, which maps WASD, arrow keys, and the first gamepad's left stick to
  `move.x` and `move.y`, and maps Space or the gamepad south button to `jump`,
  without exposing SDL types to core consumers;
- `stage_runner`, a thin standalone adapter that opens a Stage, selects Jolt and
  SDL adapters, polls input, and drives the shared `stageRuntime` session for an
  explicit frame bound;
- `usdviewStageRunner`, a Python usdview menu and timer adapter backed by a
  native binding to the same `StageSession`, with play, pause, stop,
  single-step, and reset controls and bundled Runner schema registration;
- minimal transform, falling-cube, character-import, runnable walk-and-jump,
  and obstructed first-/third-person camera-follow USDA fixtures plus
  dependency-free unit tests; and
- dual build paths through plain CMake and OpenStrata.

The character-control, camera-rig, and host-integration milestones are
implemented end to end, except for interactive host verification that depends
on a runtime containing usdview. Vehicle composition is in progress: the core
intent and wheel-command contract is implemented, while physics application,
USD schemas, Stage import, and the representative fixture remain. Behavior and
OpenExec integration are later slices.

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

## usdview plugin

The usdview adapter is built when OpenUSD includes Python support. Add the
generated package parent to `PYTHONPATH` and the package directory containing
`plugInfo.json` to `PXR_PLUGINPATH_NAME`:

```powershell
$env:PYTHONPATH = "$PWD\build\cy2026-windows-x86_64-py313-usd\plugins\usdviewStageRunner\python;$env:PYTHONPATH"
$env:PXR_PLUGINPATH_NAME = "$PWD\build\cy2026-windows-x86_64-py313-usd\plugins\usdviewStageRunner\python\usdviewStageRunner;$env:PXR_PLUGINPATH_NAME"
usdview tests\fixtures\minimal.usda
```

The **Stage Runner** menu exposes Play, Pause, Stop, Single Step, and Reset.
Stop and Reset discard the plugin-owned anonymous runtime layer; they do not
change persistent authored layers. See the
[plugin README](plugins/usdviewStageRunner/README.md) for layout and runtime
defaults.

### OpenStrata Plugin View

The `plugin-view` build intent stages that same usdview package and native
`StageSession` binding into the `runnerSchema` bundle. OpenStrata then supplies
the bundle's Python, plugin-discovery, and loader paths without changing the
current shell:

```powershell
ost build --intent plugin-view
ost plugin view plugins/runnerSchema tests/fixtures/character_walk.usda
```

This intentionally does not use `--with`: OpenStrata 0.22.8 requires every
`--with` input to be a manifest-backed plugin bundle, but does not currently
model a usdview Python host extension as a plugin kind. The `plugin-view`
intent therefore stages the adapter inside the valid `runnerSchema` bundle.

`third_person_camera.usda` can be opened the same way. The selected OpenStrata
runtime must contain usdview, and a Jolt package must be discoverable at build
time to execute physics declarations; otherwise the shared adapter reports the
same unavailable-backend error as `stage_runner` and ordinary usdview.

## Host usage

```text
stage_runner <scene.usd[a|c]> [--frames N] [--fixed-dt SECONDS]
             [--max-fixed-steps N] [--deterministic]
             [--move-x VALUE] [--move-y VALUE] [--jump]
```

The default loop runs 300 frames at a 60 Hz target. `--deterministic` injects
one fixed interval per frame and does not sleep, making integration tests fast
and repeatable. `--move-x` and `--move-y` accept normalized values from -1 to 1,
and `--jump` holds the jump action for deterministic adapter-to-Stage tests.
All three require `--deterministic`. Interactive runs use SDL keyboard and
gamepad input.

Physics prims apply both `RunnerPhysicsBodyAPI` and `RunnerColliderAPI`.
`runner:physics:motionType` accepts `static` or `dynamic`, mass is authored with
`runner:physics:mass`, and the initial collider contract uses
`runner:physics:shape = "box"` plus positive local-space
`runner:physics:halfExtents`. Ordered scale ops multiply those extents. A Stage
with physics declarations must be Y-up with `metersPerUnit = 1`. Physics prims
require one translate op followed only by scale ops, plus identity ancestor
transforms unless they set `resetXformStack`. A build with both OpenUSD and Jolt
is required to simulate the declarations. Authored `runner:physics:*`
attributes without their owning API schema are rejected as legacy data.

Character prims additionally apply `RunnerCharacterAPI` to the same dynamic
physics prim. `runner:character:groundProbeDistance`,
`runner:character:maximumSlopeAngleRadians`, and
`runner:character:jumpSpeed` configure the prim-indexed runtime controller.
Character attributes without `RunnerCharacterAPI`, characters without both
physics APIs, and static character bodies are rejected.

Camera prims apply `RunnerCameraRigAPI`. `runner:camera:target` and the optional
`runner:camera:anchor` are prim relationships; non-free modes require exactly
one target. Mode, offset, distance, pitch, yaw, damping, collision enablement,
and collision clearance are read into a prim-indexed camera rig. The importer
rejects non-camera application sites,
unresolved or non-xformable references, invalid values, and authored camera
properties without their owning API schema.
Camera declarations currently require a Y-up Stage. Camera, target, and anchor
translations must already be representable in world space by their imported
local `RuntimeTransform`; non-identity ancestor transforms require
`resetXformStack` until composed camera transforms are supported. Rig Camera
prims accept an empty transform stack or one translate op optionally followed
by the reserved double-precision `xformOp:orient:runnerCamera`; other authored
transform ops are rejected so the runtime pose is not composed with an unknown
rotation. A rig may not target or anchor itself. At each fixed step, imported
rigs evaluate after physics extraction. Changed camera poses use the shared
dirty queue to update translation and the reserved orientation only.
Collision-enabled third-person rigs use the optional backend-neutral segment
query, ignore a bound target body, shorten the desired camera distance by the
authored clearance, and smooth the collision-adjusted pose.

## License

Project code is licensed under Apache-2.0.
