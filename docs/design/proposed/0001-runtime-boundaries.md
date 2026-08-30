# 0001: Runtime Ownership and Dependency Boundaries

Status: proposed

## Context

An interactive Stage contains both durable scene description and rapidly
changing simulation state. OpenUSD, OpenExec, physics SDKs, and device libraries
could each become the de facto runtime owner if their responsibilities are not
defined before implementation starts. That would make logic hard to test and
would couple public APIs to one backend.

## Proposal

Adopt five ownership rules:

1. The USD Stage owns composition, authored configuration, and stable prim
   identity.
2. A Runtime World owns transient component and simulation state.
3. OpenExec evaluates runtime-facing graphs but does not own the host loop or
   domain implementations.
4. Physics and input SDKs are hidden behind core interfaces in backend adapters.
5. OpenUSD and OpenExec plugins remain thin wrappers around ordinary C++
   libraries.

The dependency direction is core first, adapters second, composition last:

```text
runtimeCore <- domain core <- backend or plugin <- stage_runner
```

## Consequences

- Core libraries can be unit-tested without OpenUSD, OpenExec, Jolt, or SDL.
- The Stage and Runtime World require explicit bidirectional synchronization.
- Prim identity mapping becomes a first-class runtime service.
- Some useful backend features require explicit extensions instead of leaking
  SDK types into the common interface.
- Initial implementation takes more boundary work than placing all behavior in
  the host application, but later systems can reuse the same contracts.

## Acceptance criteria

Move this decision to `accepted/` only when the repository contains:

- at least one core library with no OpenUSD/OpenExec/backend dependency;
- at least one adapter implementing a core interface;
- tests that exercise the core without the adapter SDK; and
- a build-time or CI check for forbidden dependency edges.

The complete direction is indexed by the [design specification](../spec.md).
The ownership model and dependency rules are expanded in
[runtime-model.md](../runtime-model.md) and [modules.md](../modules.md).
