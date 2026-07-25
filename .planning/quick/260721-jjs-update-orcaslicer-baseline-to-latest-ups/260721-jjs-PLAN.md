---
quick_id: 260721-jjs
slug: update-orcaslicer-baseline-to-latest-ups
date: 2026-07-21
status: in_progress
mode: quick
validation: enabled
---

# Quick Task 260721-jjs: Update the OrcaSlicer source-truth baseline

## Goal

Move `third_party/OrcaSlicer` to official `upstream/main` commit `8b93cc5df3347c657ce6ac9a58f6923a21c2959b`, retain every reviewed load-bearing OWzx compatibility delta, and record a reproducible baseline before right-click implementation begins.

This is gate 1 of a four-gate execution requested by the user:

1. Baseline compatibility (this task)
2. Viewport context hit and selection routing
3. Menu contract and bounded backend action slices
4. Full source-truth verification and migration closeout

## Task 1: Reconstruct the submodule branch

1. Create `owzx-upstream-2026-07-21` from `upstream/main` at `8b93cc5d...`; preserve the old local branch.
2. Cherry-pick/reapply only:
   - `2976d14778`: `ConfigOptionEnumGeneric` and enum-vector null-`keys_map` guards.
   - the minimal CGAL 5.4 `MeshBoolean.cpp` overload compatibility delta from `b3cec7363e`.
3. Inspect `git diff 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD`. It must contain only the two reviewed compatibility changes and their English ASCII comments.
4. Add `configEnumNullKeysMapGuards()` to `tests/ViewModelSmokeTests.cpp`, covering scalar serialize/deserialize and vector serialize/deserialize with a null `keys_map`.

## Task 2: Prove compatibility

1. After the canonical build produces the test binary, run `build\ViewModelSmokeTests.exe configEnumNullKeysMapGuards` and require exit code 0.
2. Run `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py` from the root, then explicitly check the two changed submodule files: `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py src/libslic3r/Config.hpp src/libslic3r/MeshBoolean.cpp` from `third_party/OrcaSlicer`. Do not use the submodule-wide `--all` mode because an unrelated tracked quoted/non-ASCII 3MF path triggers WinError 123 on Windows.
3. Run the canonical verification command from the root:
   `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
4. Record configure, build, app smoke/E2E, ViewModelSmokeTests execution, and QML warning results.

## Task 3: Record the baseline

1. Update `docs/源码真值基线.md` with official upstream ref/commit/date, resulting OWzx child commit, preserved compatibility deltas, and latest stable tag as informational only. Verify with `rg -n "8b93cc5d|v2\.4\.2|ConfigOptionEnumGeneric|CGAL 5\.4" docs/源码真值基线.md`.
2. Update active baseline consumers that declare the stale lock: `AGENTS.md`, `docs/源码对照迁移任务追踪.md`, `docs/v3.6-ui-inventory.md`, `docs/架构梳理_快速认知.md`, and `docs/架构梳理.html`. Preserve historical wording only when it is explicitly labeled historical rather than active.
3. Update the root submodule gitlink only after verification passes.
4. Keep the submodule and parent commits separate. Do not stage unrelated dirty files.

## Reproducibility Gate

The parent gitlink must not be committed until its child commit is reachable from the configured fork. After explicit user authorization: push `owzx-upstream-2026-07-21`; verify `git ls-remote origin refs/heads/owzx-upstream-2026-07-21` returns the exact child SHA; create the local parent gitlink/docs commit without publishing it; clone the root from the local repository at that exact parent commit into a temporary directory; run `git submodule update --init third_party/OrcaSlicer`; only then may the parent commit be published. Without authorization, leave the parent gitlink uncommitted and report the baseline gate incomplete.

## Done Criteria

- `git merge-base --is-ancestor 8b93cc5d... HEAD` succeeds inside the submodule.
- `git diff 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD` contains only `Config.hpp` null-map guards and CGAL 5.4 MeshBoolean compatibility.
- `git diff --name-only 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD` equals exactly `src/libslic3r/Config.hpp` and `src/libslic3r/MeshBoolean.cpp`.
- Focused tests and the canonical full verification pass.
- Encoding guard passes.
- Baseline/tracker docs contain no active `v7.0.1 / 0d4ac73...` claim.
- `git ls-remote` proves the configured fork branch resolves to the exact child SHA, and a fresh temporary root clone completes `git submodule update --init third_party/OrcaSlicer`.
- The next quick task is explicitly seeded as viewport context hit/selection routing.

## Changed-File Allowlist

- `third_party/OrcaSlicer` gitlink and, inside the submodule, only `src/libslic3r/Config.hpp`, `src/libslic3r/MeshBoolean.cpp`, plus a focused upstream-compatible test file if required.
- `tests/ViewModelSmokeTests.cpp`
- `AGENTS.md`
- `docs/源码真值基线.md`
- `docs/源码对照迁移任务追踪.md`
- `docs/v3.6-ui-inventory.md`
- `docs/架构梳理_快速认知.md`
- `docs/架构梳理.html`
- This quick task's planning/summary/verification artifacts.
