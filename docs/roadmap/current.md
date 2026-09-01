# Milestone 4: Camera Rigs

Status: in progress

The character-control vertical slice and runnable first-/third-person camera
follow path are implemented. The standalone host now imports and evaluates
camera rigs after physics extraction, then incrementally synchronizes dirty
camera translations and orientations. This milestone next adds camera collision
avoidance without coupling `cameraCore` to Jolt.

## Outcome

```text
controlled character RuntimeTransform
    -> camera target and rig state
    -> desired camera transform
    -> optional physics collision probe
    -> smoothing
    -> dirty RuntimeTransform
    -> USD Camera xform update
```

The representative Stage contains first- and third-person views that follow the
same controlled target. Isolated core coverage verifies repeatable live mode
changes, while the Stage test verifies follow updates and dirty USD writes under
a controlled clock.

## Scope

### Collision probe

- Add the smallest backend-neutral physics query needed to probe from the rig
  origin toward the desired third-person camera position.
- Shorten the camera distance when geometry blocks the desired pose without
  exposing Jolt handles or result types to `cameraCore`.
- Extend `third_person_camera.usda` with deterministic obstruction coverage.

## Recommended PR sequence

The schema/importer and runnable follow slices are implemented and recorded in
the [current architecture](../architecture/overview.md). The next PR adds the
collision probe and its obstructed-camera scenario.

## Completion criteria

- Camera collision support does not expose Jolt types to `cameraCore`.
- Plain CMake and OpenStrata build paths remain valid in supported
  environments.

The relevant long-term contracts are documented in the
[runtime model](../design/runtime-model.md),
[module boundaries](../design/modules.md),
[USD integration](../design/usd-integration.md),
[host integration](../design/hosts.md), and
[testing strategy](../design/testing.md).
