# Milestone 1: Jolt Physics Vertical Slice

Status: in progress; `physicsCore` skeleton implemented

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

### `physicsCore`

- Add minimal world, body, shape, and constraint contracts.
- Use backend-neutral handles and descriptors.
- Add a deterministic mock backend or equivalent contract test.
- Depend on `runtimeCore`, not OpenUSD, OpenExec, or Jolt.

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

1. **`physicsCore` skeleton** — implemented: contracts, typed handles,
   descriptors, validation, and mock-backed deterministic tests.
2. **`physicsJolt` bootstrap** — initialization, one box body, a static floor,
   and fixed stepping.
3. **Runtime physics integration** — prim/body mapping, transform extraction,
   and dirty synchronization.
4. **Stage importer** — create bodies and colliders through an explicitly
   temporary convention.
5. **Full vertical slice** — connect input to physics and verify the complete
   path through USD.

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
