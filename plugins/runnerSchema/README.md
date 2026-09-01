# runnerSchema

`runnerSchema` is the codeless OpenUSD schema bundle for authored
`usd-stage-runner` declarations. It defines four single-apply APIs:

- `RunnerPhysicsBodyAPI` declares `runner:physics:motionType` and
  `runner:physics:mass`;
- `RunnerColliderAPI` declares `runner:physics:shape` and
  `runner:physics:halfExtents`; and
- `RunnerCharacterAPI` declares ground-probe distance, maximum walkable slope
  angle in radians, and jump speed; and
- `RunnerCameraRigAPI` declares target and optional anchor relationships plus
  mode, framing offset, distance, pitch, yaw, and damping.

The half extents are local-space values. Ordered scale ops are applied when the
host creates the backend shape. The initial contract accepts only `box` shapes,
`static` or `dynamic` motion, and positive finite mass and extents.
Characters additionally require both physics APIs on the same prim and dynamic
motion. Character distances and speeds use Stage meters, and the current host
requires `metersPerUnit = 1` for all physics declarations.

Camera rigs apply only to `UsdGeomCamera` prims. `free` mode may omit a target;
`firstPerson`, `thirdPerson`, and `orbit` require exactly one target prim. An
authored anchor also resolves to exactly one prim. Referenced prims must be
loaded into the Runtime World and provide runtime transforms.
The current importer requires a Y-up Stage and rejects camera, target, or
anchor translations changed by ancestor transforms unless the prim resets its
xform stack.

`schema.usda` is the source of truth. `plugInfo.json` and
`generatedSchema.usda` are committed generated resources so the bundle can be
used when `usdGenSchema` or a compatible `pxr` Python environment is not
available. CMake regenerates them only after verifying that the selected Python
can import OpenUSD.
