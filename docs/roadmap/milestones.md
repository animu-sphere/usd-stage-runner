# Planned Milestones

Status: not started unless noted otherwise

Milestone 3 is in progress; its backend-neutral character core and Jolt
ground-query adapter are implemented. The remaining milestones have not
started.

Work is ordered so every milestone adds a runnable, testable capability. The
[character-controller vertical slice](current.md) is the current milestone.
Physics and the initial Runner physics schemas are already implemented and are
recorded in the [current architecture](../architecture/overview.md), not as
open roadmap work.

## Delivery order

| Milestone | Capability | Proof |
| --- | --- | --- |
| 3 | Character controller | A Stage-declared character moves, grounds, and jumps through physics. |
| 4 | Camera rigs | First- and third-person modes follow the controlled character. |
| 5 | Host integration | The same Stage can play safely through `stage_runner`, usdview, and OST Plugin View. |
| 6 | Vehicle composition | A USD-composed four-wheel vehicle is drivable without a four-wheel-only runtime contract. |
| 7 | Behavior runtime | An AI character produces the same intent contract as a player. |
| 8 | OpenExec integration | Small Exec nodes invoke operations already exposed by core libraries. |
| 9 | Animation integration | Runtime motion drives a USD Skeleton and animation state without redefining character control around one asset format. |
| 10 | Runtime tooling | Hosts can inspect, debug, profile, and explicitly bake runtime state. |

Character and camera form the next uninterrupted implementation path. Host
integration then proves that those systems are reusable libraries rather than
features embedded in the standalone executable.

## Milestone 3: Character controller

Targets: `libs/characterCore`, `RunnerCharacterAPI`, and required
backend-neutral character capabilities in `physicsCore`

- Add `CharacterIntent` with desired velocity, facing direction, and jump.
- Track velocity, grounded state, jump state, facing, and support body.
- Implement ground detection, movement, basic slope handling, and jumping
  through `physicsCore`, with Jolt-specific code confined to `physicsJolt`.
- Accept the same intent contract from human input, injected tests, and later
  AI systems.
- Map keyboard and gamepad actions through `inputSdl` rather than reading
  device buttons in character code.
- Add a deterministic `character_walk.usda` scenario.

Success: a Stage-declared character can walk, ground, and jump without direct
transform edits or a dependency on a particular visual representation.

## Milestone 4: Camera rigs

Targets: `libs/cameraCore`, followed by `RunnerCameraRigAPI`

- Refer to a target and optional anchor by prim identity.
- Define rig mode, offset, distance, pitch, yaw, damping, and collision-probe
  configuration as declarations.
- Implement free, first-person, third-person, and orbit modes in reusable
  runtime code.
- Calculate the desired transform, perform an optional physics probe, smooth
  the result, and synchronize it to a `UsdGeomCamera`.
- Add collision avoidance incrementally without coupling `cameraCore` to Jolt.
- Add a `third_person_camera.usda` scenario.

Success: first- and third-person modes can be switched while following the
controlled character, with repeatable behavior under a controlled clock.

## Milestone 5: Host integration

Targets: `plugins/usdviewStageRunner` and OST Plugin View integration

- Reuse the same Runtime World and subsystem libraries as `stage_runner`.
- Keep host adapters limited to lifecycle, frame driving, UI, rendering hooks,
  and Stage access.
- Provide play, pause, single-step, and reset controls first.
- Put simulated values in a discardable session/runtime layer.
- Discard the layer on reset or stop; defer authored-layer persistence to an
  explicit bake or commit operation.
- Verify equivalent fixed-step and synchronization semantics in each host.

Success: a Stage that runs through `stage_runner` can be played in usdview and
OST Plugin View without duplicating simulation logic or contaminating its root
layer.

At this point the representative demo is:

```text
open a USD Stage
    -> press Play
    -> control a character with keyboard or gamepad
    -> move and collide through Jolt
    -> follow it with a third-person camera
    -> observe live Stage updates in the selected host
```

## Milestone 6: Vehicle composition

Targets: `libs/vehicleCore`, then focused schema declarations

- Compose chassis, wheels, suspension, steering, powertrain, and braking rather
  than introducing a monolithic car object.
- Add `VehicleIntent` for throttle, brake, steering, and handbrake.
- Begin with `RunnerVehicleAPI` and `RunnerWheelAPI`; add narrower APIs when the
  implementation needs them.
- Reuse USD hierarchy, references, payloads, inheritance, and variants for
  vehicle assembly.
- Keep the runtime compatible with other wheel counts and layouts.
- Add a deterministic `four_wheel_vehicle.usda` scenario.

Success: a USD-composed four-wheel vehicle is drivable with normalized input,
while its controller and physics path remain reusable by other vehicle forms.

## Milestone 7: Behavior runtime

Targets: `libs/behaviorCore`, followed by `RunnerBehaviorAPI`

- Add stateful `Sequence`, `Selector`, `Condition`, `Action`, and `Decorator`
  nodes.
- Keep `BehaviorInstance`, blackboard values, and node state in the Runtime
  World rather than forcing them into the USD prim hierarchy.
- Let USD declarations refer to behavior assets without making the behavior
  tree mirror the scene hierarchy.
- Produce the same character and vehicle intent contracts used by human input.
- Add a chase, guard, or wander scenario in `behavior_chase.usda`.

Success: an AI-controlled character moves autonomously through the same
controller and physics path as a player.

## Milestone 8: OpenExec integration

Targets: `plugins/execRunner`, `execPhysics`, `execCharacter`, `execVehicle`,
and `execBehavior` as required by proven use cases

- Start with thin nodes such as `ReadInputAction`, `ReadRuntimeTransform`,
  `ReadVelocity`, `MoveCharacter`, `ApplyForce`, `SetCameraTarget`,
  `DriveVehicle`, and `TickBehavior`.
- Route every node through a documented core interface.
- Keep host orchestration, behavior state, and domain algorithms outside node
  implementations.
- Preserve C++, Python, host UI, and test access to the same operations without
  requiring OpenExec.

Success: OpenExec evaluation reads runtime state and changes intent or invokes
an operation through the same public boundary used by non-Exec code.

## Milestone 9: Animation integration

Targets are selected only after the character and host slices expose a concrete
animation use case.

- Connect character state to USD Skeleton and animation playback.
- Add the smallest useful locomotion-state and blending contract.
- Keep retargeting, IK, facial expression, and format-specific features out of
  the initial slice.
- Treat VRM as one visual representation rather than the identity of the
  character runtime.

Success: the representative character visibly reflects its runtime locomotion
state without coupling `characterCore` to one asset format.

## Milestone 10: Runtime tooling

- Inspect prim-indexed runtime components and subsystem state.
- Visualize colliders, contacts, velocity, grounded state, suspension, camera
  probes, and behavior state through host-rendered diagnostics.
- Add bounded timing and profiling data without putting UI in core libraries.
- Add explicit simulation bake and selected-property commit workflows.

Success: a developer can explain a running Stage's state, locate subsystem
costs, and deliberately persist selected results.

## Cross-cutting follow-up

- Incremental USD-to-runtime updates using Stage change notices.
- CI checks for forbidden dependency edges.
- Additional input and physics backends.
- Richer camera collision, rig blending, and cinematic modes.
- Vehicle variants such as motorcycles, trailers, and tracked vehicles.
- Packaging, compatibility policy, release records, task guides, and generated
  API/schema reference once real usage requires them.

The initial non-goals in [concepts/overview.md](../concepts/overview.md) remain
out of scope until a completed vertical slice demonstrates a concrete need.
