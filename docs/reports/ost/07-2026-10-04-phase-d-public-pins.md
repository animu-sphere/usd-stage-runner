# Phase D public physicsUsd pins

Date: 2026-10-04 (Asia/Tokyo). Scope: publication of the two verified parser
candidates from the [hosted delivery report](06-2026-10-04-phase-d-hosted-delivery.md)
and downstream consumption of those exact public binaries.

## Publication and fresh pulls

OpenStrata 0.23.14 pushed the candidates to the existing public
`ghcr.io/animu-sphere/usd-physics-plugins` repository. Each push reported
`status: pushed` and `already_present: false`. The existing core/backend pins
remain the consumer inputs; only the parser is added.

| Target | Archive content digest | OCI manifest digest |
| --- | --- | --- |
| `cy2026-windows-x86_64-py313-usd` | `sha256:a3a1cc3e90ba56f99151f26f6db22673944b94da51e72259de05a4dbee548876` | `sha256:2b5289786e0a8a7d807922057a552d5111674c729baad75baec111f49f710269` |
| `cy2026-linux-x86_64-py313-usd` | `sha256:46d4ea15d829fd8333ea9fe5dc432c26bb996bf59c0c953550fb636394aece2e` | `sha256:f84f4a731e6a9d7325a8092ef46c40e614a80384538a11cf0b8583919450fbf5` |

The descriptive publication tags are `physicsUsd-phase-d-<target>`. Consumer
requirements use the immutable OCI digests above, not those tags. This is a
Phase D consumer delivery, not a formal versioned release.

Both packages were then pulled without registry credentials into a new,
separate local registry. Each pull required the expected content digest,
`package` kind, exact target, SPDX SBOM, and SLSA/in-toto provenance. OCI bytes,
archive digest and safety, all six installed files, target, kind, and both
evidence records passed. A subsequent `ost artifact verify` again matched all
six files with no missing, extra, or mismatched files. The original package
source identity remains revision `a5439b1c17a8f3720915187d0044e5fc4e204786` from
the successful physics CI run; publication did not rebuild or alter it.

Redacted push/pull/verify JSON is retained in ignored `.ci` directories in the
two local repositories. Stage Runner's `ost library pull` resolved the parser
alongside the existing `physicsCore` and `physicsJolt` package pins.

## Consumer gate

The source CI now consumes the pinned parser package through
`libs/stageRuntime/openstrata.library.yaml`. It no longer checks out parser
source or builds a private parser install. Both plain CMake and the
`physics-usd` intent require their CMake cache to resolve the exact parser
prefix returned by `ost library pull`.

Local Windows consumption passed all 52 plain-CMake tests and all 55
OpenStrata `physics-usd` tests with MSVC 19.51, Python 3.13.14, and OpenStrata
0.23.14. Both cache assertions resolved the public parser prefix. The coverage
assertions found all 11 required host tests in CMake and all 13 in Plugin View.

## Hosted public-pin parity

[Stage Runner run 37184752376](https://github.com/animu-sphere/usd-stage-runner/actions/runs/37184752376)
passed on `windows-2025-vs2026` and `ubuntu-24.04` at consumer commit
`7fc37961ed604bd0b2eb22ed8d3c4aac094fd5ac`, using OpenStrata 0.23.1 and the
existing runtime/core/backend pins. The logs identify the public parser prefix
for each target; neither cell checked out or built the sibling parser.

| Check | Windows | Linux |
| --- | --- | --- |
| Compatibility workspace tests before default switch | 47 passed | 47 passed |
| Standard-enabled plain CMake | 52 passed | 52 passed |
| Standard-enabled OpenStrata `physics-usd` intent | 55 passed | 55 passed |

Both cells found all 11 required standard/compatibility host tests in CMake and
all 13 in Plugin View. The ordinary CMake workflow also passed its two core
cells and Jolt cell in
[run 37184752288](https://github.com/animu-sphere/usd-stage-runner/actions/runs/37184752288).

This completed the delivery gate for enabling standard import by default and
switching the host examples to standard fixtures. The follow-up CI clears any
cached standard-import option before the plain-CMake configure, checks the
default workspace's standard/compatibility coverage, and retains a separate
explicit OFF build with the enable-option diagnostic test.

After the switch, the local Windows gate again passed 52 default-enabled
plain-CMake tests, 47 explicit OFF compatibility tests (including
`stage_runner.standard_requires_package`), and 55 OpenStrata intent tests.
The standard configure removed the cached option rather than forcing ON, so
the 11-test coverage assertion also verified the new default.
