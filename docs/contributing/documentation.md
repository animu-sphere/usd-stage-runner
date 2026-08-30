# Documentation Guidelines

Documentation is part of the implementation contract. A change is incomplete if
it changes a public boundary, implemented architecture, or delivery status
without updating the owning page.

## Category ownership

| Category | Put this here | Do not put this here |
| --- | --- | --- |
| `concepts/` | Stable vocabulary, purpose, principles, and non-goals. | Procedures or implementation inventories. |
| `architecture/` | Current targets, dependencies, runtime flow, and on-disk layout. | Unimplemented aspirations or historical alternatives. |
| `design/` | Intended contracts, rationale, proposals, and accepted decisions. | Claims that a proposal is already implemented. |
| `roadmap/` | Incomplete, ordered work and measurable success conditions. | Completed work or decision rationale. |
| `contributing/` | Repository maintenance procedures. | End-user task guides. |

Add `guides/`, `reference/`, `releases/`, or `reports/` only when the repository
has real procedures, factual contracts, releases, or validation evidence for
them.

## Status rules

Design decisions use `proposed`, `accepted`, `superseded`, or `rejected`.
Roadmap items use `in progress` or `not started`. Architecture pages describe
only implemented state and therefore do not need proposal statuses.

An accepted decision is historical evidence. Do not substantially rewrite it;
add a new decision that supersedes it and link both documents.

## Links and paths

- Use relative links for repository documentation.
- Link to the owning document instead of duplicating its full content.
- Keep category `README.md` tables synchronized with their files.
- Prefer stable headings because other pages may link to them.
- Wrap commands, paths, targets, schema names, and action names in code spans.

## Change checklist

Before merging a documentation change:

1. Confirm planned behavior is not presented as implemented behavior.
2. Confirm each new page appears in its category index.
3. Check all relative links and heading anchors.
4. Update [architecture/](../architecture/) with implementation changes.
5. Remove completed work from [roadmap/](../roadmap/) rather than maintaining a
   second changelog there.
6. Keep the [design specification](../design/spec.md), its focused contract
   pages, and decision records consistent; call out intentional differences
   explicitly.
