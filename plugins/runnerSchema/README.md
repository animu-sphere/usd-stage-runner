# runnerSchema

`runnerSchema` is the codeless OpenUSD schema bundle for authored
`usd-stage-runner` declarations. It defines three single-apply APIs:

- `RunnerPhysicsBodyAPI` declares `runner:physics:motionType` and
  `runner:physics:mass`;
- `RunnerColliderAPI` declares `runner:physics:shape` and
  `runner:physics:halfExtents`; and
- `RunnerCharacterAPI` declares ground-probe distance, maximum walkable slope
  angle in radians, and jump speed.

The half extents are local-space values. Ordered scale ops are applied when the
host creates the backend shape. The initial contract accepts only `box` shapes,
`static` or `dynamic` motion, and positive finite mass and extents.
Characters additionally require both physics APIs on the same prim and dynamic
motion. Character distances and speeds use Stage meters, and the current host
requires `metersPerUnit = 1` for all physics declarations.

`schema.usda` is the source of truth. `plugInfo.json` and
`generatedSchema.usda` are committed generated resources so the bundle can be
used when `usdGenSchema` or a compatible `pxr` Python environment is not
available. CMake regenerates them only after verifying that the selected Python
can import OpenUSD.
