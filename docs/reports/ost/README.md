# OpenStrata Dogfooding Reports

These reports record what using [OpenStrata](https://github.com/animu-sphere/open-strata)
in `usd-stage-runner` was actually like. They are upstream feedback first and a
local evidence trail second.

The series is append-only. Do not rewrite an old report to match a later
explanation or fix. Add a new report and place a short forward-note on the old
one when later evidence supersedes it.

| # | Date | Report | `ost` | Focus |
| --- | --- | --- | --- | --- |
| 1 | 2026-08-30 | [A local Windows build finishes work but does not return](01-2026-08-30-v0.22.8-windows-ninja-wait.md) | 0.22.8 | Local `ost build`, per-library build, and direct CMake/Ninja invocations remain alive after output stops; both Windows CI paths pass on the same commit. **Live P1 diagnostic ask** |
| 2 | 2026-08-30 | [The Windows runtime exports its producer's Python paths](02-2026-08-30-v0.22.6-runtime-python-paths.md) | 0.22.6 | A direct consumer on `windows-2022` cannot configure against the pulled runtime until its Python dependency hints and imported-target include paths are repaired. **Upstream runtime-export issue** |
| 3 | 2026-09-02 | [Plugin View cannot compose a usdview host add-on with `--with`](03-2026-09-02-v0.22.8-usdview-host-plugin-composition.md) | 0.22.8 | The shared Stage Runner adapter works when staged inside the schema bundle, but `--with` has no truthful bundle kind for a Python usdview `PluginContainer`; the selected runtime also lacks usdview for Level 6. **Upstream modeling ask; repository workaround implemented** |
