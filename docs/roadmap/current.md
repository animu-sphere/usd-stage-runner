# Next Milestone: Input and Transform

Status: ⬜ not started

Phase 0 established the buildable runtime skeleton and is now represented in
the [architecture overview](../architecture/overview.md). The next vertical
slice adds backend-neutral input actions, an SDL adapter, and the first dirty
runtime-to-USD transform synchronization path.

## Scope

- add `inputCore` without SDL types in its public API;
- add an `inputSdl` adapter for keyboard and gamepad state;
- normalize physical controls into named actions;
- add a simple movement-intent runtime component;
- add runtime transform state and a dirty synchronization queue;
- synchronize only dirty runtime transforms to their USD prims; and
- add core unit tests plus an adapter/Stage integration test.

Initial actions should include `move.x` and `move.y`. Device bindings belong to
the adapter or host configuration; consumers see action names and values only.

## Required boundaries

- `inputCore` depends inward on `runtimeCore` and does not include SDL or
  OpenUSD types.
- `inputSdl` implements the core input boundary and owns SDL object lifetime.
- USD writes stay in the host or a dedicated adapter, not in `runtimeCore`.
- Runtime transforms are keyed by the existing prim identity; no second scene
  hierarchy is introduced.
- Tests can inject action state without requiring a physical device.

## Completion criteria

```text
keyboard or gamepad state
    -> named move actions
    -> movement intent
    -> dirty runtime transform
    -> USD xform update for /World/PlayerCube
```

The demo must run with a real SDL adapter, while the controller and transform
logic remain testable without SDL or wall-clock sleeps.
