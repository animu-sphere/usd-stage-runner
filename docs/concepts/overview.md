# Overview

`usd-stage-runner` is an experimental runtime for treating an OpenUSD Stage as
an interactive, real-time space rather than only as a scene to inspect.

The project aims to make it possible to:

- control characters and vehicles represented by USD prims;
- evaluate behavior, controller, and calculation graphs;
- run collision, rigid-body, character, and vehicle simulation;
- follow runtime objects with first-person, third-person, orbit, and chase
  cameras; and
- synchronize the resulting runtime state back to the Stage.

It is not intended to become a full game engine. The central experiment is to
combine OpenUSD composition, OpenExec evaluation, real-time physics, and small,
composable runtime systems without making any one of them own the entire
application.

## The four-part model

```text
USD Stage       structure, composition, authored values, persistence
Runtime World   transient per-frame simulation state
OpenExec        dependency and behavior evaluation
Physics backend collision, constraints, and physical simulation
```

The USD Stage remains authoritative for what exists and how it is composed. A
Runtime World is derived from that description and owns fast-changing state.
OpenExec evaluates graphs over runtime-facing interfaces. A physics backend,
initially expected to be Jolt Physics, performs physical simulation behind a
backend-neutral boundary.

## Prim and component vocabulary

The design deliberately avoids introducing a second, independent scene
hierarchy:

```text
USD prim          approximately an entity
USD API schema    a component declaration
runtime component the live instance created from that declaration
```

USD references, payloads, variants, and other composition features can therefore
serve the role often filled by prefabs and entity composition. Runtime component
instances are indexed by prim identity rather than by a competing hierarchy.

## Intended users

The initial users are developers exploring interactive OpenUSD runtimes and
contributors building reusable runtime, physics, input, camera, behavior, and
OpenExec integration libraries.

## Non-goals for the initial phases

- a custom renderer;
- a full-featured ECS framework;
- a general animation system;
- multiplayer or deterministic rollback;
- an editor UI or scripting language; and
- massive-world streaming.

The first proof is intentionally small: open a Stage containing a ground plane,
a controllable cube, and a camera; then carry input through runtime logic and
physics back to a visible USD transform.
