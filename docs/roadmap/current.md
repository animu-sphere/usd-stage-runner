# Next Milestone: Jolt Physics

Status: ⬜ not started

The input and transform slice is implemented and recorded in the
[architecture overview](../architecture/overview.md). The next vertical slice
introduces backend-neutral physics contracts and a Jolt adapter, then replaces
direct movement integration with force-based motion.

## Scope

- add minimal `physicsCore` interfaces for worlds, bodies, shapes, and
  constraints;
- add a `physicsJolt` adapter without leaking Jolt types into core APIs;
- build rigid bodies and colliders from the representative Stage;
- advance physics through the existing bounded fixed-step accumulator;
- extract changed body transforms into runtime transform components;
- synchronize dirty simulated transforms to USD; and
- connect movement intent to force-based player movement.

## Required boundaries

- `physicsCore` depends inward on `runtimeCore` and does not include OpenUSD,
  OpenExec, or Jolt types.
- `physicsJolt` owns Jolt object lifetime and implements the core contracts.
- USD declarations and writes stay in the host or a dedicated adapter.
- Runtime bodies remain keyed by existing prim identity.
- Physics and controller tests use deterministic fixed steps without wall-clock
  sleeps.

## Completion criteria

```text
move actions
    -> movement intent
    -> physics force
    -> fixed Jolt step
    -> dirty runtime transform
    -> USD xform update
```

The representative Stage must contain a cube that falls onto a floor and can
then be controlled through `input -> physics -> USD`.
