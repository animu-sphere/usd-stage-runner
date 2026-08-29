# usd-stage-runner Documentation

This documentation is organized by responsibility. Each category answers one
primary kind of question, and planned behavior is kept separate from behavior
that is already implemented.

| Category | Answers | Start here |
| --- | --- | --- |
| [concepts/](concepts/) | What `usd-stage-runner` is and which ideas define it. | [overview.md](concepts/overview.md) |
| [architecture/](architecture/) | How the repository and runtime are structured on the default branch today. | [overview.md](architecture/overview.md) |
| [design/](design/) | What is being designed and why, including proposed and accepted decisions. | [spec.md](design/spec.md) |
| [roadmap/](roadmap/) | What is incomplete and in which order it should be delivered. | [current.md](roadmap/current.md) |
| [contributing/](contributing/) | How contributors should keep documentation accurate and navigable. | [documentation.md](contributing/documentation.md) |

## Source-of-truth policy

[design/spec.md](design/spec.md) is the canonical specification for the intended
runtime. It describes the destination, not a claim that every subsystem exists.

[architecture/](architecture/) is the source of truth for the repository as it
is implemented. When code lands, architecture pages must be updated in the same
change. If an architecture page and the code disagree, the code is authoritative
and the page is a documentation bug.

[roadmap/](roadmap/) contains only incomplete work. Completed milestones should
be removed from the roadmap and reflected in architecture documentation and,
once releases exist, immutable release records.

## Categories not created yet

Task guides, generated API reference, release records, and validation reports
will be added when there is implemented behavior to document. Empty categories
are intentionally avoided.
