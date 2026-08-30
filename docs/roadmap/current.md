# Milestone 1: Jolt Physics Vertical Slice

Status: implemented

The `physicsJolt` bootstrap, backend-neutral Runtime integration, temporary
Stage import, and complete input-to-physics path are implemented.

The input-and-transform slice is implemented and recorded in the
[architecture overview](../architecture/overview.md). This milestone replaces
temporary direct transform movement with physics-driven motion while preserving
the existing fixed-step and dirty-synchronization foundations.

## Outcome

```text
move actions
    -> movement intent
    -> force or desired motion
    -> bounded Jolt fixed step
    -> runtime transform
    -> dirty queue
    -> USD xform update
```

The representative Stage contains `/World/Ground`, `/World/PlayerCube`, and
`/World/Camera`. The cube falls under gravity, rests on the ground, and responds
to input through physics.

## Scope

### `physicsJolt`

- Initialize and shut down Jolt.
- Create box shapes and static or dynamic bodies.
- Define the first collision layers.
- Advance fixed simulation steps.
- Extract changed body transforms.
- Own Jolt objects and body lifetime without exposing Jolt types publicly.

### Runtime and Stage integration

- Map `PrimId` values to physics bodies.
- Import the ground and player body through a temporary Stage convention.
- Feed movement intent to force or desired-motion commands.
- Update and dirty `RuntimeTransform` only when simulation results change.
- Synchronize only dirty body transforms to USD.

## Recommended PR sequence

1. **`physicsJolt` bootstrap** — implemented: initialization, box bodies, a
   static floor, fixed stepping, changed-state extraction, and adapter coverage.
2. **Runtime physics integration** — implemented: prim/body mapping, transform
   extraction, and dirty synchronization.
3. **Stage importer** — implemented: bodies and colliders are created from an
   explicitly temporary Cube attribute convention.
4. **Full vertical slice** — implemented: input sets desired horizontal body
   velocity before Jolt stepping, and changed transforms flow back through the
   dirty USD synchronization queue.

The following schema work is deliberately a separate milestone so the asset
contract is based on a proven runtime path.

## Completion criteria

- The cube falls under gravity and settles on the floor.
- Keyboard, gamepad, or injected movement input moves it through physics.
- Fixed-step results are repeatable with a controlled clock.
- Runtime-to-USD writes contain only dirty simulated transforms.
- Unit, Jolt adapter, and Stage integration tests cover the slice.
- Plain CMake and OpenStrata build paths continue to work in their supported
  environments.

The relevant long-term contracts are documented in the
[runtime model](../design/runtime-model.md),
[module boundaries](../design/modules.md), and
[testing strategy](../design/testing.md).
