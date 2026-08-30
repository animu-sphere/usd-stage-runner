# Runtime Model

Status: intended contract; input and transform portions implemented

## State ownership

The USD Stage and Runtime World own different kinds of state.

| Owner | State |
| --- | --- |
| USD Stage | Hierarchy, composition, references, payloads, variants, authored configuration, persistent properties, and stable prim identity. |
| Runtime World | Per-frame input, velocities, backend handles, character and vehicle state, behavior state, camera smoothing, and dirty synchronization state. |
| OpenExec | Dependency evaluation over runtime-facing interfaces, not domain state or the host loop. |
| Physics backend | Collision, constraints, and simulation behind `physicsCore` contracts. |

High-frequency state must not travel between runtime systems by repeatedly
reading and authoring USD attributes. The Stage is the source of truth for scene
definition; the Runtime World is the source of truth for live state between
synchronization points.

## Prim identity and components

A USD prim path is also the runtime entity identity. For example,
`/World/Player` can own runtime transform, physics-body, character-controller,
and behavior components without introducing another hierarchy.

```text
/World/Player
    RuntimeTransform
    PhysicsBody
    CharacterController
    Behavior
```

USD API schemas declare which components should be created. Runtime components
are the live instances of those declarations. USD hierarchy and composition
remain the only scene hierarchy.

## Input and intent

Physical devices are normalized into named actions before controller logic
creates domain intent.

```text
keyboard / gamepad                  behavior / AI
          |                              |
          v                              v
     named actions --------------> controller intent
                                      |
                                      v
                         character or vehicle system
```

Initial character intent carries desired velocity, facing direction, and jump
state. Vehicle intent carries throttle, brake, steering, and handbrake. Human
input and AI produce the same intent contracts so their physics implementation
is shared.

## Frame execution

The target host-frame order is:

```text
poll devices
    -> update normalized input actions
    -> update player and AI intent
    -> evaluate behavior and OpenExec
    -> apply motion commands and forces
    -> advance zero or more bounded fixed physics steps
    -> extract simulation transforms
    -> mark runtime components dirty
    -> synchronize dirty state to USD
    -> evaluate camera rigs
    -> render
```

Host and render frames may use variable time. Physics starts at 60 Hz
(`dt = 1 / 60`) and uses the existing bounded accumulator. A controlled clock
must be injectable so the same path can be tested without wall-clock sleeps.

The current implementation stops after direct movement updates the runtime
transform. The [Jolt milestone](../roadmap/current.md) replaces that temporary
movement step with physics-driven motion.

## Domain systems

### Characters

Characters translate `CharacterIntent` into physics operations. Their runtime
state includes velocity, grounded state, jump state, facing, and support body.
They do not move by directly editing authored transforms.

### Camera rigs

A USD Camera prim represents a camera in the Stage. A runtime rig calculates its
live transform from a prim-path target, optional anchor, smoothing, spring,
damping, and collision avoidance. Initial modes are free, first-person,
third-person, and orbit; vehicle chase, cockpit, and cinematic modes follow.

### Vehicles

Vehicles are composed systems rather than a monolithic car component:

```text
Vehicle
|- Chassis
|- Wheel[]
|- Suspension[]
|- Steering
`- Drivetrain
```

The composition must support different wheel counts, trailers, motorcycles,
and unusual vehicles without hard-coding a four-wheel layout.

### Behavior and OpenExec

`behaviorCore` owns stateful behavior-tree evaluation with initial `Sequence`,
`Selector`, `Condition`, and `Action` nodes. OpenExec provides dependency and
calculation graphs over runtime-facing interfaces. Neither OpenExec evaluation
state nor an Exec node replaces behavior state or domain implementations.
