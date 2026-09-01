# Milestone 4: Camera Rigs

Status: in progress

The character-control vertical slice, backend-neutral camera core, and camera
schema importer are implemented. This milestone next evaluates those imported
rigs in the runnable Stage without putting camera behavior in the standalone
host.

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

The representative Stage adds first- and third-person views that can switch
while following the character from `character_walk.usda`. A controlled clock
makes following and smoothing deterministic.

## Scope

### Runtime and Stage integration

- Add `RunnerCameraRigAPI` with the vertical slice that imports and updates it.
- Resolve target and anchor paths to Runtime World prim identities.
- Update camera rigs after physics extraction and before dirty USD
  synchronization.
- Synchronize only changed `UsdGeomCamera` transforms.
- Add `third_person_camera.usda` as the runnable golden scenario.

## Recommended PR sequence

1. **Schema and importer (implemented)** — `RunnerCameraRigAPI` plus
   prim-reference and authored-configuration validation.
2. **Runnable follow slice** — first- and third-person modes follow the
   character and synchronize incrementally in `third_person_camera.usda`.
3. **Collision probe** — add the smallest backend-neutral physics query needed
   to keep the third-person camera out of geometry.

## Completion criteria

- First- and third-person modes follow the controlled character.
- Mode switching preserves a documented, repeatable rig state.
- Core tests require neither OpenUSD nor Jolt.
- Following and smoothing are repeatable under a controlled clock.
- Camera collision support does not expose Jolt types to `cameraCore`.
- Runtime-to-USD writes contain only dirty camera transforms.
- Plain CMake and OpenStrata build paths remain valid in supported
  environments.

The relevant long-term contracts are documented in the
[runtime model](../design/runtime-model.md),
[module boundaries](../design/modules.md),
[USD integration](../design/usd-integration.md),
[host integration](../design/hosts.md), and
[testing strategy](../design/testing.md).
