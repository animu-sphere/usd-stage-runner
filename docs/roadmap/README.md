# Roadmap

The roadmap contains only incomplete work. Design rationale belongs in
[design/](../design/), while implemented behavior belongs in
[architecture/](../architecture/).

Legend: 🚧 in progress · ⬜ not started

| Document | Contents |
| --- | --- |
| [current.md](current.md) | The next milestone and its completion criteria. |
| [backlog.md](backlog.md) | Ordered vertical slices after the current milestone and unscheduled work. |

When a milestone is completed, remove its task detail from the roadmap, update
the architecture pages, and retain historical detail in a release record once a
release process exists.

## Quality bar

Every phase should preserve these properties:

- core logic is testable without OpenUSD, OpenExec, or concrete backends;
- runtime/backend dependency boundaries are explicit and checked;
- fixed-step behavior is deterministic under a controlled clock where practical;
- Stage fixtures are minimal and reproducible;
- CMake remains usable directly while OpenStrata owns reproducible workspace and
  distribution workflows; and
- documentation distinguishes implemented behavior from planned behavior.
