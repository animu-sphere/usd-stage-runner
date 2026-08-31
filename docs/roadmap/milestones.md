# Planned Milestones

Status: not started unless noted otherwise

Work is ordered so every milestone adds a runnable, testable capability. The
[character-controller vertical slice](current.md) is the current milestone.

## Delivery order

| Milestone | Capability | Proof |
| --- | --- | --- |
| 3 | Character controller | A character moves, grounds, and jumps through physics. |
| 4 | Camera rigs | First- and third-person modes follow a runtime target. |
| 5 | Vehicle composition | A USD-composed four-wheel vehicle is drivable without a four-wheel-only runtime contract. |
| 6 | Behavior runtime | An AI character produces the same intent contract as a player. |
| 7 | OpenExec integration | Small Exec nodes read runtime state and set intent through core interfaces. |

## Milestone 3: Character controller

Targets: `libs/characterCore`, followed by `RunnerCharacterAPI`

- Add `CharacterIntent` with desired velocity, facing direction, and jump.
- Track velocity, grounded state, jump state, facing, and support body.
- Implement ground detection, movement, basic slope handling, and jumping
  through `physicsCore`.
- Accept the same intent contract from human input and AI.
- Add a deterministic `character_walk.usda` scenario.

Success: a character can walk and jump in a small Stage without direct
transform edits.

## Milestone 4: Camera rigs

Targets: `libs/cameraCore`, followed by `RunnerCameraRigAPI`

- Refer to targets and optional anchors by prim path.
- Implement free, first-person, third-person, and orbit modes.
- Add runtime smoothing, spring, and damping state.
- Add collision avoidance incrementally.
- Synchronize the result to a USD Camera prim.
- Add a `third_person_camera.usda` scenario.

Success: first- and third-person modes can be switched while following the
controlled character.

## Milestone 5: Vehicle composition

Targets: `libs/vehicleCore`, then focused schema declarations

- Compose chassis, wheels, suspension, steering, drivetrain, and braking.
- Add `VehicleIntent` for throttle, brake, steering, and handbrake.
- Begin with `RunnerVehicleAPI` and `RunnerWheelAPI`; add narrower APIs when the
  implementation needs them.
- Reuse USD hierarchy, references, and variants for vehicle assembly.
- Add a deterministic `four_wheel_vehicle.usda` scenario.

Success: a USD-composed vehicle is drivable with normalized input, while the
runtime design remains compatible with other wheel counts and layouts.

## Milestone 6: Behavior runtime

Targets: `libs/behaviorCore`

- Add stateful `Sequence`, `Selector`, `Condition`, and `Action` nodes.
- Keep behavior state in the Runtime World.
- Produce the same character and vehicle intent contracts used by human input.
- Add a chase or wander scenario in `behavior_chase.usda`.

Success: an AI-controlled character moves autonomously through the same
controller and physics path as a player.

## Milestone 7: OpenExec integration

Targets: `plugins/execRunner`, `execPhysics`, `execBehavior`, and `execVehicle`
as required by proven use cases

- Start with small nodes such as `ReadInputAction`, `ReadRuntimeTransform`,
  `ReadVelocity`, `SetCharacterIntent`, `SetVehicleIntent`, `ApplyForce`, and
  `ReadGroundState`.
- Route every node through a documented core interface.
- Keep host orchestration, behavior state, and domain algorithms outside node
  implementations.

Success: OpenExec evaluation reads runtime state and changes an intent or
component through the same public boundary used by non-Exec code.

## Platform integration tracks

These tracks reuse the runtime milestones rather than introducing alternate
implementations.

### usdview prototype and plugin

- A minimal prototype may be scheduled after physics to prove host independence.
- Full integration adds play, pause, stop, frame tick, camera selection, runtime
  settings, and debug visualization.
- Simulation values must use a discardable session/runtime layer before the
  plugin is treated as an authoring-safe workflow.

### OST Plugin View

- Keep OpenStrata responsible for environment, dependency pinning, build, test,
  packaging, CI, and host integration.
- Load the ordinary runtime libraries from OST Plugin View without introducing
  OpenStrata APIs into core targets.

## Cross-cutting follow-up

- Incremental USD-to-runtime updates using Stage change notices.
- Play-session discard, bake, and selected-property commit workflows.
- Host-rendered diagnostics for colliders, contacts, velocity, ground state,
  suspension, cameras, and behavior.
- CI checks for forbidden dependency edges.
- Additional input and physics backends.
- Richer camera collision, rig blending, and cinematic modes.
- Vehicle variants such as motorcycles, trailers, and tracked vehicles.
- Packaging, compatibility policy, release records, task guides, and generated
  API/schema reference once real usage requires them.

The initial non-goals in [concepts/overview.md](../concepts/overview.md) remain
out of scope until a completed vertical slice demonstrates a concrete need.
