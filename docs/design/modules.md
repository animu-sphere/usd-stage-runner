# Modules and Dependency Boundaries

Status: intended contract; `runtimeCore`, `inputCore`, `physicsCore`,
`characterCore`, `cameraCore`, `stageRuntime`, `inputSdl`, `physicsJolt`, and
the physics, character, and camera contracts in `runnerSchema` implemented

## Core and adapter rule

Backend-neutral behavior lives in ordinary C++ core libraries. SDK-specific
code lives in adapters or plugins.

```text
inputCore    <- inputSdl
physicsCore  <- physicsJolt
characterCore <- host / execCharacter
cameraCore    <- host / execRunner
stageRuntime  <- host adapters
behaviorCore  <- host / execBehavior
vehicleCore   <- host / execVehicle
```

Public core APIs must not expose SDL, Jolt, OpenExec, or unnecessary OpenUSD
types. Backend-specific capabilities are explicit extensions rather than leaks
through a supposedly universal interface.

The first `physicsCore` abstraction stays deliberately small:

```text
PhysicsWorld
PhysicsBody
PhysicsShape
PhysicsConstraint
```

## Target responsibilities

| Area | Core responsibility | Adapter or plugin responsibility |
| --- | --- | --- |
| Input | Named actions and controller intent. | SDL device polling and mapping. |
| Physics | Backend-neutral worlds, bodies, shapes, constraints, commands, and optional queries. | Jolt initialization, object lifetime, layers, stepping, queries, and transform extraction. |
| Character | Character intent and controller state. | Physics operations through `physicsCore`. |
| Camera | Targeting, rig modes, smoothing, and probes. | `stageRuntime` USD synchronization and host rendering. |
| Vehicle | Chassis, wheel, suspension, steering, and drivetrain composition. | Backend capabilities through `physicsCore`; thin Exec wrappers. |
| Behavior | Stateful behavior-tree evaluation. | Thin host and OpenExec invocation. |
| Schema | No runtime implementation. | Applied OpenUSD API schema definitions and parsing. |
| Stage session | Prim-indexed system import, play-session execution, reset/rebuild, and dirty USD synchronization. | Host Stage acquisition, clock driving, UI, and backend selection. |

## Allowed dependency direction

```text
runtimeCore

inputCore      -> runtimeCore
physicsCore    -> runtimeCore
characterCore  -> runtimeCore + physicsCore
cameraCore     -> runtimeCore
stageRuntime   -> runtimeCore + inputCore + physicsCore + characterCore + cameraCore + OpenUSD
behaviorCore   -> runtimeCore
vehicleCore    -> runtimeCore + physicsCore

inputSdl       -> inputCore + SDL
physicsJolt    -> physicsCore + Jolt
runnerSchema   -> OpenUSD
exec*          -> relevant core + runnerSchema + OpenExec
stage_runner   -> stageRuntime + inputSdl + physicsJolt + OpenUSD plug
```

Forbidden edges include:

```text
runtimeCore  -/-> OpenExec, Jolt, or SDL
physicsCore  -/-> OpenUSD or Jolt
characterCore -/-> Jolt
vehicleCore  -/-> OpenExec
behaviorCore -/-> OpenExec
cameraCore   -/-> OpenExec or Jolt
stageRuntime -/-> OpenExec, Jolt, or SDL
```

The CMake target graph and CI should enforce these boundaries as targets are
introduced.

## Intended repository topology

```text
usd-stage-runner/
|- CMakeLists.txt
|- CMakePresets.json
|- openstrata.toml
|- openstrata.ci.yaml
|- docs/
|- libs/
|  |- runtimeCore/
|  |- inputCore/
|  |- physicsCore/
|  |- characterCore/
|  |- cameraCore/
|  |- stageRuntime/
|  |- behaviorCore/
|  `- vehicleCore/
|- backends/
|  |- inputSdl/
|  `- physicsJolt/
|- plugins/
|  |- runnerSchema/
|  |- execRunner/
|  |- execPhysics/
|  |- execCharacter/
|  |- execBehavior/
|  |- execVehicle/
|  `- usdviewStageRunner/
|- apps/stage_runner/
|- examples/
|  |- character/
|  |- vehicle/
|  |- behavior/
|  `- camera/
|- tests/
`- third_party/
```

Directories are added with the vertical slice that needs them; empty
placeholders are not required.
