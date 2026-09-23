# Modules and Dependency Boundaries

Status: intended contract; local physics extraction proposed, not implemented

The current repository implements `runtimeCore`, `inputCore`, `physicsCore`,
`characterCore`, `cameraCore`, the initial `vehicleCore`, `stageRuntime`,
`inputSdl`, `physicsJolt`, and the physics, character, and camera contracts in
`runnerSchema`. The target boundary below moves the reusable physics contracts,
Jolt adapter, and physics-specific USD interpretation to
`usd-physics-plugins`. See the
[proposed repository-boundary decision](proposed/0002-physics-repository-boundary.md).
The bounded hand-off details are in the
[physics extraction contract](physics-extraction.md).

## Core, capability, and adapter rule

Backend-neutral gameplay policy lives in ordinary C++ core libraries.
SDK-specific code lives in adapters or external subsystem packages. Stage
Runner coordinates those libraries; it does not absorb their implementation.

```text
inputCore      <- inputSdl
characterCore  <- host / execCharacter
cameraCore     <- host / execRunner
vehicleCore    <- host / execVehicle
behaviorCore   <- host / execBehavior
stageRuntime   <- host adapters

physics contracts <- usd-physics-plugins backends
stageRuntime       <- usd-physics-plugins composition
```

Public Stage Runner APIs must not expose SDL, Jolt, OpenExec, or unnecessary
OpenUSD types. Character, camera, and vehicle code consume the narrowest useful
backend-neutral physics capability, such as ground queries, collision queries,
body state, velocity commands, forces, or torques. New capability interfaces
are preferred over SDK-type leakage or a monolithic vehicle abstraction.

## Target responsibilities

| Area | Stage Runner responsibility | External capability or adapter responsibility |
| --- | --- | --- |
| Runtime | Prim identity, components, transforms, dirty queues, clocks, fixed-step accumulation, and play-session state. | None; `runtimeCore` remains backend-neutral. |
| Stage session | Stage traversal, Runtime World construction, subsystem composition and lifecycle, fixed-step order, reset/rebuild, and runtime-layer synchronization. | Physics construction and import are requested through `usd-physics-plugins` capabilities. |
| Input | Named actions and controller intent. | SDL device polling and mapping in `inputSdl`. |
| Physics | Consume backend-neutral contracts; do not own their implementation. | `usd-physics-plugins` owns worlds, bodies, shapes, constraints, commands, queries, Jolt lifetime, stepping, and state extraction. |
| Character | Gameplay intent, movement policy, grounding state, slope acceptance, facing, jumping, and rising/falling transitions. | Ground/body queries and velocity commands through physics contracts. |
| Camera | Targeting, rig modes, smoothing, desired/current pose, and callback-based collision probes. | Segment or ray queries through physics contracts; host rendering remains outside core. |
| Vehicle | Intent, chassis/wheel gameplay composition, and independent steering, powertrain, service-brake, and handbrake distribution for arbitrary wheel layouts. | Rigid bodies, constraints, forces, torques, contacts, and suspension or wheel queries through composable physics contracts. |
| Behavior | Stateful behavior-tree and blackboard evaluation that produces intent or runtime commands. | Thin host and OpenExec invocation. |
| Schema | Character, camera, vehicle, and other Stage Runner-specific gameplay declarations. | Standard `UsdPhysics` declarations and their physics interpretation belong with the physics package. |
| Hosts | Standalone, usdview, and later host lifecycle, UI, clocks, and composition choices. | Selected device, physics, rendering, or other adapters. |

## Target dependency direction

```text
runtimeCore

inputCore       -> runtimeCore
characterCore   -> runtimeCore + physics contracts
cameraCore      -> runtimeCore
vehicleCore     -> runtimeCore + physics contracts
behaviorCore    -> runtimeCore

stageRuntime    -> runtimeCore
                + inputCore
                + characterCore
                + cameraCore
                + vehicleCore
                + usd-physics-plugins
                + OpenUSD

inputSdl        -> inputCore + SDL
runnerSchema    -> OpenUSD
exec*           -> relevant core + runnerSchema + OpenExec
usdview adapter -> stageRuntime + usdview/OpenUSD Python APIs
standalone app  -> stageRuntime + selected adapters
```

Forbidden edges include:

```text
runtimeCore    -/-> OpenExec, Jolt, SDL, or OpenUSD
characterCore  -/-> Jolt
cameraCore     -/-> Jolt
vehicleCore    -/-> Jolt or OpenExec
behaviorCore   -/-> OpenExec
stageRuntime   -/-> Jolt directly
```

The CMake target graph and CI should enforce these boundaries during and after
the extraction. OpenStrata composes the repositories at build, packaging, and
runtime-host boundaries; it does not become a runtime API dependency.

## Repository ownership

```text
usd-stage-runner/
|- docs/
|- libs/
|  |- runtimeCore/
|  |- inputCore/
|  |- characterCore/
|  |- cameraCore/
|  |- stageRuntime/
|  |- behaviorCore/
|  `- vehicleCore/
|- backends/
|  `- inputSdl/
|- plugins/
|  |- runnerSchema/
|  |- execRunner/
|  |- execCharacter/
|  |- execBehavior/
|  |- execVehicle/
|  `- usdviewStageRunner/
`- apps/stage_runner/

usd-physics-plugins/
|- backend-neutral physics contracts
|- Jolt backend
|- UsdPhysics interpretation
`- deterministic contract and adapter tests
```

Directories and adapters are added only with a working vertical slice. The
local `libs/physicsCore` and `backends/physicsJolt` implementations have been
removed; installed `usd-physics-plugins` packages are now the authoritative
physics implementation. Stage Runner keeps only its Stage-owned prim/body
bridge and gameplay or host policy.
