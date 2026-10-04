# Phase D hosted delivery preparation

Date: 2026-10-04 (Asia/Tokyo). Scope: hosted pre-publication results and local
verification of the resulting packages. This follows the
[local standard-physics gate](05-2026-10-04-phase-d-standard-physics.md).

## Hosted consumer results

[Stage Runner run 37145069321](https://github.com/animu-sphere/usd-stage-runner/actions/runs/37145069321)
passed on `ubuntu-24.04` and `windows-2025-vs2026`, using OpenStrata 0.23.1,
the existing pinned runtime/core/backend artifacts, Jolt 5.5.0, and the parser
source at `3daad33a1f9f8418ce25f41d14526f0450062d35`.

| Check | Windows | Linux |
| --- | --- | --- |
| Compatibility workspace tests | 47 passed | 47 passed |
| Standalone installed-parser test | 1 passed | 1 passed |
| Standard-enabled plain CMake | 52 passed | 52 passed |
| Standard-enabled OpenStrata intent | 55 passed | 55 passed |

The parity gate requires standard and compatibility falling bodies, grounded
walking, jumping, camera obstruction, StageSession lifecycle, native usdview,
and both OpenStrata-staged Character adapters. The parser is built from source
and installed separately; these results establish hosted installed-package
parity before public parser pins.

Both PRs have merged: [Stage Runner #32](https://github.com/animu-sphere/usd-stage-runner/pull/32)
and [physics packages #11](https://github.com/animu-sphere/usd-physics-plugins/pull/11).

## Verified package candidates

[Physics run 37144640730](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/37144640730)
passed its graph, Windows, and Linux cells with OpenStrata 0.23.2. Each OS passed
12 workspace tests and isolated build/test/package steps for `physicsCore`,
Jolt-enabled `physicsJolt`, and `physicsUsd`. Its source head was
`3daad33a1f9f8418ce25f41d14526f0450062d35`; package manifests identify the PR merge
revision `a5439b1c17a8f3720915187d0044e5fc4e204786` as their source.

The two report artifacts were downloaded into the sibling repository's ignored
`.ci/phase-d-delivery` directory. All six package outputs were imported into a
separate local OpenStrata registry and passed `ost artifact verify` with both
`--require-sbom` and `--require-provenance`. Archive digests and every installed
file matched. The two parser candidates have these content digests:

| Target | Archive content digest |
| --- | --- |
| `cy2026-windows-x86_64-py313-usd` | `sha256:a3a1cc3e90ba56f99151f26f6db22673944b94da51e72259de05a4dbee548876` |
| `cy2026-linux-x86_64-py313-usd` | `sha256:46d4ea15d829fd8333ea9fe5dc432c26bb996bf59c0c953550fb636394aece2e` |

These are verified CI candidates. They have not been published as part of this
check, and no OCI manifest digest is claimed. The Windows/Linux runtime digest
in each parser manifest matches the corresponding Stage Runner toolchain.

## Windows packaged-parser consumer check

The Windows parser candidate was extracted with `ost artifact extract` into
Stage Runner's ignored `.ci/hosted-physics-usd-install` prefix. A fresh
`.ci/hosted-parity-cmake` build used that installed binary, the existing pinned
`physicsCore`/`physicsJolt` prefixes, the matching OpenUSD 26.08 SDK, and the
compatible external Jolt SDK. The local compiler was MSVC 19.51, with Python
3.13.14 and OpenStrata 0.23.14.

Configuration and linking succeeded. The coverage assertion found all 11
required standard/compatibility host tests, and all 52 CMake tests passed,
including the native Python adapter and runtime-layer preservation tests.
The parser was consumed from the hosted archive, without a parser source build.

This check uses the current published core/backend pins. It does not require
replacing them with the new package candidates. It establishes local Windows
binary compatibility for the parser; Linux archived-binary consumption and
OpenStrata remote resolution still need the public-pin gate.

## Next delivery gate

Publish the two verified parser candidates to the existing public
`ghcr.io/animu-sphere/usd-physics-plugins` repository as Phase D consumer inputs.
Record the actual push results and verify fresh pulls with the expected content
digest, package kind, target, SBOM, and provenance. Add both exact content and
OCI source digests to Stage Runner's library requirements, then repeat hosted
CMake/OpenStrata parity against those pins.

Only after that gate should standard import become the default and host examples
switch to standard fixtures. Formal versioned releases remain governed by the
sibling repository's proposed release schema.
