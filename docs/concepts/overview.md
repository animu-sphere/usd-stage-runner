# Overview

`usd-stage-runner` is an experimental, lightweight real-time runtime and
orchestration layer for treating an OpenUSD Stage as an interactive world
rather than only as a scene to inspect. It executes a Stage; it does not
convert the Stage into a separate game-engine scene.

The project aims to make it possible to:

- control characters and vehicles represented by USD prims;
- evaluate behavior, controller, and calculation graphs;
- run collision, rigid-body, character, and vehicle simulation;
- follow runtime objects with first-person, third-person, orbit, and chase
  cameras; and
- synchronize the resulting runtime state back to the Stage.

It is not intended to become a full game engine, physics engine, renderer, or
avatar-format implementation. Its role is to construct a transient Runtime
World, own real-time and fixed-step orchestration, coordinate reusable systems,
and synchronize live results to a discardable USD runtime layer.

## The four-part model

```text
USD Stage       structure, composition, authored values, persistence
Runtime World   transient per-frame simulation state
OpenExec        dependency and behavior evaluation
Physics package collision, constraints, queries, and physical simulation
```

The USD Stage remains authoritative for what exists and how it is composed. A
Runtime World is derived from that description and owns fast-changing state.
OpenExec is an optional execution surface over runtime-facing interfaces, not
the runtime's foundation. A reusable physics package performs physical
simulation behind backend-neutral capabilities. The current repository still
contains `physicsCore` and the Jolt adapter; the proposed direction extracts
them to `usd-physics-plugins` and makes Stage Runner their consumer.

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

`stage_runner` is the first host and proof harness, not the runtime itself. The
same libraries are intended to run from the standalone host, usdview, and OST
Plugin View.

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

The representative product demo grows that proof into a Stage-declared
character controlled by a keyboard or gamepad, moved through Jolt, followed by
a third-person camera, and updated live in any supported host. VRM can supply a
character's visual representation, but it does not define the character runtime
contract.
