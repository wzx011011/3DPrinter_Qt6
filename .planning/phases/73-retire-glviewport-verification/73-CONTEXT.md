---
phase: 73-retire-glviewport-verification
subsystem: rhi-renderer
tags: [rhi, cleanup, verification, source-truth]
created: 2026-07-04
---

# Phase 73 Context

## Goal

Remove the redundant OpenGL `GLViewport` implementation and verify the RHI path
is the only functional hardware viewport path.

## Requirements

- `GRET-01`: `GLViewportRenderer.{cpp,h}`, `GLViewport.{cpp,h}`, and
  `GCodeRenderer`'s `GLViewport` dependency are removed; the build succeeds
  without them.
- `GRET-02`: `OWZX_OPENGL` no longer activates a separate GL path; only
  `RhiViewport` and `SoftwareViewport` remain registered.

## Current State

- `RhiViewport` is already registered under the QML type name `GLViewport` on
  the default QRhi path.
- `SoftwareViewport` remains a non-RHI fallback registered under the same QML
  type name when QRhi cannot initialize.
- `RhiViewportRenderer` owns both Prepare scene rendering and Preview G-code
  rendering through `previewData`, `roleVisibility`, draw ranges, and GCV1
  parsing.
- `GLViewport` is only referenced from the explicit `OWZX_OPENGL` startup branch
  and creates either `GLViewportRenderer` or `GCodeRenderer`.
- `GCodeRenderer` is not the active Preview path after the RHI migration; it is
  only reachable through deleted `GLViewport::createRenderer`.

## Constraints

- Keep the QML import/type contract stable: QML may continue to instantiate
  `OWzxGL.GLViewport`, but that type must resolve to `RhiViewport` on the normal
  path or `SoftwareViewport` on fallback.
- Do not rewrite Prepare/Preview QML just to rename `GLViewport`; the enum
  names and bindings are part of the compatibility surface.
- Keep D3D11-first QRhi policy and explicit `OWZX_RHI_RENDERER` override.
- Do not create new build directories; use `build/` only.
- Use the canonical verifier for final verification:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

## Source Truth

The source-truth behavior has already been ported from the GL path across
Phases 65-72:

- Gizmo math and picking helpers are pure C++.
- RHI renderer draws move, rotate, scale, cut plane, and wipe tower.
- RHI viewport handles gizmo drag, object picking, and Preview G-code data.

This phase is a retirement/verification phase, not a behavior redesign.

## Audit Targets

- `src/qml_gui/main_qml.cpp`
- `CMakeLists.txt`
- `tests/QmlUiAuditTests.cpp`
- `src/qml_gui/Renderer/GLViewport.{h,cpp}`
- `src/qml_gui/Renderer/GLViewportRenderer.{h,cpp}`
- `src/qml_gui/Renderer/GCodeRenderer.{h,cpp}`
