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

Hosted Windows/Linux parity and the standard-default switch remain pending.
