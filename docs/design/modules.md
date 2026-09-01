# Modules and Dependency Boundaries

Status: intended contract; `runtimeCore`, `inputCore`, `physicsCore`,
`characterCore`, `cameraCore`, `inputSdl`, `physicsJolt`, and the physics,
character, and camera contracts in `runnerSchema` implemented

## Core and adapter rule

Backend-neutral behavior lives in ordinary C++ core libraries. SDK-specific
code lives in adapters or plugins.

```text
inputCore    <- inputSdl
physicsCore  <- physicsJolt
characterCore <- host / execCharacter
cameraCore    <- host / execRunner
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
| Camera | Targeting, rig modes, smoothing, and probes. | Host rendering and USD camera synchronization. |
| Vehicle | Chassis, wheel, suspension, steering, and drivetrain composition. | Backend capabilities through `physicsCore`; thin Exec wrappers. |
| Behavior | Stateful behavior-tree evaluation. | Thin host and OpenExec invocation. |
| Schema | No runtime implementation. | Applied OpenUSD API schema definitions and parsing. |

## Allowed dependency direction

```text
runtimeCore

inputCore      -> runtimeCore
physicsCore    -> runtimeCore
characterCore  -> runtimeCore + physicsCore
cameraCore     -> runtimeCore
behaviorCore   -> runtimeCore
vehicleCore    -> runtimeCore + physicsCore

inputSdl       -> inputCore + SDL
physicsJolt    -> physicsCore + Jolt
runnerSchema   -> OpenUSD
exec*          -> relevant core + runnerSchema + OpenExec
stage_runner   -> composed runtime libraries
```

Forbidden edges include:

```text
runtimeCore  -/-> OpenExec, Jolt, or SDL
physicsCore  -/-> OpenUSD or Jolt
characterCore -/-> Jolt
vehicleCore  -/-> OpenExec
behaviorCore -/-> OpenExec
cameraCore   -/-> OpenExec or Jolt
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
