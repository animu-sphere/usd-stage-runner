# Roadmap

Updated: 2026-09-01

The roadmap contains incomplete delivery work only. Design rationale and
contracts belong in [design](../design/); implemented behavior belongs in
[architecture](../architecture/).

| Document | Contents |
| --- | --- |
| [current.md](current.md) | The camera-rig vertical slice, completion criteria, and recommended PR sequence. |
| [milestones.md](milestones.md) | Ordered Camera-through-Tooling milestones, host integration, the representative demo, and cross-cutting follow-up work. |

Status vocabulary: **in progress** or **not started**. A milestone is complete
only when its runnable vertical slice and required tests exist.

## Delivery policy

- Deliver small vertical slices rather than disconnected framework skeletons.
- Keep a Stage runnable and inspectable at every completed milestone.
- Introduce schemas and directories with the slice that consumes them.
- Follow the direct Character -> Camera -> Host path before expanding into
  vehicle, behavior, and execution adapters.
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
