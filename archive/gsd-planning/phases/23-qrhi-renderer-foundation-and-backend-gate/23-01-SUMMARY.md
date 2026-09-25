---
phase: 23-qrhi-renderer-foundation-and-backend-gate
plan: 01
subsystem: rendering
tags: [qt6, qrhi, qml, startup, diagnostics]

requires:
  - phase: 23-context
    provides: QRhi renderer gate decisions and fallback contract
provides:
  - Gated OWZX_RHI_RENDERER startup selection
  - D3D12-first and D3D11-fallback QRhi preflight helper
  - Structured backend diagnostics for startup logging
  - QmlUiAudit coverage for default fallback and QRhi gate
affects: [phase-23, phase-24, phase-25, phase-26, phase-28]

tech-stack:
  added: [Qt6::GuiPrivate QRhi private headers]
  patterns:
    - C++ startup selector owns backend policy before QGuiApplication construction
    - QML keeps using a registered GLViewport type while C++ decides implementation

key-files:
  created:
    - src/qml_gui/Renderer/RhiBackendSelector.h
    - src/qml_gui/Renderer/RhiBackendSelector.cpp
  modified:
    - CMakeLists.txt
    - src/qml_gui/main_qml.cpp
    - tests/QmlUiAuditTests.cpp
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md

key-decisions:
  - "OWZX_OPENGL remains the highest-priority legacy override; OWZX_RHI_RENDERER is ignored on that path."
  - "OWZX_RHI_RENDERER uses app-side D3D12 then D3D11 preflight; Vulkan is not part of the app selector with this Qt SDK."
  - "QRhi selection failure records diagnostics and falls back to SoftwareViewport instead of failing startup."

patterns-established:
  - "Renderer startup gates are resolved before QGuiApplication and scenegraph initialization."
  - "QmlUiAuditTests statically guard renderer gate behavior and canonical script defaults."

requirements-completed: [RHI-01, RHI-02, PERF-05]

duration: 2h
completed: 2026-06-27
---

# Phase 23 Plan 01: QRhi Backend Gate And Startup Fallback Summary

**D3D12-first QRhi startup selector with D3D11 fallback, structured diagnostics, and default SoftwareViewport preservation**

## Performance

- **Duration:** 2h
- **Started:** 2026-06-27T00:43:47Z
- **Completed:** 2026-06-27T02:20:00Z
- **Tasks:** 3
- **Files modified:** 8

## Accomplishments

- Added `RhiBackendSelector` with explicit `OWZX_RHI_RENDERER` handling, allowlisted request values, D3D12-first / D3D11-fallback probing, selected graphics API, attempt list, and diagnostic text.
- Updated app startup so default no-env launch still sets `QT_QUICK_BACKEND=software`, `OWZX_OPENGL` still selects legacy OpenGL, and QRhi only runs behind the new explicit gate.
- Extended `QmlUiAuditTests` so default fallback, QRhi gate isolation, canonical script defaults, D3D12/D3D11 policy, and no app-side Vulkan default are regression guarded.

## Task Commits

1. **Task 1-3: backend selector, startup gate, audit guard** - `620865f` (`feat(23-01): add gated qrhi backend selector`)

## Files Created/Modified

- `src/qml_gui/Renderer/RhiBackendSelector.h` - Structured QRhi backend selection result and attempt diagnostics.
- `src/qml_gui/Renderer/RhiBackendSelector.cpp` - Windows QRhi preflight for D3D12 and D3D11 with invalid-env fallback diagnostics.
- `src/qml_gui/main_qml.cpp` - Pre-`QGuiApplication` renderer selection preserving default software and legacy OpenGL paths.
- `CMakeLists.txt` - Adds selector sources and Qt GuiPrivate dependency for QRhi private headers.
- `tests/QmlUiAuditTests.cpp` - Adds static regression guard for QRhi gate and fallback contract.

## Decisions Made

- Kept `OWZX_OPENGL` priority above QRhi so existing debug/fallback behavior is unchanged.
- Kept Phase 23 registration on `SoftwareViewport` until Plan 23-02 introduces `RhiViewport`; Plan 23-01 only selects a QRhi graphics API and logs selection evidence.
- Used Qt GuiPrivate because app-side preflight needs QRhi creation APIs; this matches the existing benchmark target.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- Direct `cmake --build build --target QmlUiAuditTests` outside the vcvars environment failed to link `mpr.lib`; canonical verification through `scripts/auto_verify_with_vcvars.ps1` succeeded and is the authoritative build path.
- The RED test initially required a full rebuild before failing because the previous `build/QmlUiAuditTests.exe` binary had not compiled the new test yet.

## Verification

- RED: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` failed at QML UI audit after adding the test first; `build/qml_audit_red.txt` showed `Unable to read RhiBackendSelector.h`.
- GREEN: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; output included QML UI audit passed and E2E pipeline passed.
- Focused: `build/QmlUiAuditTests.exe -o build/qml_audit_23_01_after_braces.txt,txt` reported 9 passed, 0 failed.
- Static: `rg -n "OWZX_RHI_RENDERER|Direct3D12|Direct3D11|RhiBackendSelector|attempts|failure" src/qml_gui/Renderer CMakeLists.txt`.
- Static: `rg -n "OWZX_RHI_RENDERER|setGraphicsApi|Direct3D12|Direct3D11|SoftwareViewport|appendStartupLog" src/qml_gui/main_qml.cpp`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 23-02. The app can now choose a QRhi scenegraph API behind `OWZX_RHI_RENDERER`; the next plan must add and register the actual `QQuickRhiItem` viewport host.

---
*Phase: 23-qrhi-renderer-foundation-and-backend-gate*
*Completed: 2026-06-27*
