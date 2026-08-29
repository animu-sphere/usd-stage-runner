# Architecture Overview

## Current state

The repository currently contains documentation only. There is no runtime
library, application, USD/OpenExec plugin, physics backend, build definition, or
test suite yet.

This page must evolve with the code. A planned directory should move into the
implemented inventory only when its build targets and tests exist on the default
branch.

## Planned repository boundaries

The following map is a navigation contract for the initial repository skeleton;
it is not an implementation inventory.

| Path | Responsibility | Dependency character |
| --- | --- | --- |
| `libs/` | Backend-neutral runtime systems such as world, input, physics interfaces, behavior, camera, and vehicle logic. | Must not depend on OpenExec or concrete device/physics implementations. |
| `backends/` | Concrete adapters such as Jolt Physics and SDL input. | Depends inward on interfaces in `libs/`. |
| `plugins/` | USD schemas and OpenExec integration. | Thin integration over core libraries; plugin logic must not become the runtime implementation. |
| `apps/` | Executable hosts such as `stage_runner`. | Composes the Stage, runtime systems, backends, and plugins. |
| `examples/` | Small executable stages for character, vehicle, and camera slices. | Demonstrates public contracts. |
| `tests/` | Unit, integration, and Stage fixtures. | Verifies boundaries and end-to-end synchronization. |
| `third_party/` | Explicit third-party integration owned by the repository. | Must not leak implementation types into public core APIs. |

## Planned dependency direction

Dependencies point from composition and adapters toward small, stable core
interfaces:

```text
apps and plugins
       |
       v
backend adapters -----> core libraries
       |                    |
       v                    v
external SDKs          runtimeCore
```

In particular, the architecture intends to prevent these edges:

```text
runtimeCore  -> OpenExec
runtimeCore  -> Jolt
physicsCore  -> OpenUSD
vehicleCore  -> OpenExec
behaviorCore -> OpenExec
```

The complete intended model and rationale are in the
[design specification](../design/spec.md). Delivery order is tracked in the
[roadmap](../roadmap/).
