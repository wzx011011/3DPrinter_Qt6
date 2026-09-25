---
phase: 25-prepare-model-mesh-rendering-and-camera-interaction
plan: 04
status: verified
verified: 2026-06-27
canonical_command: "powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
canonical_log: build/phase25_04_canonical_verify.log
---

# Phase 25 Verification

Phase 25 completes the gated QRhi Prepare viewport slice for model mesh rendering, camera interaction, and selection/hover feedback. Preview G-code rendering remains Phase 26/27 scope.

## Verification Summary

| Check | Command | Log | Result |
|---|---|---|---|
| Prepare scene data focused tests | `build/PrepareSceneDataTests.exe` | `build/phase25_04_prepare_scene_tests.log` | PASS, exit 0 |
| ViewModel focused smoke tests | `build/ViewModelSmokeTests.exe` | `build/phase25_04_viewmodel_smoke_tests.log` | PASS, exit 0 |
| QML/UI audit tests | `build/QmlUiAuditTests.exe` | `build/phase25_04_qml_ui_audit_tests.log` | PASS, exit 0 |
| Canonical verification | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | `build/phase25_04_canonical_verify.log` | PASS, exit 0 |

Canonical log evidence:

- `[PrepareScene] Prepare scene data tests passed`
- `[UI] QML UI audit tests passed`
- `[E2E] All pipeline tests passed`

Earlier task-level RED/GREEN logs remain preserved under `build/phase25_01_*`, `build/phase25_02_*`, and `build/phase25_03_*`.

## Requirement Evidence

| Requirement | Status | Evidence |
|---|---|---|
| PREP-02 | Complete | `PrepareSceneData` parses packed model mesh batches into source-index-aware model vertices and QRhi renders them through persistent model vertex buffers. Covered by `tests/PrepareSceneDataTests.cpp`, `tests/QmlUiAuditTests.cpp`, and `build/phase25_04_canonical_verify.log`. |
| PREP-03 | Complete | `RhiViewport` owns `CameraController` state, routes orbit/pan/zoom/fit/presets through it, and `RhiViewportRenderer` updates the MVP uniform without `DirtyCamera` causing full model buffer upload. Covered by `tests/QmlUiAuditTests.cpp`. |
| PREP-04 | Complete | `RhiViewport` performs C++ projected-bounds picking and emits `objectPickedSource`; `PreparePage.qml` only forwards to `EditorViewModel::selectSourceObject`; selected/hovered feedback uses a separate QRhi highlight buffer. Covered by `tests/ViewModelSmokeTests.cpp` and `tests/QmlUiAuditTests.cpp`. |
| PREP-07 | Complete | Source-truth mapping comments and planning records map Prepare behavior to upstream `GLCanvas3D`, `PartPlate`, `Camera`, and `Selection` concepts while documenting QRhi as a Qt-native transport difference. Covered by summaries `25-01-SUMMARY.md`, `25-02-SUMMARY.md`, and `25-03-SUMMARY.md`. |

## Technical Evidence

Model rendering:

- `ProjectServiceMock::meshBatchSourceObjectIndices()` mirrors the batch order of `meshData()`.
- `EditorViewModel::meshBatchSourceObjectIndices` and `activePlateObjectIndices` expose renderer-facing source identity.
- `PrepareSceneData::setModelMeshData()` validates packed payloads and active-plate membership before producing model vertices and batches.
- `RhiViewportRenderer::uploadModelBuffer()` uploads model vertices only for mesh, plate, visibility, or GPU dirty state.

Camera:

- `RhiViewport` uses `CameraController` and overrides mouse/wheel input.
- `RhiViewportRenderer` owns `m_cameraUniformBuffer` and `m_cameraMvp`.
- `QmlUiAuditTests::rhiViewportRendererUsesModelBuffersAndCameraUniforms` guards that camera dirty state is not ORed into full model mesh upload conditions.

Selection and hover:

- `EditorViewModel::selectSourceObject(int sourceIndex)` validates against the active plate object set before mutating selection.
- `RhiViewport::pickSourceObjectAt()` and `projectBoundsToScreenRect()` keep picking logic in C++.
- `RhiViewportRenderer::uploadHighlightBuffer()` handles `DirtySelection` through `m_highlightVertexBuffer`, separate from `m_modelVertexBuffer`.
- GL and Software viewport types expose compatible `hoveredSourceObjectIndex` and `objectPickedSource` members so fallback QML loading remains clean.

## Manual QRhi Smoke

No interactive manual QRhi smoke was recorded in this pass. The implementation remains behind `OWZX_RHI_RENDERER=1`, and the canonical script intentionally does not enable that gate. This is acceptable for Phase 25 because the completed scope is guarded by C++ unit/static audits and the default app startup path remains stable.

Manual follow-up when promoting QRhi:

- Launch with `OWZX_RHI_RENDERER=1`.
- Load a small STL/OBJ/3MF.
- Confirm active-plate model visibility, orbit/pan/zoom/fit behavior, right-click context menu behavior, and click/hover selection feedback.

## Residual Risks

- QRhi is still opt-in; Phase 28 owns fallback hardening, failure notification, and review before promotion.
- Picking currently uses projected model batch bounds, not triangle-accurate hit testing. This is adequate for the Phase 25 object-level selection bridge and can be refined later if precise gizmo manipulation requires it.
- Preview G-code rendering is not complete in Phase 25 and remains Phase 26/27 scope.

## Handoff To Phase 26

Phase 26 should consume the same QRhi patterns:

- Keep segment buffers GPU-resident.
- Keep QML as binding/forwarding only.
- Separate layer/current-range/color-mode uniforms or dirty ranges from full G-code segment rebuilds.
- Preserve stable default startup and QRhi gate until Phase 28 promotion work completes.

## Sign-Off

- Focused tests passed.
- Canonical verification passed.
- Phase 25 requirements PREP-02, PREP-03, PREP-04, and PREP-07 are complete.
- Next planned phase: Phase 26, Preview G-Code GPU Pipeline.
