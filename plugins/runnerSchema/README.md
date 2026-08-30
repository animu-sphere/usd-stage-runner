# runnerSchema

`runnerSchema` is the codeless OpenUSD schema bundle for authored
`usd-stage-runner` declarations. It defines two single-apply APIs:

- `RunnerPhysicsBodyAPI` declares `runner:physics:motionType` and
  `runner:physics:mass`;
- `RunnerColliderAPI` declares `runner:physics:shape` and
  `runner:physics:halfExtents`.

The half extents are local-space values. Ordered scale ops are applied when the
host creates the backend shape. The initial contract accepts only `box` shapes,
`static` or `dynamic` motion, and positive finite mass and extents.

`schema.usda` is the source of truth. `plugInfo.json` and
`generatedSchema.usda` are committed generated resources so the bundle can be
used when `usdGenSchema` or a compatible `pxr` Python environment is not
available. CMake regenerates them only after verifying that the selected Python
can import OpenUSD.
