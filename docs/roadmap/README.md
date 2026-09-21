# Roadmap

Updated: 2026-09-21

The roadmap contains incomplete delivery work only. Design rationale and
contracts belong in [design](../design/); implemented behavior belongs in
[architecture](../architecture/).

| Document | Contents |
| --- | --- |
| [current.md](current.md) | The physics-boundary freeze, ownership audit, migration seams, and vehicle-core hold point. |
| [milestones.md](milestones.md) | Ordered extraction, `UsdPhysics`, consumer migration, validation, vehicle, and richer-runtime phases. |

Status vocabulary: **in progress** or **not started**. A milestone is complete
only when its runnable vertical slice and required tests exist.

## Delivery policy

- Deliver small vertical slices rather than disconnected framework skeletons.
- Keep a Stage runnable and inspectable at every completed milestone.
- Introduce schemas and directories with the slice that consumes them.
- Extract and validate shared physics ownership before resuming vehicle
  physics, behavior, and execution-adapter expansion.
- Remove completed task detail from the roadmap and update architecture pages.
- Preserve direct CMake builds while OpenStrata owns reproducible environment,
  packaging, and CI workflows.

## Quality bar

Every phase must preserve these properties:

- core logic is testable without OpenUSD, OpenExec, or concrete backend SDKs;
- backend and runtime dependency boundaries are explicit and checkable;
- fixed-step behavior is deterministic under a controlled clock;
- Runtime-to-USD writes are incremental;
- representative Stage fixtures are minimal and reproducible; and
- documentation distinguishes implemented behavior from planned behavior.
