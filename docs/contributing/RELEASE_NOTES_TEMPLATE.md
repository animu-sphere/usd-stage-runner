# usd-stage-runner {tag}

A small real-time runtime host for OpenUSD stages: the frame clock, input,
character, camera and vehicle libraries, the `runnerSchema` bundle and the
`stage_runner` application.

{changelog}

## Consuming a package from another repository

Each member is published as a `.tar.zst` asset below and as a digest-pinned
OpenStrata artifact in `ghcr.io/animu-sphere/usd-stage-runner`, tagged
`<member>-{version}-<target>`. Paste the entries you need under
`requires.libraries`, `requires.bundles` or `requires.tools`:

{pins}

## Artifacts

| artifact | contents |
| --- | --- |
| `<library>-{version}-<target>.tar.zst` | one installed library for `<target>` |
| `runnerSchema-{version}-<target>.tar.zst` | the schema bundle |
| `stage_runner-{version}-<target>.tar.zst` | the application |
| `<member>-{version}-<target>.manifest.json` | its OpenStrata manifest |
| `external-library-pins.md` / `.json` | the pin table above, as a file |
| `usd-stage-runner-{version}-src.tar.gz` | source archive at this tag |
| `SHA256SUMS` | SHA-256 checksums of every file above |

## SHA-256 checksums

```text
{checksums}
```
