---
phase: 35
status: passed
verified: 2026-06-28
requirements:
  - RENDER-01
  - RENDER-02
  - RENDER-03
  - TEST-03
---

# Phase 35 Verification: D3D11 Preview Rendering Interaction

## Status

Passed.

## Evidence

- RED: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 1 after adding `previewRhiRendererBindsPreviewStateAndUsesExactDrawSpans`; the canonical pipeline stopped at QML UI audit before renderer changes.
- GREEN: the same canonical command exited 0 after replacing proportional Preview playback with exact draw spans.
- Automated suites reported by the canonical command:
  - PrepareScene data tests passed.
  - PartPlate tests passed.
  - QML UI audit tests passed.
  - E2E pipeline tests passed.
- Startup diagnostics include the normal QRhi auto path selecting D3D11:
  - `QRhi backend selection: enabled=true requested=auto selected=d3d11 attempts=[d3d11:ok]`.

## Requirement Check

| Requirement | Result | Evidence |
|---|---|---|
| RENDER-01 | Passed | Normal startup keeps `RhiViewport` as `GLViewport`; latest diagnostics show `selected=d3d11`; audit prevents direct `SoftwareViewport` Preview usage. |
| RENDER-02 | Passed | Renderer now indexes packed segments by exact layer and move, and draws zero segments for `moveEnd <= 0`. |
| RENDER-03 | Passed | Slider changes reuse the uploaded segment buffer and only alter draw range; canonical E2E pipeline remains passing. |
| TEST-03 | Passed | `QmlUiAuditTests::previewRhiRendererBindsPreviewStateAndUsesExactDrawSpans` guards Preview bindings and renderer range logic. |

## Residual Risk

- Manual visual UAT for the full load -> slice -> Preview flow remains in Phase 36.
