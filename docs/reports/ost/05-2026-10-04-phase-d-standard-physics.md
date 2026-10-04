# Phase D local standard-physics gate

Date: 2026-10-04 (Asia/Tokyo). Scope: local Windows verification and CI
preparation, before `physicsUsd` artifact publication.

Follow-up: the [hosted delivery report](06-2026-10-04-phase-d-hosted-delivery.md)
records subsequent Windows/Linux CI success and package-candidate verification.

## Environment and inputs

- Windows x86_64, MSVC 19.51 / Visual Studio 18 2026, CMake 4.4,
  Python 3.13.14, and local OpenStrata 0.23.14.
- OpenUSD 26.08, existing installed/pinned `physicsCore` and Jolt-enabled
  `physicsJolt`, and a compatible external Jolt SDK.
- `physicsUsd` source at sibling revision
  `193d01c4897d6a261733c09d2ab7339a088b2b2d`, built as its standalone CMake
  package and installed into a separate prefix.

The source-CI workflow retains its OpenStrata 0.23.1 bootstrap. These local
results do not establish hosted compatibility with that pinned CLI.

## Observations

`tests/run_physics_usd_ci.ps1` passed its standalone parser test, install,
plain-CMake build and 52 tests, then the `physics-usd` OpenStrata build and
tests. A final run including the staged standard Character test passed 55 tests.
The coverage check requires both standard and compatibility host scenarios,
so a build omitting the parser, Jolt, or native Python adapter fails the gate.

The standalone additions use the same expected results for standard and
Runner fixtures: grounded walking, jumping, and camera obstruction with dirty
USD writes. Session parity compares 180 frames and verifies Reset, persistent
edits, Stop, and runtime-layer removal. Native and staged usdview tests verify
movement, jumping, camera following, and persistent-layer preservation.

The parser was compiled separately from Stage Runner and linked through its
installed `physicsUsd::physicsUsd` target. Stage Runner's core/backend inputs
remain installed packages; the workflow does not add a sibling source target.

## Remaining delivery gates

The workflow now prepares these checks on Windows and Linux, but this report
contains no hosted execution or Linux result. The parser is a pinned source
install for pre-publication validation. Publish and pull both platform
artifacts, add exact content/OCI digest pins, and repeat parity against that
graph before enabling standard import by default or changing host examples.
