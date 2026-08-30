# Design

Design documents define intended runtime contracts and explain why those
contracts exist. They describe the destination architecture, not the current
implementation inventory.

## Specification

| Document | Purpose |
| --- | --- |
| [spec.md](spec.md) | Entry point, system map, invariants, and links to the focused contracts below. |
| [runtime-model.md](runtime-model.md) | State ownership, prim identity, frame execution, and intent flow. |
| [modules.md](modules.md) | Target topology, core/adapter boundaries, and allowed dependencies. |
| [usd-integration.md](usd-integration.md) | Applied API schemas, incremental synchronization, and play-session layers. |
| [hosts.md](hosts.md) | Responsibilities of `stage_runner`, usdview, OST Plugin View, and OpenStrata. |
| [testing.md](testing.md) | Deterministic unit, adapter, integration, and Stage-fixture strategy. |

## Decision lifecycle

Focused decisions use this lifecycle:

```text
proposed -> accepted -> superseded (or rejected)
```

- **proposed**: approved as direction or under consideration, but not yet fully
  validated by implementation;
- **accepted**: implemented and retained as an architectural decision;
- **superseded**: replaced by a newer decision and kept for history; and
- **rejected**: evaluated but deliberately not adopted.

Do not silently rewrite an accepted decision when a material design change is
made. Add a replacement decision and link both records.

### Proposed decisions

| Document | Purpose |
| --- | --- |
| [proposed/0001-runtime-boundaries.md](proposed/0001-runtime-boundaries.md) | Establish USD, Runtime World, OpenExec, core-library, and backend ownership boundaries. |

There are no accepted decisions yet. A proposal moves to `accepted/` only when
its acceptance criteria are satisfied by the repository.
