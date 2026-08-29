# Design

Design documents explain intended behavior and why significant choices were
made. They use a lightweight lifecycle:

```text
proposed -> accepted -> superseded (or rejected)
```

- **proposed**: under consideration or approved as direction but not yet proven
  by an implementation;
- **accepted**: implemented and retained as an architectural decision;
- **superseded**: replaced by a newer decision and kept for history; and
- **rejected**: evaluated but deliberately not adopted.

Do not silently rewrite an accepted decision when a material design change is
made. Add a replacement decision and link the two documents.

## Canonical specification

| Document | Status | Purpose |
| --- | --- | --- |
| [spec.md](spec.md) | canonical, pre-implementation | Intended runtime architecture, execution model, systems, dependencies, and scope. |

## Proposed decisions

| Document | Purpose |
| --- | --- |
| [proposed/0001-runtime-boundaries.md](proposed/0001-runtime-boundaries.md) | Establish USD, Runtime World, OpenExec, core-library, and backend ownership boundaries. |

There are no accepted decisions yet. A proposal should move to `accepted/` only
with the implementation that validates it.
