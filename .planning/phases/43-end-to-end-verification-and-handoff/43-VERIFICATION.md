---
phase: 43
artifact: verification
status: passed
verified_at: 2026-06-29T10:36:36+08:00
commit: ac84277
requirements: [VERIFY-01, VERIFY-02, VERIFY-03, VERIFY-04]
manual_uat: pending
---

# Phase 43 Verification

## Commands

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result: passed. Output included:

- `[PrepareScene] Prepare scene data tests passed`
- `[PartPlate] PartPlate tests passed`
- `[UI] QML UI audit tests passed`
- `[E2E] All pipeline tests passed`

```powershell
git diff --check
```

Result: passed; only repository line-ending warnings were emitted.

## Automated Evidence

- `tests/E2EWorkflowTests.cpp::test_local_import_slice_preview_export_workflow` covers the local main path: real STL import through `EditorViewModel`, Prepare slice readiness, slice completion, non-empty `GCV1` Preview payload, layer/move/view interactions, current-plate export, all-valid-plate export, and output byte-size validation.
- `tests/E2EWorkflowTests.cpp::test_import_format_coverage_matrix_real_fixtures` covers STL, 3MF, and OBJ fixture import and recursively checks whether AMF/STEP fixtures exist before classifying them.
- `tests/QmlUiAuditTests.cpp::previewNormalPathCoversFullWorkflowBindingsAndDiagnostics` guards the Preview normal path, D3D11 default ordering, fallback-only `SoftwareViewport` registration, interaction setters that must not clear `previewData`, and runtime diagnostic strings.
- `src/core/services/ProjectServiceMock.cpp`, `src/core/services/SliceService.cpp`, `src/core/viewmodels/PreviewViewModel.cpp`, and `src/qml_gui/Renderer/RhiViewportRenderer.cpp` now log import, slice, export, Preview payload, and render draw-range transitions.

## Renderer Backend Evidence

`build/startup_diagnostics.log` contains the latest canonical smoke launch evidence:

```text
2026-06-29T10:33:25.446 QRhi backend selection: enabled=true requested=auto selected=d3d11 attempts=[d3d11:ok]
```

This verifies the normal Windows Preview path selected D3D11 QRhi, not `SoftwareViewport`.

## Format Matrix

| Format | Phase 43 status | Evidence | Notes |
|---|---|---|---|
| STL | tested | `Prusa.stl` real fixture loaded by `ProjectServiceMock::loadFile`; E2E workflow slices and exports from it | Covered by full workflow test. |
| 3MF | tested | `Geräte/Büchse.3mf` real fixture loaded by `ProjectServiceMock::loadFile` | Verifies non-ASCII project path handling and source-path preservation. |
| OBJ | tested | `20mm_cube.obj` fixture loaded by `ProjectServiceMock::loadFile` | Covered as import-format evidence, not full slice/export workflow source. |
| AMF | classified: fixture unavailable | Recursive repository fixture scan found no committed `.amf` fixture | UI exposes AMF import, but Phase 43 does not claim real AMF behavior without a fixture. |
| STEP/STP | classified: fixture unavailable | Recursive repository fixture scan found no committed `.step` or `.stp` fixture; OCCT is available in build notes, but this path still needs a committed fixture for deterministic verification | Not marked blocked by dependency, only unverified by fixture availability. |

## Requirement Status

- `VERIFY-01`: satisfied by automated E2E workflow coverage.
- `VERIFY-02`: satisfied for STL/3MF/OBJ with explicit AMF/STEP classification.
- `VERIFY-03`: satisfied by QML/source audit guarding Preview normal path and interaction bindings.
- `VERIFY-04`: satisfied by startup, service, viewmodel, and renderer diagnostics.
- `VERIFY-05`: pending manual UAT confirmation in the running app.
