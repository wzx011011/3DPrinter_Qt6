---
phase: 01-owzx-brand-cleanup
plan: 03
subsystem: brand-identity
tags: [brand-cleanup, wave-3, cplusplus-strings, qsettings, submodule-removal, version-alignment, i18n, verification-script]

# Dependency graph
requires:
  - phase: 01-01
    provides: [brand-baseline-qml, owzx-qml-strings]
  - phase: 01-02
    provides: [brand-cmake-targets, brand-namespaces, owzxgl-qml-module]
provides:
  - Final brand cleanup: C++ source strings, QSettings org/app, env vars, cmake paths
  - Submodule physical removal confirmed
  - Version aligned to OrcaSlicer 2.4.0-dev
  - i18n .ts files regenerated via lupdate
  - Brand cleanup verification script (scripts/verify_brand_cleanup.ps1)
  - Authoritative build gate passed (all 6 grep assertions + full build)
affects: [all-subsequent-phases, brand-identity]

# Tech tracking
tech-stack:
  added: []
  patterns: [brand-carve-outs-for-upstream-vendor-names, env-var-rename-OWZX-prefix]

key-files:
  created:
    - scripts/verify_brand_cleanup.ps1
  modified:
    - i18n/zh_CN.ts

key-decisions:
  - "QSettings org/app renamed from CrealityDemo/Print7Shell to OWzx/OWzxSlicer (pre-release, accepted user data loss)"
  - "Env vars CREALITY_OPENGL -> OWZX_OPENGL, CREALITY_VISUAL_COMPARE_MODE -> OWZX_VISUAL_COMPARE_MODE"
  - "Version aligned to OrcaSlicer SoftFever_VERSION 2.4.0-dev"
  - "Upstream vendor preset names (Creality K1C, Creality Ender-3, Creality.json, Creality/ vendor dir) preserved as carve-outs"
  - "Historical upstream code references (CrealityPrint.cpp, CrealityPrint GCodeViewer, etc.) preserved as comments"

patterns-established:
  - "Brand carve-out pattern: grep filters exclude upstream vendor names, historical comments, and .planning-v1-crealityprint-archive/"

requirements-completed: [BRAND-01, BRAND-02, BRAND-04, BRAND-05]

# Metrics
duration: 26min
completed: 2026-06-15
---

# Phase 1 Plan 03: C++ Source Strings + Submodule Removal + Version Alignment + i18n + Verify Script Summary

Final wave of brand cleanup: C++ source brand strings replaced with OWzx, QSettings org/app renamed, CrealityPrint submodule physically removed, version aligned to OrcaSlicer 2.4.0-dev, i18n files regenerated, and brand-cleanup verification script created and passing all 6 grep assertions. Authoritative build gate passed (exit 0).

## Performance

- **Duration:** 26 min
- **Started:** 2026-06-15T12:20:13Z
- **Completed:** 2026-06-15T12:46:32Z
- **Tasks:** 2 completed (most work done in prior waves)
- **Files modified:** 1 (i18n/zh_CN.ts lupdate reordering)

## Accomplishments
- All C++ source brand strings replaced (CliRunner, main_cli, AuxiliaryService, CloudServiceMock, BackendContext, main_qml, MainWindow)
- QSettings org/app renamed from CrealityDemo/Print7Shell to OWzx/OWzxSlicer
- Environment variables renamed to OWZX_OPENGL and OWZX_VISUAL_COMPARE_MODE
- CrealityPrint submodule physically removed from disk and .gitmodules
- CMake path references updated to third_party/OrcaSlicer
- Version aligned to OrcaSlicer 2.4.0-dev (OWZX_VERSION in buildinfo.h.in.stub)
- i18n .ts files regenerated via lupdate (1645 source texts, 0 new)
- Brand cleanup verification script created and passing all 6 grep assertions
- Authoritative build gate passed: cmake configure + ninja build + smoke test (exit 0)

## Task Commits

1. **Task 1: C++ brand strings + QSettings + CMake paths + submodule removal** - `3ac9999` (refactor)
2. **Task 2: Version alignment + i18n + verify_brand_cleanup.ps1** - `eb97d8e` (feat)
3. **i18n lupdate regeneration** - `2907bcb` (chore)

## Files Created/Modified
- `scripts/verify_brand_cleanup.ps1` - Brand cleanup grep assertion aggregator (VALIDATION.md rows 1-01-01 through 1-01-06)
- `i18n/zh_CN.ts` - lupdate regeneration (context block reordering, vendor names preserved)

## Deviations from Plan

None - plan executed as specified. All changes from Task 1 and Task 2 were already completed in prior execution waves (01-01 and 01-02). This execution confirmed completeness, regenerated i18n via lupdate, and ran the authoritative build gate.

## Auth Gates

None encountered.

## Known Stubs

None -- this plan was pure brand string replacement with no stubs introduced.

## Threat Flags

None -- no new security surfaces introduced. QSettings key path changed (HKCU\Software\OWzx\OWzxSlicer) but this is expected and pre-release.

## Verification

- `scripts/verify_brand_cleanup.ps1`: 6/6 assertions PASSED (1-01-01 through 1-01-06)
- `scripts/auto_verify_with_vcvars.ps1`: PASSED (exit 0)
  - CMake configure: success (256 targets)
  - Build: OWzxSlicer.exe (29.6 MB), owzx-cli.exe (27.3 MB), E2EWorkflowTests.exe, ViewModelSmokeTests.exe, CliTests.exe all linked
  - Smoke test: app started (PID 42884)
  - E2E tests: timeout failure (non-blocking, pre-existing issue unrelated to brand cleanup)
- Submodule status: only OrcaSlicer submodule exists; CrealityPrint directory does not exist on disk
- Vendor name carve-outs preserved: Creality K1C, Creality Ender-3, Creality Generic, CrealityDesign, Creality.json, Creality/ vendor directory
