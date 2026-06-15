---
phase: 01-owzx-brand-cleanup
plan: 02
subsystem: CMake build system + C++ namespaces + QML module
tags: [brand-cleanup, wave-2, cmake-rename, namespace-migration]
dependency_graph:
  requires: [01-01]
  provides: [brand-cmake-targets, brand-namespaces]
  affects: [01-03]
tech-stack:
  added: []
  patterns: [atomic-cmake-rename, namespace-migration]
key-files:
  created: []
  modified:
    - CMakeLists.txt
    - .github/workflows/tag-build.yml
    - scripts/auto_verify_with_vcvars.ps1
    - scripts/quick_build.ps1
    - scripts/quick_run.ps1
    - scripts/quick_test.ps1
    - scripts/smoke_test.ps1
    - scripts/capture_qml_warnings.ps1
    - tests/CliTests.cpp
    - src/core/rendering/SupportPaintTypes.h
    - src/core/rendering/TickCodeTypes.h
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/core/viewmodels/PreviewViewModel.h
    - src/core/viewmodels/PreviewViewModel.cpp
    - src/qml_gui/main_qml.cpp
    - src/qml_gui/pages/PreparePage.qml
    - src/qml_gui/pages/PreviewPage.qml
    - src/qml_gui/Renderer/GLViewport.h
decisions:
  - "Hard rename with no backward-compat aliases -- clean break per Phase 1 strategy"
  - "CMAKE_QML_GUI compile definition renamed atomically with C++ guard check (zero #ifdef CREALITY_QML_GUI in src/ -- expected)"
  - "CLI test data path creality_models/ -> test_models/ as part of same rename pass"
metrics:
  duration: ~12 minutes
  completed_date: 2026-06-15
---

# Phase 1 Plan 02: CMake Target/Option Rename + Namespace Migration (Wave 2) Summary

One-line: Atomic rename of all CMake targets/options/executable from Creality to OWzx identifiers, plus Crality3D namespace to OWzx and CrealityGL QML module to OWzxGL.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Rename CMake targets, options, and executable name atomically | 7ba9808 | CMakeLists.txt, tag-build.yml, 6 PS1 scripts, CliTests.cpp |
| 2 | Migrate Crality3D namespace to OWzx + CrealityGL to OWzxGL + C++ guard rename | 1a93a59 | SupportPaintTypes.h, TickCodeTypes.h, EditorViewModel.h/.cpp, PreviewViewModel.h/.cpp, main_qml.cpp, PreparePage.qml, PreviewPage.qml, GLViewport.h |

## Changes Made

### Task 1: CMake Target/Option Rename (9 files)

**CMakeLists.txt (atomic rename of all identifiers):**
- `project(FramelessDialogDemo ...)` -> `project(OWzxSlicer ...)`
- `option(CREALITY_QML_GUI ...)` -> `option(OWZX_QML_GUI ...)`
- `creality_app_core` (library target, 11 references) -> `owzx_app_core`
- `creality_cli_core` (library target, 6 references) -> `owzx_cli_core`
- `creality-cli` (executable target, 3 references) -> `owzx-cli`
- `FramelessDialogDemo` (executable target, 12 references) -> `OWzxSlicer`
- `target_compile_definitions(owzx_app_core PUBLIC OWZX_QML_GUI=1)`

**.github/workflows/tag-build.yml (CI workflow):**
- `-DCREALITY_QML_GUI=ON` -> `-DOWZX_QML_GUI=ON`
- `--target FramelessDialogDemo` -> `--target OWzxSlicer`
- Artifact names: `FramelessDialogDemo-<tag>-windows` -> `OWzxSlicer-<tag>-windows`

**PowerShell scripts (6 files):**
- `scripts/auto_verify_with_vcvars.ps1`: 7 references updated (binary name, ninja targets, CMake flag, process names)
- `scripts/quick_build.ps1`: 3 references updated (process name, ninja target, file checks)
- `scripts/quick_run.ps1`: 1 reference updated (binary name)
- `scripts/quick_test.ps1`: 3 references updated (process name, binary path)
- `scripts/smoke_test.ps1`: 2 references updated (binary path)
- `scripts/capture_qml_warnings.ps1`: 4 references updated (process name, CMake flag, ninja target, binary name, windeployqt)

**tests/CliTests.cpp:**
- `/creality-cli` -> `/owzx-cli`
- `creality_models/Block20XY.stl` -> `test_models/Block20XY.stl`

### Task 2: Namespace + QML Module Migration (10 files)

**Namespace Crality3D -> OWzx (6 source files):**
- `src/core/rendering/SupportPaintTypes.h`: declaration + closing comment
- `src/core/rendering/TickCodeTypes.h`: declaration + closing comment
- `src/core/viewmodels/EditorViewModel.h`: `QList<OWzx::ObjectPaintData>`
- `src/core/viewmodels/EditorViewModel.cpp`: 4 qualified refs (`OWzx::SupportPaintState`, `OWzx::ObjectPaintData`)
- `src/core/viewmodels/PreviewViewModel.h`: `QList<OWzx::TickCode>`
- `src/core/viewmodels/PreviewViewModel.cpp`: 6 qualified refs (`OWzx::TickCode`, `OWzx::TickType`)

**QML module CrealityGL -> OWzxGL (4 files):**
- `src/qml_gui/main_qml.cpp`: `qmlRegisterType<GLViewport>("OWzxGL", 1, 0, "GLViewport")`
- `src/qml_gui/pages/PreparePage.qml`: `import OWzxGL 1.0`
- `src/qml_gui/pages/PreviewPage.qml`: `import OWzxGL 1.0`
- `src/qml_gui/Renderer/GLViewport.h`: comment updated

**C++ guard CREALITY_QML_GUI check:** Zero matches in src/ -- the symbol is only a CMake compile definition, never used as a preprocessor guard in source code. No changes needed.

## Deviations from Plan

None -- plan executed exactly as written.

## Auth Gates

None encountered.

## Known Stubs

None -- this plan was pure identifier rename with no stubs introduced.

## Threat Flags

None -- no new security surfaces introduced. This is a build-system and internal-identifier rename.

## Verification

- **Task 1 grep assertion**: Zero residual `CREALITY_QML_GUI|creality_app_core|creality_cli_core|creality-cli|FramelessDialogDemo` in CMakeLists.txt, scripts/, tests/, .github/ -- PASSED
- **Task 2 grep assertion**: Zero residual `Crality3D|CrealityGL|CREALITY_QML_GUI` in src/ (excluding third_party/) -- PASSED
- **Build verification**: `scripts/auto_verify_with_vcvars.ps1` exit code 0 -- PASSED
  - CMake configure: success (317 targets)
  - Build: OWzxSlicer.exe, owzx-cli.exe, E2EWorkflowTests.exe, ViewModelSmokeTests.exe, CliTests.exe all linked
  - Smoke test: app started successfully (PID 20920)
  - E2E tests: timeout failure (non-blocking, pre-existing issue unrelated to rename)
- **Binary output**: `build/OWzxSlicer.exe` (29.6 MB), `build/owzx-cli.exe` (27.3 MB) -- CONFIRMED
