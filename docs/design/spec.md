# Runtime Design Specification

Status: canonical direction; partially implemented

This page is the entry point for the intended `usd-stage-runner` runtime. The
[architecture documentation](../architecture/) separately records what is
implemented today, while the [roadmap](../roadmap/) orders the remaining work.

## Purpose

`usd-stage-runner` treats an OpenUSD Stage as an interactive, real-time world.
It connects USD composition to input, physics, characters, vehicles, cameras,
behavior, and OpenExec evaluation through small reusable runtime libraries.

The project is not intended to become a complete game engine. It advances
through runnable vertical slices that can be inspected and tested on a real
Stage.

## System map

```text
USD Stage
    | declarations and authored state
    v
Runtime World
    | transient state and prim-indexed components
    +--> Input and controller intent
    +--> Physics, characters, and vehicles
    +--> Behavior and OpenExec evaluation
    +--> Camera rigs
    `--> Incremental USD synchronization
             ^
             |
      stage_runner / usdview / OST Plugin View
```

The focused specifications are:

- [Runtime model](runtime-model.md): ownership, identity, fixed stepping, and
  execution order.
- [Input actions and intent](input.md): device normalization, action value
  types, and the boundary between input and gameplay intent.
- [Modules and dependencies](modules.md): reusable library boundaries, backend
  adapters, plugins, and intended repository topology.
- [USD integration](usd-integration.md): schema declarations, bidirectional
  synchronization, and non-destructive play sessions.
- [Host integration](hosts.md): standalone, usdview, OST Plugin View, and
  OpenStrata responsibilities.
- [Testing strategy](testing.md): deterministic verification and representative
  Stage fixtures.

## Project invariants

1. USD owns scene definition; Runtime World owns transient simulation state.
2. A USD prim is the runtime entity identity; no second scene hierarchy is
   introduced.
3. Runtime core libraries do not depend on backend SDKs.
4. OpenExec nodes are adapters and are not the only implementation of runtime
   logic.
5. Physical devices, named input actions, and gameplay intent are separate
   layers.
6. Characters and vehicles move through physics rather than direct transform
   editing.
7. Runtime-to-USD synchronization is incremental.
8. Physics advances with a deterministic, bounded fixed timestep.
9. USD composition, references, and variants provide prefab-like composition.
10. Standalone, usdview, and OST hosts reuse the same runtime libraries.
11. New abstraction follows a working vertical slice rather than preceding it.

Any proposal or implementation that breaks an invariant requires an explicit
design decision explaining the replacement.
