# Changelog

Notable changes to the released runner. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). While the version is
0.x, a minor version may change the public API or ABI.

## [0.1.0] - 2026-10-31

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
- Windows packages are built with MSVC 19.51 on `windows-2025-vs2026`. Link them
  with that toolset or newer.
- `stage_runner` needs the OpenUSD runtime beside it; it has not been run from
  the extracted package outside the release lane's suites.
- macOS is not a release target.
