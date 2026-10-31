# Changelog

Notable changes to the released runner. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). While the version is
0.x, a minor version may change the public API or ABI.

## [0.1.0] - unreleased

The first versioned release, for `cy2026` / `usd` on Windows x86_64 and Linux
x86_64, against `usd-physics-plugins` 0.1.

### Added

- Libraries: `runtimeCore`, `inputCore`, `characterCore`, `cameraCore`,
  `vehicleCore`, `stageRuntime` and the `inputSdl` backend.
- The `runnerSchema` bundle and the `stage_runner` application.
- Opt-in standard `UsdPhysics` scene rebuild through installed `physicsUsd`, beside
  the compatibility path.
- A release workflow publishing each member by digest, with a generated consumer
  pin table.

### Known limitations

- `stageRuntime`, `stage_runner` and `runnerSchema` need the Jolt SDK that
  `physicsJolt` recorded; the package does not contain it.
- The hosted release lanes install no SDL, so the published `inputSdl` reports no
  backend. A device-input build needs SDL at configure and is source-built.
- macOS is not a release target.
