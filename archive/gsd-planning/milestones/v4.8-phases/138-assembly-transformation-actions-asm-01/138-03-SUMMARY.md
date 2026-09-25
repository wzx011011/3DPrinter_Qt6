# Summary 138-03: Renderer Thread-Through + AssemblePage UI

**Phase:** 138 — Assembly Transformation Actions ASM-01
**Plan:** 138-03 (Wave 3, deps 138-01, 138-02)
**Status:** Complete
**Requirement:** ASM-01 (visual slice)

## What was done

Closed the visual loop for ASM-01: the assembly canvas now renders assembled volumes at their per-instance assemble-pose offset, and AssemblePage exposes a Move/Rotate/Scale mode selector wired to the existing gizmo drag signals. After this plan the feature is visually end-to-end: gizmo drag → assemble transform written (Plan 02) → volume re-renders at new pose.

### Renderer approach chosen: translate-only

The plan allowed full-matrix compose OR translate-only (with rotate/scale render fidelity as a documented follow-up). I shipped **translate-only** for low risk:
- The Move gizmo is the primary ASM-01 interaction and its visual is fully correct.
- Rotate/scale of the assemble pose are fully **persisted** (Plan 01 accessors + Plan 02 routing write all three axes) and **round-trip** (Plan 04 asserts via real 3MF). Only the live rotate/scale *visual* on the canvas is approximated (the volume's world-space vertices reflect the ordinary transform, not the assemble rotate/scale).
- This avoids the risk of a per-vertex QMatrix4x4 compose in the hot render path; the assemble offset is applied as a parallel per-batch translation, exactly mirroring the existing explosion-offset block.

A full-matrix compose (build QMatrix4x4 from assemble pos/rot/scale, transform each vertex) is the documented render-fidelity follow-up.

### Changes

**`src/core/viewmodels/EditorViewModel.h` / `.cpp`**
- New `Q_PROPERTY(QVariantList assembleOffsets ...)` with `NOTIFY stateChanged`, exposing one `QVector3D` per source object index (parallel to `meshBatchSourceObjectIndices`). Implementation `assembleOffsets()` proxies to `projectService_->assembleOffset(objectIndex)` (Plan 01).

**`src/qml_gui/Renderer/RhiViewport.h` / `.cpp`**
- New `Q_PROPERTY(QVariantList assembleOffsets ...)` + `setAssembleOffsets` setter that marks scene dirty + `update()` (mirroring `setMeshBatchSourceObjectIndices`).

**`src/qml_gui/Renderer/RhiViewportRenderer.h` / `.cpp`**
- New `m_assembleOffsets` + `m_assembleOffsetBySource` (QHash<int, QVector3D>) members.
- `synchronize()`: mirrors `m_assembleOffsets` from the viewport, force model re-upload on change, builds `m_assembleOffsetBySource` by zipping the offsets with the parallel `meshBatchSourceObjectIndices` list (skip zero offsets).
- `buildModelVertices`: new `CanvasAssembleView`-gated block (after the explosion-offset block) that applies each batch's assemble offset to every vertex in `[firstVertex, firstVertex+vertexCount)`. Prepare (`CanvasView3D`) and Preview (`CanvasPreview`) paths unchanged.

**`src/qml_gui/pages/AssemblePage.qml`**
- New `assembleTransformMode` (default GizmoMove) + `activeGizmoDragMode` properties + `setAssembleGizmoIfAvailable(mode)` helper (mask-gated, mirrors PreparePage).
- New Move/Rotate/Scale selector (3 `CxButton`s) in the topBar, each gated on `editorVm.availableGizmoMask` bits 0/1/2, `qsTr()`-labeled.
- `gizmoMode` binding now tracks `assembleTransformMode` when Assembly Measure is inactive (was hard-GizmoMove).
- New `assembleOffsets` binding on the GLViewport.
- New gizmo drag-signal handlers (`onGizmoDragBegin`/`onGizmoMoveRequested`/`onGizmoRotateRequested`/`onGizmoScaleRequested`/`onGizmoDragEnd`) mirrored from PreparePage.qml — forward to `editorVm.begin/apply/endGizmo*Drag` (Plan 02 routing handles the assemble-target branching).
- Assembly Measure (Ctrl+Y) wiring unchanged — takes precedence in `gizmoMode`.

## Verification

- Canonical build: **exit 0**, 0 errors (`build_p138_03c.log`). (One iteration: fixed a `std::min` qsizetype/int mismatch in the synchronize hash-build.)
- ctest: 5/5 groups PASS — **QmlUiAuditTests PASS** (AssemblePage.qml QML audit clean), ViewModel PASS (no Prepare regression).
- E2E pipeline: PASS.
- App launch liveness: `APP_RUNNING_PID=24472`.

## Human-gated step (must_haves)

Manual UI smoke: launch app, enter assembly view, select a single volume, pick Move mode, drag the gizmo, confirm the volume translates on screen. This is the one step the automated build/ctest cannot cover (the translate-only render path is exercised by the QML audit + build, but the live drag interaction needs a human eye). Deferred to Phase 140 manual verification gate.

## Render-fidelity follow-up (documented, out of scope)

- Assemble rotate/scale live-visual compose (full QMatrix4x4 per-vertex transform). The transforms persist and round-trip regardless; only the live rotate/scale visual is approximated.
