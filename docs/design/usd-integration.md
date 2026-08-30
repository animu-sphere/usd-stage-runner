# USD Integration

Status: intended contract; temporary physics import, transform import, and
dirty write-back implemented

## Runtime declarations

USD schemas declare how a prim should be interpreted by the runtime. They do
not contain simulation implementations or expose private backend state.

Applied API schemas are preferred over a large typed-schema hierarchy because
they compose with existing assets, references, vehicles, and characters.

Candidate contracts, introduced only when their runtime slice exists, are:

| Slice | Applied API schemas |
| --- | --- |
| Physics | `RunnerPhysicsBodyAPI`, `RunnerColliderAPI` |
| Character | `RunnerCharacterAPI` |
| Camera | `RunnerCameraRigAPI` |
| Vehicle | `RunnerVehicleAPI`, `RunnerWheelAPI`, followed by focused suspension, steering, and drivetrain APIs as needed |
| Behavior | `RunnerBehaviorAPI` |

Example physics declaration:

```usda
def Xform "Player"
(
    apiSchemas = ["RunnerPhysicsBodyAPI", "RunnerColliderAPI"]
)
{
    token runner:physics:motionType = "dynamic"
    float runner:physics:mass = 70
}
```

Temporary conventions are acceptable while proving a vertical slice, but the
schema milestone must replace them before they become a public asset contract.

The implemented temporary convention is limited to `UsdGeomCube` prims. It
uses custom `runner:physics:motionType` token values `static` and `dynamic`, plus
an optional `runner:physics:mass` double for dynamic bodies. Cube size and local
scale ops determine the box half extents. Physics Cubes currently require a
Y-up Stage and translate/scale-only local transform ops. These names
deliberately mirror the candidate schema namespace, but they are not yet a
stable asset contract.

## Runtime to USD

Writes are incremental. A changed simulation component marks its prim dirty;
the synchronization point drains the duplicate-free queue and authors only
changed properties.

```text
physics body transform changes
    -> RuntimeTransform dirty
    -> synchronization queue
    -> USD xform update
```

The runtime must not rewrite every simulated prim every frame.

## USD to runtime

An explicit rebuild is sufficient initially. Later, Stage notices such as
`UsdNotice::ObjectsChanged` can identify changed prims or properties and update
only affected components. Relevant live-authoring changes include collider
dimensions, mass, camera-rig parameters, and behavior assets.

## Play-session layer

Simulation writes must not silently contaminate authored scene data. Interactive
hosts should place live values in a dedicated anonymous or session layer:

```text
root layer
    + session / runtime layer
```

Stopping a play session should support three explicit outcomes:

- discard all runtime values;
- bake the result; or
- commit selected properties.

This boundary is required before usdview or editor integration is considered
safe for authoring workflows.

## Debug data

Core systems expose diagnostics as data or debug primitives; they do not own a
renderer. Hosts may visualize colliders, centers of mass, velocities, contacts,
ground state, suspension, camera probes, and behavior state.
