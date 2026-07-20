# Phase 182: Editor Workflow Fix + Architecture-Divergence Research — SUMMARY

**Status:** Executed
**Workstream:** CL
**Requirement:** CRASH-03
**Executed:** 2026-07-20

## Outcome

- **A7 (STEP reload_from_disk):** FIXED — 1 file, ~40 lines added (project-relative path fallback).
- **A3/A5/A6/A10:** NOT APPLICABLE — see RESEARCH.md (all 4 are upstream-wxWidgets/OpenGL-specific with no Qt6 equivalent).

## A7 Fix Detail

### Problem

After a 3MF save+reload cycle, `obj->input_file` may hold only a bare filename (upstream `store_bbs_3mf` default does not preserve full source path). `ProjectServiceMock::reloadFromDisk` called `ReadSTLFile(obj->input_file.c_str())` directly, which fails on a bare filename → reload silently falls through to repairing the existing mesh (no actual reload).

This mirrors upstream OrcaSlicer #14591 (commit `5ba5c6672d`): "reload_from_disk matched reloaded source volumes with an exact source.input_file string comparison. After a project is saved and reopened, the stored source path is only the filename... so the comparison never matched."

### Fix

`src/core/services/ProjectServiceMock.cpp:4268-4360` — added a `resolveSourcePath` lambda that:
1. Tries the stored `input_file` as-is (covers absolute paths and same-cwd files).
2. On failure, extracts the filename portion via `QFileInfo::fileName()` and resolves it relative to `currentProjectPath_`'s directory (same-folder lookup, mirroring upstream's fallback).
3. Returns empty string if neither works (caller falls back to repairing existing mesh, preserving prior behavior).

The resolved path is then used for `ReadSTLFile` instead of the raw `obj->input_file`.

### Qt6 adaptation note

Upstream's fix is in `Plater::priv::reload_from_disk` and uses a volume-by-volume `source.input_file` comparison (because upstream reloads a whole selection of volumes). Qt6's `reloadFromDisk` reloads a single object's volumes via `obj->input_file`, so the fix is simpler: one path resolution per object, applied to all volumes.

### Files changed

- `src/core/services/ProjectServiceMock.cpp` (~40 lines added in `reloadFromDisk`)

### Verification

- Build: `scripts/auto_verify_with_vcvars.ps1` — 0 errors (build log: `build_v5_4_p182.log`).
- Manual test path: save project with STL model → close → reopen → trigger reload_from_disk → STL reloads even if absolute path moved (as long as file is next to the 3MF).

## A3/A5/A6/A10 — all NOT APPLICABLE

See RESEARCH.md for the per-item investigation. All 4 had zero grep matches for their bug-signature tokens in the corresponding Qt6 files, confirming Qt6 architecture has no equivalent code paths.

## Pattern conclusion (across Phase 180/181/182)

**All 10 P11.B Qt-side crash fixes investigated in v5.4 are NOT APPLICABLE except A7.** The 9 N/A items (A1/A2/A3/A4/A5/A6/A8/A9/A10) all target upstream-wxWidgets/ImGui/OpenGL-specific code with no Qt6 equivalent. Only A7 (STEP reload path resolution) had a real Qt6 counterpart and got fixed.

**Implication:** The 2026-07-19 bb3 sync introduced zero Qt6-side regressions in the crash/behavior-fix category. Qt6 architecture is structurally immune to the upstream bug classes that bb3 fixed. The one real fix (A7) was a pre-existing Qt6 limitation (bare-filename reload failure) that bb3's corresponding upstream fix prompted us to address.

## Code changes total: ~40 lines (1 file)

## Recommendation for Phase 187 (REGRESS-08)

For CRASH-03 anchor, assert the A7 fix landed:
- Verify `ProjectServiceMock.cpp` contains `resolveSourcePath` (the lambda name) OR `currentProjectPath()` call inside `reloadFromDisk`.
- This locks the project-relative fallback so a future refactor can't silently remove it.
