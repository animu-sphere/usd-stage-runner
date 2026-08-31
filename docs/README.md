# usd-stage-runner Documentation

The documentation separates the implemented repository from the intended
runtime and from the work required to get there. Start with the category that
matches the question you are trying to answer.

| Question | Source of truth | Start here |
| --- | --- | --- |
| What is this project and which ideas define it? | [Concepts](concepts/) | [Project overview](concepts/overview.md) |
| What exists on the default branch today? | [Architecture](architecture/) | [Current architecture](architecture/overview.md) |
| What are the intended contracts and boundaries? | [Design](design/) | [Design specification](design/spec.md) |
| What should be implemented next, and in what order? | [Roadmap](roadmap/) | [Current milestone](roadmap/current.md) |
| What did dated toolchain verification actually observe? | [Reports](reports/) | [OpenStrata dogfooding](reports/ost/) |
| How should contributors maintain these documents? | [Contributing](contributing/) | [Documentation guidelines](contributing/documentation.md) |

## Reading paths

For an implementation overview, read the
[current architecture](architecture/overview.md), then the
[current character milestone](roadmap/current.md).

For design work, begin with the [design specification](design/spec.md) and then
open the focused contract for the area being changed:

- [runtime model](design/runtime-model.md);
- [input actions and intent](design/input.md);
- [module and dependency boundaries](design/modules.md);
- [USD declarations and synchronization](design/usd-integration.md);
- [host integration](design/hosts.md); and
- [testing strategy](design/testing.md).

## Source-of-truth policy

- Code is authoritative for implemented behavior. The
  [architecture documentation](architecture/) records that behavior and must be
  updated with implementation changes.
- The [design documentation](design/) defines intended contracts. It may
  describe systems that do not exist yet, but must label that status clearly.
- The [roadmap](roadmap/) contains incomplete delivery work only. Once a
  milestone is implemented, remove its task detail from the roadmap and update
  architecture documentation.
- The [reports](reports/) preserve dated observations. A later result adds a
  follow-up rather than rewriting the original evidence.
- Design rationale belongs in a focused design page or decision record rather
  than in milestone checklists.

Task guides, generated API reference, and release records will be added only
when implemented behavior gives those categories real content.
