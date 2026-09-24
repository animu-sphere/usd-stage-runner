# Phase C hosted physics consumer verification

## Scope

This report records Stage Runner's hosted installed-package verification on
2026-09-23 UTC. The [OpenStrata run](https://github.com/animu-sphere/usd-stage-runner/actions/runs/35853380748)
tested commit `f6dd2a089d2ce5ac0bd01612c97cb47f5ae7094f` on
`windows-2025-vs2026` and `ubuntu-24.04`. Its second attempt ran with the
repository variable `OST_CI_DISABLE_CACHE=true`; both artifact-cache restore
steps were skipped. The variable was removed after the run.

OpenStrata 0.23.1 used the pinned `cy2026` / `usd` runtimes and pulled the
published `physicsCore` and `physicsJolt` packages by content and OCI manifest
digest. The Jolt SDK was built from Physics 5.5.0 at
`23dadd0e603f1b321142d4c74df07fce85064989`.

| Linux package | Archive content digest | OCI manifest digest |
| --- | --- | --- |
| `physicsCore` | `sha256:81a3433b759e6ec817188a2137f8da24ff8dbebed09a176f48a7ec2596d6f5ad` | `sha256:f050253cea8f6db163b67c9860e88bdf213c8ef400956c29fec130bc4b6726d7` |
| `physicsJolt` | `sha256:2be1f995f3e59d29fcc5a3db66321d4022fd83965d68f09e9b82b5b73bf2703b` | `sha256:e51df5e3815cbbe57034a390c6218f7467be70f92500b437eb7c77f2feda9020` |

## Result

Both OpenStrata cells built the Stage Runner workspace, found the real
`stage_runner.physics_falling_cube` test, passed all 46 CTest tests, and passed
the workspace verification pyramid. The tests included the native usdview
session, its Python controller, and Jolt-backed falling-body behavior. The
separate [plain-CMake run](https://github.com/animu-sphere/usd-stage-runner/actions/runs/35853380423)
passed all three jobs: Windows and Linux core builds plus the Linux
Jolt-enabled build, with eight tests in each job.

The first hosted Linux attempt exposed a static-to-shared link error when
`usdviewStageRunner` linked non-PIC Stage Runner libraries and the Jolt SDK.
The fix compiles repository static libraries as position-independent code and
uses a PIC Jolt SDK on Linux. The passing run verifies that the same installed
physics package graph builds the standalone and usdview hosts.

## Limits

The hosted suite tests the usdview native module and controller without an
interactive display. Standard `UsdPhysics` authoring, an additional external
consumer, and the sibling package's versioned release workflow remain later
work; this run closes the Stage Runner Phase C package migration.
