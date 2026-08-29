# Runtime Design Specification

Status: canonical, pre-implementation

This document is the source of truth for the intended `usd-stage-runner`
runtime. It defines the destination architecture. The
[architecture documentation](../architecture/) separately records what is
implemented today.

## 1. Purpose

`usd-stage-runner` treats an OpenUSD Stage as an interactive real-time world.
The project connects USD composition to runtime input, behavior, physics,
camera, and OpenExec evaluation while keeping those systems loosely coupled.

The application is a runtime host, not a replacement for a complete game
engine. Its primary question is whether game-like interaction can be built
around OpenUSD without collapsing scene definition and transient simulation
state into the same layer.

## 2. Sources of authority

Each layer owns a distinct kind of state:

| Layer | Owns |
| --- | --- |
| USD Stage | Structure, composition, authored configuration, references, persistence, and stable prim identity. |
| Runtime World | Per-frame component instances, clocks, input state, physics handles, behavior state, camera state, and synchronization queues. |
| OpenExec | Evaluation order, dependencies, and graph-based calculation over runtime-facing interfaces. |
| Physics backend | Collision, rigid bodies, constraints, characters, and vehicle simulation. |

The Stage is authoritative for scene definition. The Runtime World is
authoritative for live simulation state between synchronization points. High
frequency state must not be forced through authored USD values merely to move it
between runtime systems.

## 3. Runtime World

Opening a Stage builds a Runtime World by scanning prim declarations and
creating the corresponding runtime component instances. The world owns:

- prim-to-runtime identity mapping;
- a runtime component registry;
- variable frame time and fixed simulation time;
- normalized input action state;
- physics, behavior, character, vehicle, and camera state; and
- dirty queues for Stage-to-runtime and runtime-to-Stage synchronization.

A USD prim serves as the entity identity. API schemas declare components, and
runtime components realize those declarations. The runtime must not create a
second independently parented entity hierarchy.

## 4. Frame execution

The host application follows this logical order:

```text
poll devices
    -> update normalized input actions
    -> evaluate controllers, behavior, and OpenExec graphs
    -> apply desired motion, forces, and commands
    -> advance fixed physics steps
    -> update runtime transforms
    -> synchronize dirty runtime state to USD
    -> evaluate cameras
    -> render
```

Rendering may use a variable timestep. Physics uses a fixed timestep, initially
60 Hz unless a host configuration selects another rate. The host accumulates
render time and may perform zero or more bounded physics steps per frame.

Synchronization is incremental. Runtime-to-USD writes only dirty component
state. Authored Stage changes flow in the opposite direction and update or
rebuild affected runtime components. Stage change notices may provide the later
incremental implementation, but are not required for the first slice.

## 5. Core and adapter rule

Reusable behavior is implemented first as ordinary C++ libraries. OpenUSD,
OpenExec, physics SDKs, and input SDKs are integrated through adapters:

```text
inputCore    <- inputSdl       <- host or execInput
physicsCore  <- physicsJolt    <- host or execPhysics
vehicleCore  <- physicsCore    <- execVehicle
behaviorCore <- runtimeCore    <- execBehavior
cameraCore   <- runtimeCore    <- host or execRunner
```

An OpenExec node is a thin wrapper over a runtime-facing interface. It must not
contain the only implementation of controller, vehicle, camera, or behavior
logic. Concrete Jolt and SDL types must not appear in public core APIs.

The initial physics abstraction should remain deliberately small:

```text
PhysicsWorld
PhysicsBody
PhysicsShape
PhysicsConstraint
```

Backend-specific capabilities can be added as explicit extensions rather than
prematurely expanding a universal abstraction.

## 6. Input and intent

Device input is normalized into named actions before it reaches controllers:

```text
gamepad / keyboard / network / AI
                  -> input action
                  -> controller intent
                  -> runtime system
```

Initial action names include `move.x`, `move.y`, `look.x`, `look.y`, `jump`,
`accelerate`, `brake`, `steer`, and `camera.toggle`. Stage declarations and
behaviors consume actions, not physical button or axis identifiers.

Human input and AI behavior produce the same controller intent structures. A
character, for example, receives desired velocity, facing direction, and jump
intent rather than direct transform edits.

## 7. Physics and characters

Jolt Physics is the initial backend candidate. `physicsCore` exposes only the
runtime contract; `physicsJolt` owns Jolt object creation, stepping, and result
extraction.

A character controller is not modeled as arbitrary rigid-body transform edits.
It owns desired velocity, ground state, jump state, and facing direction, and it
translates intent into physics operations.

```text
input or AI -> CharacterIntent -> CharacterController -> physics
```

## 8. Vehicle composition

A vehicle is composed from multiple Stage declarations and runtime components,
not represented by one monolithic component:

```text
VehicleComponent
|- Chassis
|- Wheel[]
|- Suspension[]
|- Steering
`- Drivetrain
```

Candidate API schemas include `RunnerVehicleAPI`, `RunnerWheelAPI`, and focused
drive, steering, and suspension declarations. Composition must support different
wheel counts, drive layouts, trailers, motorcycles, and nonstandard vehicles
without redefining the entire vehicle type.

## 9. Behavior and OpenExec

OpenExec evaluates runtime system graphs; it does not replace the host loop.
Initial nodes should be small operations such as reading an input action,
reading a transform or velocity, and setting controller intent.

Behavior trees remain a stateful runtime library with nodes such as `Sequence`,
`Selector`, `Condition`, and `Action`. OpenExec wrappers may invoke that library,
but OpenExec evaluation semantics do not own behavior-tree state.

## 10. Camera rigs

The camera uses a USD Camera prim for scene representation and a runtime camera
rig for follow behavior. A rig refers to a target prim and may implement free,
first-person, third-person, orbit, or vehicle-chase modes.

```text
target transform
    -> desired camera transform
    -> spring and damping
    -> collision avoidance
    -> USD camera synchronization
```

First-person rigs may target an anchor prim below a character or vehicle. The
same camera system must work across both kinds of target.

## 11. USD schema strategy

Early schemas should favor applied API schemas over a large typed-schema
hierarchy. Candidate contracts are:

- `RunnerPhysicsBodyAPI` and `RunnerColliderAPI`;
- `RunnerCharacterAPI`;
- `RunnerVehicleAPI` and `RunnerWheelAPI`;
- `RunnerInputAPI`;
- `RunnerBehaviorAPI`; and
- `RunnerCameraRigAPI`.

Schemas are declarative runtime contracts. They must not expose private runtime
representations or require OpenExec to understand importer internals.

## 12. Repository topology

The intended top-level structure is:

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
|  |- behaviorCore/
|  |- cameraCore/
|  `- vehicleCore/
|- backends/
|  |- physicsJolt/
|  `- inputSdl/
|- plugins/
|  |- runnerSchema/
|  |- execRunner/
|  |- execPhysics/
|  |- execBehavior/
|  `- execVehicle/
|- apps/stage_runner/
|- examples/
|- tests/
`- third_party/
```

OpenStrata provides reproducible workspace, build, dependency, test, package,
and CI infrastructure. The native project remains buildable with CMake, and
runtime implementation does not depend on OpenStrata APIs.

## 13. Dependency rules

Allowed direction:

```text
runtimeCore  -> minimal standard dependencies
physicsCore  -> runtimeCore
physicsJolt  -> physicsCore + Jolt
inputCore    -> runtimeCore
inputSdl     -> inputCore + SDL
vehicleCore  -> runtimeCore + physicsCore
cameraCore   -> runtimeCore
behaviorCore -> runtimeCore
runnerSchema -> OpenUSD
exec*        -> relevant core + runnerSchema + OpenExec
stage_runner -> composed runtime systems
```

Forbidden direction:

```text
runtimeCore  -/-> OpenExec or Jolt
physicsCore  -/-> OpenUSD
vehicleCore  -/-> OpenExec
behaviorCore -/-> OpenExec
```

Build and architecture checks should enforce these edges once the targets
exist.

## 14. Stage runner host

The first host executable remains small:

```text
stage_runner scene.usda --physics jolt --fixed-dt 0.0166667 --camera /World/Camera
```

Its responsibilities are to open the Stage, scan schema declarations, build the
Runtime World, initialize OpenExec and selected backends, run the frame loop,
and coordinate synchronization. Domain behavior belongs in libraries rather
than in command-line handling or the main loop.

## 15. Initial vertical slice

The first representative Stage contains `/World/Ground`,
`/World/PlayerCube`, and `/World/Camera`. The complete path to prove is:

```text
gamepad
 -> input action
 -> controller intent
 -> physics force
 -> Jolt step
 -> runtime transform
 -> USD synchronization
 -> following camera
```

The phased delivery plan and success conditions are maintained in
[roadmap/](../roadmap/).

## 16. Invariants

1. USD describes the scene; Runtime World owns transient simulation state.
2. OpenExec nodes are wrappers, not the only home of runtime logic.
3. Concrete backend types do not leak through public core interfaces.
4. Physical input devices are separated from named actions and intent.
5. Prim identity anchors runtime components; there is no second scene hierarchy.
6. USD composition is reused for prefab-like composition.
7. Dependencies point from adapters toward core contracts.
