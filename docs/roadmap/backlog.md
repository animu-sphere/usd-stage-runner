# Backlog

Status: ⬜ not started unless noted otherwise

Work is ordered as vertical slices. Each phase should result in a runnable,
testable capability rather than only a collection of disconnected libraries.

## Phase 3: Character controller

- Add desired velocity, ground state, jump state, and facing direction.
- Translate human or AI intent through a shared controller interface.
- Implement ground detection, movement, and jumping through physics.

Success: a character can be controlled in a small Stage.

## Phase 4: Camera rigs

- Add target relations and runtime camera components.
- Implement first-person, third-person, and chase modes.
- Add smoothing and collision avoidance incrementally.

Success: first- and third-person modes can be switched during control.

## Phase 5: OpenExec integration

- Add small nodes for input actions, transforms, velocity, and controller intent.
- Keep state and domain behavior in core runtime libraries.
- Demonstrate an `input -> OpenExec -> runtime component` path.

Success: OpenExec evaluation changes a runtime component through a documented
public boundary.

## Phase 6: Vehicle composition

- Add chassis, wheel, steering, drivetrain, brake, and suspension components.
- Add focused USD API schemas and a Jolt-backed implementation.
- Build a four-wheel demo without hard-coding the runtime to four wheels.

Success: a vehicle assembled in USD can be driven with a gamepad.

## Phase 7: Behavior

- Add `behaviorCore` with Sequence, Selector, Condition, and Action nodes.
- Preserve state independently from OpenExec evaluation semantics.
- Add thin OpenExec wrappers and an autonomous navigation example.

Success: an AI-controlled character moves autonomously in the Stage.

## Unscheduled

- authored-value change notices and incremental runtime rebuilds;
- additional input and physics backends;
- richer camera collision and rig blending;
- vehicle variants such as motorcycles and trailers;
- packaging, compatibility, and release records; and
- task-oriented guides and generated API/schema reference.

The initial non-goals in [concepts/overview.md](../concepts/overview.md) remain
out of scope until the vertical slices demonstrate a concrete need.
