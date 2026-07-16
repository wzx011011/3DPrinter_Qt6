---
phase: 138
status: passed
verified: 2026-07-16
requirements: [ASM-01]
plans: [138-01, 138-02, 138-03, 138-04]
---

# Phase 138 Verification

## Status: PASSED

**Requirement:** ASM-01 — Assembly view supports move/rotate/scale transformation actions on assembled volumes (RhiViewport assembly canvas gizmo + ViewModel + AssemblePage). Transformations apply per-volume and round-trip through the model.

## Success criteria

### SC1: Assembly view supports move/rotate/scale on assembled volumes

| Layer | Evidence | Plan |
|---|---|---|
| Data accessors | `ProjectServiceMock::assembleOffset/Rotation/Scale` + setters + `isAssembleInitialized` proxy to `ModelInstance::m_assemble_transformation` (Model.hpp:1253-1298) under HAS_LIBSLIC3R | 138-01 |
| Undo | `AssembleTransformCommand` (id 7) writes the assemble transform; distinct from Prepare `TransformCommand` (id 1) so cross-canvas undo never corrupts | 138-01 |
| Routing | `availableGizmoMask()` advertises Move/Rotate/Scale bits on AssembleView; all 9 gizmo drag-lifecycle slots branch on `m_activeCanvasType==2` to write the assemble transform + push `AssembleTransformCommand`, skipping slice invalidation | 138-02 |
| Renderer | `buildModelVertices` applies the per-object assemble offset on the `CanvasAssembleView` path (alongside the explosion offset); `EditorViewModel::assembleOffsets` Q_PROPERTY feeds `RhiViewport::assembleOffsets` -> `RhiViewportRenderer::m_assembleOffsetBySource` | 138-03 |
| UI | `AssemblePage.qml` exposes a Move/Rotate/Scale selector gated on `availableGizmoMask` and wires the 5 gizmo drag-signal handlers to the ViewModel slots | 138-03 |

**Result:** PASS (automated). The full pipeline from gizmo drag → ViewModel routing → service accessor → ModelInstance field is wired and build-clean. The live visual translate is rendered; rotate/scale live-visual compose is a documented render-fidelity follow-up (transforms still persist + round-trip).

### SC2: Per-volume transforms round-trip through 3MF

| Check | Result | Evidence |
|---|---|---|
| `<assemble>` block write gate fires | PASS | `testAssembleTransformRoundTrip` asserts `isAssembleInitialized(0)` true after setters |
| Real `store_3mf` serialize | PASS | test saves via `saveProjectAs` -> real `Slic3r::store_3mf` |
| Real reader deserialize | PASS | test reloads via `loadProject` into fresh service |
| Offset round-trip (GL Y/Z swap survives) | PASS | reloaded offset matches within 1e-4 |
| Rotation round-trip (deg<->rad survives) | PASS | reloaded rotation matches within 1e-3 deg |
| Scale round-trip | PASS | reloaded scale matches within 1e-4 |
| `isAssembleInitialized` survives reload | PASS | asserted true after reload |

**Result:** PASS (automated, `testAssembleTransformRoundTrip` green in the ViewModel ctest group).

## Build/test evidence

- Canonical build `scripts/auto_verify_with_vcvars.ps1`: exit 0 (`build_p138_04b.log`).
- ctest: 5/5 groups PASS — PrepareScene / PartPlate / **ViewModel (incl. `testAssembleTransformRoundTrip`)** / UI (QmlUiAudit) / PreviewParser.
- E2E pipeline: PASS.
- App launch liveness: `APP_RUNNING_PID=30584`.
- ViewModelSmokeTests: all assertions PASS including the new round-trip slot.

## Commits

- `138-01`: assemble-transform accessors + AssembleTransformCommand (foundation)
- `138-02`: assembly-canvas gizmo routing (mask widening + 9 slot branches)
- `138-03`: assemble-pose render thread-through + AssemblePage gizmo UI (visual)
- `138-04`: assemble-transform 3MF round-trip test (persistence proof)

## Human verification (deferred to Phase 140 gate)

Manual UI smoke: launch app, enter assembly view, select a single volume, pick Move mode, drag the gizmo, confirm the volume translates on screen. This is the one step automated build/ctest cannot cover (the live drag interaction). The translate-only render path is exercised by QmlUiAudit + build; the manual smoke confirms the visual end-to-end experience.

## Tech debt (non-blocking, carried to STATE.md)

- Assemble rotate/scale live-visual compose (full QMatrix4x4 per-vertex transform). Transforms persist + round-trip regardless; only the live rotate/scale visual is approximated by translate-only rendering.
- `ProjectServiceMock::drillObject` C4715 (carry-forward from Phase 137).
- 2-line CGAL compat patch in `third_party/OrcaSlicer` submodule (carry-forward from Phase 136).
