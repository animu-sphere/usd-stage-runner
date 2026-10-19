# Physics scene full rebuild (2026-10-20)

This run verifies the library-level consumer rebuild slice. The implemented
boundary belongs in the
[architecture](../architecture/overview.md#experimental-full-scene-physics-rebuild);
host integration and display acceptance remain in the
[current roadmap](../roadmap/current.md).

The environment is Windows, Visual Studio 18 2026 / MSVC 19.51, Release and
OpenUSD 26.08. The ON build consumes a separate installed prefix rebuilt from
the sibling physics workspace, with deterministic Jolt 5.5.0 and experimental
six-DOF enabled. The OFF build consumes the existing local Box-package prefix.
Neither configuration adds a sibling source edge or changes public artifact
pins. MSBuild and CTest run outside the sandbox.

`stage_runtime.physics_scene_rebuild` generates a static floor, two dynamic
Boxes, a Scene and a disabled/enabled fixed joint. Falling/rest checks allow
`0.03` m position and `0.05` m/s velocity tolerances. A world-anchored fixed
body stays within `0.01` m over three seconds. The fixture also covers zero
gravity, changed gravity on explicit rebuild, absent settings at the factory,
replacement of the source Stage, removed prim mappings and old-handle rejection.
Root serialization and the session layer remain unchanged by reconstruction
and stepping.

Failure injection uses real Jolt worlds behind a neutral forwarding wrapper.
Shape and body creation fail after partial allocation; joint creation fails
after a prior constraint exists. Missing optional joint capability is tested
independently. Assertions verify error code/path, destruction of old and partial
worlds, empty mappings and recovery on a later valid rebuild. Invalid/ambiguous
snapshots fail before factory invocation. A null factory result reports typed
backend unavailability at the scene path.

The first new-fixture run failed because the default scale op expects float3
and the fixture supplied double3. The fixture was corrected to `GfVec3f`;
the rebuilt full-scene test then passed. No production workaround was needed.

The sibling physics dependency build passed all 25 tests, including gravity,
experimental joints/springs and clean-prefix consumers. Its No-Jolt gravity
verification is separate from this consumer run.

| Verification | Result |
| --- | --- |
| Fresh scene ON configure/build | Passed against installed rebuilt packages |
| Full scene ON CTest | 53/53 passed after fixture correction |
| Fresh scene OFF configure/build and CTest | 53/53 passed against existing Box packages, including the explicit disabled diagnostic |
| Separate installed `stageRuntime` consumer | Configure, link and execution passed using imported targets and installed headers only |
| Whitespace checks | `git diff --check` passed |

The changed documentation files pass relative-link and heading checks using
the sibling workspace's lightweight checker. Applying that checker to this
entire repository flags the pre-existing fragment-only quick-start link in
README: the checker does not handle fragment-only targets. This is not a
repository-wide documentation-check pass claim.

The installed consumer checks exported availability metadata, creates a
generated standard Box and Scene, steps directed gravity, rebuilds, rejects
old mappings and clears. It uses no source-tree include/library paths. This
consumer is a local build-directory verification program, not a new published
example or hosted CI gate.

Reproduction after installing coherent rebuilt physics packages:

```sh
cmake -S . -B build/scene-rebuild-on \
  -DUSD_STAGE_RUNNER_ENABLE_PHYSICS_USD=ON \
  -DUSD_STAGE_RUNNER_ENABLE_PHYSICS_SCENE=ON \
  -DUSD_STAGE_RUNNER_REQUIRE_OPENUSD=ON -DUSD_STAGE_RUNNER_REQUIRE_JOLT=ON \
  '-DCMAKE_PREFIX_PATH=<physics-prefix>;<OpenUSD-prefix>' \
  -DJolt_DIR=<Jolt-prefix>/lib/cmake/Jolt -Dpxr_DIR=<OpenUSD-prefix>
cmake --build build/scene-rebuild-on --config Release
ctest --test-dir build/scene-rebuild-on -C Release --output-on-failure
```

The joint fixture requires an installed backend with `SixDofConstraints`;
the selected SDK must meet that package's deterministic-build requirement.
Use a separate build directory with `USD_STAGE_RUNNER_ENABLE_PHYSICS_SCENE=OFF`
for the retained Box package path. No Linux, hosted CI, OpenStrata, or wider
host/display acceptance is claimed by this report.
