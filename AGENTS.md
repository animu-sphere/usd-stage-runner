# Repository Agent Instructions

## Git workflow

- Never commit or push directly to `main` unless the user explicitly requests
  a direct `main` update.
- For implementation changes, start from the current `origin/main`, create or
  reuse a `codex/*` feature branch, commit there, push that branch, and open a
  pull request targeting `main` when requested.
- Treat an unspecified request to "commit and push" as a feature-branch push,
  not authorization to push directly to `main`.
- Before pushing, verify the current branch and its upstream with
  `git status -sb`.
