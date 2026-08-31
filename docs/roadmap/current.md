# Milestone 3: Character Controller

Status: not started

The physics schemas and complete input-to-physics-to-USD path are implemented.
This milestone adds a backend-neutral character contract that moves, grounds,
and jumps through `physicsCore` instead of editing transforms directly.

## Outcome

```text
input or AI
    -> CharacterIntent
    -> character controller state
    -> physics commands and ground queries
    -> RuntimeTransform
    -> dirty queue
    -> USD xform update
```

The representative Stage adds a character that can walk, ground, and jump in a
small deterministic scenario. Human input and later behavior systems use the
same intent boundary.

## Scope

### `characterCore`

- Add `CharacterIntent` with desired velocity, facing direction, and jump.
- Track velocity, grounded state, jump state, facing, and support body.
- Implement ground detection, movement, basic slope handling, and jumping
  through backend-neutral `physicsCore` contracts, with Jolt-specific character
  support confined to `physicsJolt`.
- Keep controller logic testable without Jolt or OpenUSD.

### Runtime and Stage integration

- Add `RunnerCharacterAPI` only with the vertical slice that consumes it.
- Import character declarations into prim-indexed runtime components.
- Feed both movement input and deterministic injected intent through the same
  controller path.
- Map keyboard and gamepad controls to named actions; character code must not
  inspect SDL identifiers or button enums.
- Preserve fixed-step ordering and dirty-only USD synchronization.
- Add `character_walk.usda` as the runnable golden scenario.

## Recommended PR sequence

1. **Character core bootstrap** — intent, state, and isolated controller tests
   using a deterministic physics test double.
2. **Jolt character adapter** — add only the `physicsCore` capabilities needed
   for grounding, desired motion, slope limits, and jumping, then implement
   them in `physicsJolt`.
3. **Schema and importer** — `RunnerCharacterAPI` plus Stage-to-runtime mapping.
4. **Gamepad vertical slice** — SDL and injected actions drive the character in
   `character_walk.usda`, with runtime transforms synchronized incrementally.

## Completion criteria

- A Stage-declared character walks, grounds, and jumps without direct transform
  movement.
- Human input and deterministic injected intent share one contract.
- Keyboard and the first supported gamepad share named gameplay actions.
- Core tests require neither OpenUSD nor Jolt.
- Fixed-step results are repeatable under a controlled clock.
- Runtime-to-USD writes contain only dirty simulated transforms.
- Plain CMake and OpenStrata build paths remain valid in supported environments.

The relevant long-term contracts are documented in the
[runtime model](../design/runtime-model.md),
[input actions and intent](../design/input.md),
[module boundaries](../design/modules.md),
[USD integration](../design/usd-integration.md), and
[testing strategy](../design/testing.md).
