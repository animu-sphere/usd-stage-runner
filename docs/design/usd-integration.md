# USD Integration

Status: physics, character, and camera schemas and importers plus dirty
transform and camera-orientation write-back implemented; later domain schemas
and incremental Stage notices planned

## Runtime declarations

USD schemas declare how a prim should be interpreted by the runtime. They do
not contain simulation implementations or expose private backend state.

Applied API schemas are preferred over a large typed-schema hierarchy because
they compose with existing assets, references, vehicles, and characters.

Domain contracts are introduced only when their runtime slice exists:

| Slice | Applied API schemas |
| --- | --- |
| Physics | `RunnerPhysicsBodyAPI`, `RunnerColliderAPI` |
| Character | `RunnerCharacterAPI` |
| Camera | `RunnerCameraRigAPI` |
| Vehicle | `RunnerVehicleAPI`, `RunnerWheelAPI`, followed by focused suspension, steering, and drivetrain APIs as needed |
| Behavior | `RunnerBehaviorAPI` |

Schema values are declarations that are parsed into prim-indexed runtime
components. A schema instance is not itself the runtime object, and applying an
API schema never transfers simulation ownership to USD.

Example physics declaration:

```usda
def Xform "Player"
(
    apiSchemas = ["RunnerPhysicsBodyAPI", "RunnerColliderAPI"]
)
{
    token runner:physics:motionType = "dynamic"
    double runner:physics:mass = 70
    token runner:physics:shape = "box"
    double3 runner:physics:halfExtents = (0.5, 1, 0.5)
}
```

The implemented `runnerSchema` bundle is codeless: `schema.usda` defines the
authored contract and generated plugin resources register it without adding a
C++ ABI. A physics prim must apply both APIs. Motion accepts `static` or
`dynamic`; mass and all three half extents must be positive and finite; and the
initial shape contract accepts only `box`. Half extents are local-space values
multiplied by ordered scale ops.

Physics prims currently require a Y-up Stage with `metersPerUnit = 1`, one
translate op followed only by scale ops, and identity ancestor transforms
unless `resetXformStack` is set. These restrictions keep local USD translation
identical to the Jolt world-space position until composed transform support
lands. The importer creates and binds backend bodies through `PhysicsRuntime`;
a physics-declaring Stage is rejected when Jolt is unavailable.

The implemented `RunnerCharacterAPI` declares
`runner:character:groundProbeDistance`,
`runner:character:maximumSlopeAngleRadians`, and
`runner:character:jumpSpeed`. It must be applied with both physics APIs to the
same dynamic body prim. After physics import, the host binds a prim-indexed
runtime `CharacterController` to that body and the backend-neutral ground-query
capability. The schema importer does not itself choose or update player intent.
The host feeds movement and jump actions through `CharacterIntent` at each
fixed step.

The implemented `RunnerCameraRigAPI` applies to `UsdGeomCamera` prims and
declares `target` and optional `anchor` prim relationships plus mode, offset,
distance, pitch, yaw, damping, collision enablement, and collision clearance.
The importer resolves relationship targets
to prim-indexed Runtime World transforms and validates the authored values
through `cameraCore`. Non-free modes require exactly one target. Per-frame rig
evaluation runs after physics extraction. Changed poses enter the shared dirty
queue; the host synchronizes their translation and writes forward direction as
the dedicated `xformOp:orient:runnerCamera` quaternion op.
Because `RuntimeTransform` currently imports local translations, camera
declarations require a Y-up Stage and camera, target, and anchor prims whose
local translation also represents their world translation. Non-identity
ancestor transforms are rejected unless `resetXformStack` makes the prim local
to world. Rig Camera prims accept an empty transform stack or one translate op
optionally followed by the reserved double-precision runtime orient op, and self
target or anchor references are invalid. Collision-enabled rigs require a
physics world that exposes the backend-neutral segment query; the host ignores
the followed target's body when it is bound. Composed camera transforms remain
part of the later transform work.

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
