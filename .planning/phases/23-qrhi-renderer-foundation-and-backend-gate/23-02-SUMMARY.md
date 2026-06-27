---
phase: 23-qrhi-renderer-foundation-and-backend-gate
plan: 02
subsystem: rendering
tags: [qt6, qrhi, qml, shaders, viewport]

requires:
  - phase: 23-plan-01
    provides: Gated QRhi backend selection and fallback contract
provides:
  - QQuickRhiItem viewport registered as OWzxGL.GLViewport only after QRhi gate success
  - Minimal QRhi renderer pipeline with app-managed .qsb shader resources
  - GLViewport-compatible property and enum surface for existing Prepare/Preview bindings
  - QmlUiAudit coverage for QRhi host, shader build, and default fallback preservation
affects: [phase-23, phase-24, phase-25, phase-26]

tech-stack:
  added: [QQuickRhiItem, QQuickRhiItemRenderer, Qt6::ShaderTools]
  patterns:
    - C++ registers one implementation under the existing QML GLViewport type name
    - QML keeps binding viewport properties; renderer state and backend policy remain in C++
    - App shaders are compiled by qt_add_shaders and loaded from Qt resources

key-files:
  created:
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/qml_gui/Renderer/shaders/rhi_viewport.vert
    - src/qml_gui/Renderer/shaders/rhi_viewport.frag
  modified:
    - CMakeLists.txt
    - src/qml_gui/main_qml.cpp
    - tests/QmlUiAuditTests.cpp
    - .planning/REQUIREMENTS.md
    - .planning/STATE.md

key-decisions:
  - "RhiViewport is registered as OWzxGL.GLViewport only when OWZX_RHI_RENDERER preflight succeeds."
  - "SoftwareViewport remains the default and failure fallback; OWZX_OPENGL still selects legacy GLViewport."
  - "Phase 23 QRhi drawing is a diagnostic primitive only; real Prepare mesh and Preview G-code rendering remain deferred to Phases 24-26."

requirements-completed: [RHI-03, RHI-05]

duration: 2h
completed: 2026-06-27
---

# Phase 23 Plan 02: QQuickRhiItem Viewport Host And Shader Pipeline Summary

**QRhi viewport host, shader resources, and explicit-gate registration are in place without changing the default startup path.**

## Performance

- **Duration:** 2h
- **Started:** 2026-06-27T01:20:00Z
- **Completed:** 2026-06-27T02:14:00Z
- **Tasks:** 3
- **Files modified:** 11

## Accomplishments

- Added `RhiViewport : QQuickRhiItem` with the property and enum surface expected by current `PreparePage.qml` and `PreviewPage.qml` bindings.
- Added `RhiViewportRenderer : QQuickRhiItemRenderer` with QRhi resource lifecycle, shader loading, static vertex buffer upload, and diagnostic triangle rendering.
- Added `rhi_viewport.vert` / `rhi_viewport.frag` and wired them through `qt_add_shaders` under `:/rhi_viewport/shaders`.
- Updated startup registration so `OWZX_OPENGL` still maps to legacy `GLViewport`, successful `OWZX_RHI_RENDERER` maps to `RhiViewport`, and default/failure maps to `SoftwareViewport`.
- Extended `QmlUiAuditTests` to guard the QRhi host, shader resource wiring, explicit gate, and canonical verification defaults.

## Task Commits

1. **Task 1-3: QRhi host, shader pipeline, registration and audit guard** - `ed75889` (`feat(23-02): add qrhi viewport host`)

## Files Created/Modified

- `src/qml_gui/Renderer/RhiViewport.h` - QML item contract with GLViewport-compatible properties and enums.
- `src/qml_gui/Renderer/RhiViewport.cpp` - State setters, update scheduling, thumbnail stub, and renderer creation.
- `src/qml_gui/Renderer/RhiViewportRenderer.h` - QRhi renderer class and resource state.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - Shader loading, pipeline creation, static vertex upload, and diagnostic draw.
- `src/qml_gui/Renderer/shaders/rhi_viewport.vert` - Vertex shader for the diagnostic primitive.
- `src/qml_gui/Renderer/shaders/rhi_viewport.frag` - Fragment shader for vertex-color output.
- `CMakeLists.txt` - Adds renderer sources and Qt Shader Tools `.qsb` resource generation.
- `src/qml_gui/main_qml.cpp` - Registers `RhiViewport` under the existing `OWzxGL.GLViewport` type after QRhi selection success.
- `tests/QmlUiAuditTests.cpp` - Adds static regression checks for host, shaders, gate, and fallback.

## Decisions Made

- Kept the QRhi item API compatible with existing viewport bindings so QML pages do not need renderer-specific branches.
- Kept all renderer policy and state in C++; QML remains a binding layer.
- Kept this plan to the host and shader pipeline. Mesh upload, plate draw, camera interaction, and G-code segment buffers are deliberately left to the later phase plans where their data contracts are defined.

## Deviations from Plan

- The audit test initially checked `QQuickRhiItemRenderer` in the `.cpp`; the actual inheritance is correctly declared in `RhiViewportRenderer.h`. The test was corrected to validate the header for the base class and the `.cpp` for shader resource loading.

## Issues Encountered

- First GREEN attempt failed QML UI audit after compile because the test asserted the renderer base-class string in the wrong source file.
- Earlier compile failed on `<QShader>` include; the working Qt 6.10 private-header pattern is `<rhi/qshader.h>` alongside `<rhi/qrhi.h>`.

## Verification

- RED: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` failed at QML UI audit after adding the 23-02 test first; `build/qml_audit_23_02_red.txt` showed `Unable to read RhiViewport.h`.
- GREEN: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; output included QML UI audit passed and E2E pipeline passed.
- Focused: `build/QmlUiAuditTests.exe -o build/qml_audit_23_02_green.txt,txt` reported 9 passed, 0 failed.
- Static: `rg -n "class RhiViewport|QQuickRhiItem|QQuickRhiItemRenderer|Q_PROPERTY\\(QByteArray meshData|CanvasPreview|GizmoMove" src/qml_gui/Renderer`.
- Static: `rg -n "qt_add_shaders\\(OWzxSlicer|rhi_viewport|RhiViewport|Qt6::GuiPrivate|ShaderTools" CMakeLists.txt src/qml_gui/Renderer`.
- Static: `rg -n "qmlRegisterType<.*RhiViewport|qmlRegisterType<SoftwareViewport>|qmlRegisterType<GLViewport>|OWZX_RHI_RENDERER|OWZX_OPENGL" src/qml_gui/main_qml.cpp tests/QmlUiAuditTests.cpp`.

## User Setup Required

None. QRhi is still opt-in via `OWZX_RHI_RENDERER`; default app launch remains on `SoftwareViewport`.

## Next Phase Readiness

Ready for Plan 23-03. The app now has a gated QRhi viewport host and compiled shader resources; the final Phase 23 plan should close benchmark diagnostics, verification artifacts, and traceability before moving to real Prepare/Preview rendering phases.

---
*Phase: 23-qrhi-renderer-foundation-and-backend-gate*
*Completed: 2026-06-27*
